import copy
import json
from pathlib import Path
import sys
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import action_rules as ar


def document():
    return ar.build_document("test", [
        {"id": "Attack", "displayName": "普攻", "abilityTags": ["Character.State.Skill.Attack"], "stateTags": ["Character.State.Skill.Attack"]},
        {"id": "Dash", "displayName": "冲刺", "abilityTags": ["Character.State.Movement.Dash"], "stateTags": ["Character.State.Movement.Dash"]},
    ], [{"id": "R001", "enabled": False, "subject": "player", "current": "Dash", "request": "Attack", "effect": "block", "condition": "", "reason": "示例"}])


class RulesTests(unittest.TestCase):
    def test_empty_identifiers_and_reason(self):
        for location in ("root", "action", "rule", "reason"):
            for value in ("", "   ", None):
                source = document()
                target = source if location == "root" else source["actions" if location == "action" else "rules"][0]
                target["reason" if location == "reason" else "id"] = value
                with self.subTest(location=location, value=value), self.assertRaises(ar.RuleError):
                    ar.validate_document(source)

    def test_runtime_size_and_count_limits(self):
        for section, limit in (("actions", ar.MAX_ACTIONS), ("rules", ar.MAX_RULES)):
            source = document()
            source[section] = [source[section][0]] * (limit + 1)
            with self.assertRaisesRegex(ar.RuleError, "maximum"):
                ar.validate_document(source)
        with tempfile.TemporaryDirectory() as directory, patch.object(ar, "MAX_RUNTIME_SOURCE", 10):
            output = Path(directory) / "too-large.json"
            with self.assertRaisesRegex(ar.RuleError, "runtime limit"):
                ar.write_document(document(), output)
            self.assertFalse(output.exists())

    def test_ability_tag_parent_overlap(self):
        for tag in ("Character.State.Skill", "Character.State.Skill.Attack", "Character.State.Skill.Attack.1"):
            source = document()
            source["actions"][1]["abilityTags"] = [tag]
            with self.subTest(tag=tag), self.assertRaisesRegex(ar.RuleError, "Overlapping"):
                ar.validate_document(source)

    def test_self_interrupt_rejected(self):
        source = document()
        source["rules"][0].update(current="Attack", effect="interrupt")
        with self.assertRaisesRegex(ar.RuleError, "self interruption"):
            ar.validate_document(source)

    def test_lua_comments_and_strings_do_not_export_functions(self):
        snippets = [
            '--[=[ fake = function(ctx) return true end ]=]\nreturn { functions = {} }',
            'return { functions = { real = function(ctx) local s = "fake = function(ctx)" return true end } }',
            'return { functions = { real = function(ctx) local t = { fake = function(ctx) return true end } return true end } }',
            'return { functions = { real = function(ctx) return true end }, fake = function(ctx) return true end }',
        ]
        for snippet in snippets:
            with self.subTest(snippet=snippet):
                self.assertNotIn("fake", ar._lua_declared_functions(snippet))

    def test_lua_duplicate_declaration_rejected(self):
        with self.assertRaisesRegex(ar.RuleError, "Duplicate Lua"):
            ar._lua_declared_functions('return { functions = { a = function(ctx) return true end, a = function(ctx) return false end } }')

    def test_lua_sample_exports(self):
        root = Path(__file__).resolve().parents[3]
        source = root / "Content/ActionRules/Examples/conditions.lua"
        self.assertEqual(ar._lua_declared_functions(source.read_text(encoding="utf-8")),
                         {"player_only", "has_stamina", "in_cancel_window"})

    def test_same_input_output_refused_with_force(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "rules.json"
            original = json.dumps(document())
            path.write_text(original, encoding="utf-8")
            with patch("sys.stderr", new_callable=__import__("io").StringIO):
                result = ar.main(["--input", str(path), "--output", str(path), "--force"])
            self.assertEqual(result, 2)
            self.assertEqual(path.read_text(encoding="utf-8"), original)

    def test_all_authoring_roundtrips(self):
        root = Path(__file__).resolve().parents[3]
        template = root / "Tools/ActionRules/Templates/ActionRules_Template.xlsx"
        if not template.is_file():
            self.fail("Versioned authoring template is missing")
        expected = ar.read_json(root / "Content/ActionRules/Examples/player_combat.example.json")
        shared = ["--id", "PlayerCombat.Example", "--tags-file", str(root / "Config/Tags/CharacterTag.ini")]
        cases = [
            ["--input", str(template), *shared],
            ["--input", str(root / "Tools/ActionRules/examples/actions.md"), *shared],
            ["--actions-csv", str(root / "Tools/ActionRules/examples/Actions.csv"), "--rules-csv", str(root / "Tools/ActionRules/examples/Rules.csv"), *shared],
            ["--input", str(root / "Content/ActionRules/Examples/player_combat.example.json")],
        ]
        with tempfile.TemporaryDirectory() as directory:
            for index, arguments in enumerate(cases):
                output = Path(directory) / f"route-{index}.json"
                result = subprocess.run([sys.executable, str(root / "Tools/ActionRules/action_rules.py"), *arguments, "--output", str(output)],
                                        capture_output=True, text=True, encoding="utf-8")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(ar.read_json(output), expected)

    def test_python_example_cli(self):
        root = Path(__file__).resolve().parents[3]
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "python-example.json"
            result = subprocess.run([sys.executable, str(root / "Tools/ActionRules/examples/build_rules.py"), "--output", str(output)],
                                    capture_output=True, text=True, encoding="utf-8")
            self.assertEqual(result.returncode, 0, result.stderr)
            value = ar.read_json(output)
            self.assertEqual(len(value["actions"]), 3)
            self.assertEqual(len(value["rules"]), 4)
            self.assertFalse(any(row["enabled"] for row in value["rules"]))

    def test_saved_workbook_native_features(self):
        root = Path(__file__).resolve().parents[3]
        template = root / "Tools/ActionRules/Templates/ActionRules_Template.xlsx"
        if not template.is_file():
            self.fail("Versioned authoring template is missing")
        with zipfile.ZipFile(template) as archive:
            sheets = [ET.fromstring(archive.read(f"xl/worksheets/sheet{index}.xml")) for index in (1, 2)]
            for sheet in sheets:
                self.assertEqual(sheet.find("m:sheetViews/m:sheetView", ar.NS).get("showGridLines"), "0")
                self.assertEqual(sheet.find("m:sheetViews/m:sheetView/m:pane", ar.NS).get("ySplit"), "1")
                self.assertEqual(len(sheet.findall("m:tableParts/m:tablePart", ar.NS)), 1)
                self.assertFalse(sheet.findall("m:sheetData/m:row/m:c/m:f", ar.NS))
            validations = sheets[1].findall("m:dataValidations/m:dataValidation", ar.NS)
            self.assertEqual({item.get("sqref") for item in validations}, {"B2:B1001", "C2:C1001", "F2:F1001"})
            self.assertEqual(len(sheets[1].findall("m:conditionalFormatting/m:cfRule", ar.NS)), 1)

    def test_case_variant_duplicate_tags(self):
        source = document()
        source["actions"][0]["stateTags"] = ["Good.Tag", "good.tag"]
        with self.assertRaisesRegex(ar.RuleError, "duplicate"):
            ar.validate_document(source)

    def test_output_cannot_replace_script(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "my_script.py"
            with self.assertRaisesRegex(ar.RuleError, "must be a .json"):
                ar.write_document(document(), output, force=True)
            self.assertFalse(output.exists())

    def test_lua_source_utf8_size_limit(self):
        source = document()
        source["luaFile"] = "conditions.lua"
        with tempfile.TemporaryDirectory() as directory:
            script = Path(directory) / "conditions.lua"
            base = 'return { functions = {} }'
            script.write_text(base + ' ' * (ar.MAX_LUA_SOURCE - len(base)), encoding="utf-8")
            ar.validate_lua_references(source, directory)
            script.write_text(base + ' ' * (ar.MAX_LUA_SOURCE - len(base) + 1), encoding="utf-8")
            with self.assertRaisesRegex(ar.RuleError, "64 KiB"):
                ar.validate_lua_references(source, directory)
            script.write_text('--' + '中' * 22000 + '\n' + base, encoding="utf-8")
            with self.assertRaisesRegex(ar.RuleError, "64 KiB"):
                ar.validate_lua_references(source, directory)

    def test_condition_identifier_length_limit(self):
        source = document()
        source["luaFile"] = "conditions.lua"
        source["rules"][0]["condition"] = 'a' * 64
        ar.validate_document(source)
        source["rules"][0]["condition"] = 'a' * 65
        with self.assertRaisesRegex(ar.RuleError, "1..64"):
            ar.validate_document(source)

    def test_normalize_does_not_mutate(self):
        source = document()
        before = copy.deepcopy(source)
        self.assertEqual(ar.validate_document(source), before)
        self.assertEqual(source, before)

    def test_duplicate_actions(self):
        source = document()
        source["actions"].append(copy.deepcopy(source["actions"][0]))
        with self.assertRaisesRegex(ar.RuleError, "duplicate"):
            ar.validate_document(source)

    def test_duplicate_rules(self):
        source = document()
        source["rules"].append(copy.deepcopy(source["rules"][0]))
        with self.assertRaisesRegex(ar.RuleError, "duplicate"):
            ar.validate_document(source)

    def test_unknown_field(self):
        for place in ("root", "action", "rule"):
            source = document()
            target = source if place == "root" else source["actions" if place == "action" else "rules"][0]
            target["typo"] = True
            with self.subTest(place=place), self.assertRaisesRegex(ar.RuleError, "unknown"):
                ar.validate_document(source)

    def test_invalid_enums_and_refs(self):
        for key, value in (("subject", "hero"), ("effect", "allow"), ("request", "Missing"), ("current", "Missing")):
            source = document()
            source["rules"][0][key] = value
            with self.subTest(key=key), self.assertRaises(ar.RuleError):
                ar.validate_document(source)

    def test_boolean_strict(self):
        for value in ("false", "true", 0, 1, None):
            source = document()
            source["rules"][0]["enabled"] = value
            with self.subTest(value=value), self.assertRaises(ar.RuleError):
                ar.validate_document(source)
        for value in ("yes", "1", "", 1):
            with self.subTest(value=value), self.assertRaises(ar.RuleError):
                ar._boolean(value, "Enabled")
        self.assertFalse(ar._boolean("FALSE", "Enabled"))

    def test_tag_registry(self):
        with self.assertRaisesRegex(ar.RuleError, "unregistered"):
            ar.validate_document(document(), {"Unknown"})
        tags = {a["abilityTags"][0] for a in document()["actions"]}
        self.assertEqual(ar.validate_document(document(), tags), document())

    def test_bad_and_duplicate_tags(self):
        for tags in (("Bad..Tag",), ("Good.Tag", "Good.Tag"), ()):
            source = document()
            source["actions"][0]["abilityTags"] = list(tags)
            with self.subTest(tags=tags), self.assertRaises(ar.RuleError):
                ar.validate_document(source)

    def test_condition_requires_lua_and_name(self):
        source = document()
        source["rules"][0]["condition"] = "cancel_window"
        with self.assertRaisesRegex(ar.RuleError, "luaFile"):
            ar.validate_document(source)
        source["luaFile"] = "conditions.lua"
        ar.validate_document(source)
        source["rules"][0]["condition"] = "return true"
        with self.assertRaises(ar.RuleError):
            ar.validate_document(source)

    def test_lua_path_cannot_escape(self):
        for path in ("../x.lua", "C:/x.lua", "a/b.lua", "x.py", "..lua", "a\\x.lua"):
            source = document()
            source["luaFile"] = path
            with self.subTest(path=path), self.assertRaises(ar.RuleError):
                ar.validate_document(source)

    def test_lua_missing_and_undefined_preserves_output(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "rules.json"
            source = document()
            source["luaFile"] = "conditions.lua"
            source["rules"][0]["condition"] = "cancel_window"
            with self.assertRaisesRegex(ar.RuleError, "missing"):
                ar.write_document(source, output)
            lua = output.with_name("conditions.lua")
            lua.write_text("return { functions = { other = function(ctx) return true end } }", encoding="utf-8")
            with self.assertRaisesRegex(ar.RuleError, "declarations missing"):
                ar.write_document(source, output)
            self.assertFalse(output.exists())
            lua.write_text("return { functions = { cancel_window = function(ctx) return ctx.subject == 'player' end } }", encoding="utf-8")
            ar.write_document(source, output)
            self.assertTrue(output.exists())

    def test_no_overwrite_and_force(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "rules.json"
            ar.write_document(document(), output)
            before = output.read_bytes()
            altered = document()
            altered["id"] = "changed"
            with self.assertRaisesRegex(ar.RuleError, "already exists"):
                ar.write_document(altered, output)
            self.assertEqual(output.read_bytes(), before)
            ar.write_document(altered, output, force=True)
            self.assertEqual(ar.read_json(output)["id"], "changed")

    def test_invalid_force_retains_good_output(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "rules.json"
            ar.write_document(document(), output)
            before = output.read_bytes()
            broken = document()
            broken["rules"][0]["effect"] = "allow"
            with self.assertRaises(ar.RuleError):
                ar.write_document(broken, output, force=True)
            self.assertEqual(output.read_bytes(), before)

    def test_json_duplicate_key_and_nan(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.json"
            for text in ('{"id":"a","id":"b"}', '{"x":NaN}'):
                path.write_text(text)
                with self.assertRaises(ar.RuleError):
                    ar.read_json(path)

    def test_csv_quote_bom_and_blank_row(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "actions.csv"
            path.write_text('ID,DisplayName,AbilityTags,StateTags\nAttack,"普攻,一",Character.State.Skill.Attack,Character.State.Skill.Attack\n,,,\n', encoding="utf-8-sig")
            result = ar._rows(ar.read_csv(path), ar.ACTION_COLUMNS, "Actions")
            self.assertEqual(len(result), 1)
            self.assertEqual(result[0]["DisplayName"], "普攻,一")

    def test_csv_unknown_headers_and_formula(self):
        with self.assertRaises(ar.RuleError):
            ar._rows([["Bad"]], ar.ACTION_COLUMNS, "Actions")
        with self.assertRaisesRegex(ar.RuleError, "formulas"):
            ar._rows([ar.ACTION_COLUMNS, ["Attack", "=1+2", "Good.Tag", "Good.Tag"]], ar.ACTION_COLUMNS, "Actions")

    def test_markdown_contract(self):
        sample = Path(__file__).resolve().parents[1] / "examples" / "actions.md"
        tables = ar.read_markdown(sample)
        result = ar.tables_document(tables, "example")
        self.assertEqual(len(result["actions"]), 3)
        self.assertEqual(len(result["rules"]), 4)
        self.assertFalse(any(r["enabled"] for r in result["rules"]))

    def test_tags_ini(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "tags.ini"
            path.write_text('[x]\nGameplayTagList=(Tag="Good.Tag",DevComment="Example")\n', encoding="utf-8")
            self.assertEqual(ar.read_registered_tags(path), {"Good.Tag"})

    def test_xlsx_formula_rejected_even_cached(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "formula.xlsx"
            ns = ar.NS["m"]
            with zipfile.ZipFile(path, "w") as archive:
                archive.writestr("xl/workbook.xml", f'<workbook xmlns="{ns}" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><sheets><sheet name="Actions" r:id="r1"/></sheets></workbook>')
                archive.writestr("xl/_rels/workbook.xml.rels", '<Relationships><Relationship Id="r1" Target="worksheets/sheet1.xml"/></Relationships>')
                archive.writestr("xl/worksheets/sheet1.xml", f'<worksheet xmlns="{ns}"><sheetData><row r="1"><c r="A1"><f>1+1</f><v>2</v></c></row></sheetData></worksheet>')
            with self.assertRaisesRegex(ar.RuleError, "formulas unsupported"):
                ar.read_xlsx(path)

    def test_cli_bad_source_no_output(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "rules.json"
            with patch("sys.stderr", new_callable=__import__("io").StringIO):
                result = ar.main(["--input", "bad.py", "--id", "test", "--output", str(output)])
            self.assertEqual(result, 2)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
