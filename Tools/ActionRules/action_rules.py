#!/usr/bin/env python3
"""Offline action-rule authoring. No Unreal dependency and no script execution."""
from __future__ import annotations

import argparse
import csv
import io
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ACTION_COLUMNS = ("ID", "DisplayName", "AbilityTags", "StateTags")
RULE_COLUMNS = ("ID", "Enabled", "Subject", "Current", "Request", "Effect", "Condition", "Reason")
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_.-]{0,127}\Z")
FUNCTION = re.compile(r"[A-Za-z_][A-Za-z0-9_]{0,63}\Z")
TAG = re.compile(r"[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+)*\Z")
NS = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
REL = "{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id"
MAX_FILE = 8 * 1024 * 1024
MAX_ROWS = 10000
MAX_RUNTIME_SOURCE = 1024 * 1024
MAX_LUA_SOURCE = 64 * 1024
MAX_ACTIONS = 1024
MAX_RULES = 8192


class RuleError(ValueError):
    """Invalid authoring input; no output should be committed."""


def _text(value, where, *, empty=False):
    if not isinstance(value, str) or (not empty and not value.strip()):
        raise RuleError(f"{where}: expected {'optional ' if empty else ''}text")
    if len(value) > 4096 or any(ord(c) < 32 and c not in "\n\t\r" for c in value):
        raise RuleError(f"{where}: text too long or contains control characters")
    return value.strip()


def _keys(value, required, optional, where):
    if not isinstance(value, dict):
        raise RuleError(f"{where}: expected object")
    missing, extra = set(required) - value.keys(), value.keys() - set(required) - set(optional)
    if missing or extra:
        raise RuleError(f"{where}: missing={sorted(missing)}, unknown={sorted(extra)}")


def _id(value, where):
    value = _text(value, where)
    if not IDENTIFIER.fullmatch(value):
        raise RuleError(f"{where}: use a stable ASCII ID, not a row number")
    return value


def _unique(values, where):
    if len(set(values)) != len(values):
        raise RuleError(f"{where}: duplicate value")


def validate_document(document, registered_tags=None):
    """Validate/normalize a JSON-compatible document. Never mutate the caller's data."""
    _keys(document, ("schemaVersion", "id", "actions", "rules"), ("luaFile",), "root")
    if type(document["schemaVersion"]) is not int or document["schemaVersion"] != 1:
        raise RuleError("schemaVersion: only integer 1 is supported")
    result = {"schemaVersion": 1, "id": _id(document["id"], "id"), "actions": [], "rules": []}
    if "luaFile" in document:
        lua = _text(document["luaFile"], "luaFile")
        if not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*\.lua", lua) or ".." in lua:
            raise RuleError("luaFile: use a sibling .lua basename, without directories or traversal")
        result["luaFile"] = lua
    for section, limit in (("actions", MAX_ACTIONS), ("rules", MAX_RULES)):
        if not isinstance(document[section], list) or len(document[section]) > limit:
            raise RuleError(f"{section}: expected array, maximum {limit} rows")
    if not document["actions"]:
        raise RuleError("actions: at least one action is required")
    for index, action in enumerate(document["actions"], 1):
        where = f"actions[{index}]"
        _keys(action, ("id", "displayName", "abilityTags", "stateTags"), (), where)
        clean = {"id": _id(action["id"], where + ".id"),
                 "displayName": _text(action["displayName"], where + ".displayName")}
        for field in ("abilityTags", "stateTags"):
            tags = action[field]
            if not isinstance(tags, list) or not tags or len(tags) > 128:
                raise RuleError(f"{where}.{field}: requires 1..128 registered tag names")
            clean[field] = [_text(tag, where + "." + field) for tag in tags]
            _unique([tag.casefold() for tag in clean[field]], where + "." + field)
            for tag in clean[field]:
                if not TAG.fullmatch(tag):
                    raise RuleError(f"{where}.{field}: invalid tag {tag!r}")
                if registered_tags is not None and tag not in registered_tags:
                    raise RuleError(f"{where}.{field}: unregistered tag {tag!r}")
        result["actions"].append(clean)
    ids = [action["id"] for action in result["actions"]]
    _unique(ids, "actions.id")
    # GAS parent matching must identify at most one action. A parent and child on
    # separate action definitions are ambiguous, even if no current rule uses them.
    tag_owners = {}
    for action in result["actions"]:
        for tag in action["abilityTags"]:
            key = tag.casefold()
            previous = tag_owners.get(key)
            if previous and previous != action["id"]:
                raise RuleError(f"Overlapping action abilityTags: {previous} / {action['id']}")
            tag_owners[key] = action["id"]
    for tag, owner in tag_owners.items():
        parts = tag.split(".")
        for end in range(1, len(parts)):
            parent_owner = tag_owners.get(".".join(parts[:end]))
            if parent_owner and parent_owner != owner:
                raise RuleError(f"Overlapping action abilityTags: {parent_owner} / {owner}")
    for index, rule in enumerate(document["rules"], 1):
        where = f"rules[{index}]"
        _keys(rule, ("id", "enabled", "subject", "current", "request", "effect", "reason"), ("condition",), where)
        if type(rule["enabled"]) is not bool:
            raise RuleError(f"{where}.enabled: expected JSON boolean, not text or number")
        clean = {"id": _id(rule["id"], where + ".id"), "enabled": rule["enabled"]}
        for field, allowed in (("subject", ("any", "player", "enemy")), ("effect", ("block", "interrupt"))):
            if rule[field] not in allowed:
                raise RuleError(f"{where}.{field}: expected one of {allowed}")
            clean[field] = rule[field]
        for field in ("current", "request"):
            clean[field] = _id(rule[field], where + "." + field)
            if clean[field] not in ids:
                raise RuleError(f"{where}.{field}: unknown action {clean[field]!r}")
        if clean["current"] == clean["request"] and clean["effect"] == "interrupt":
            raise RuleError(f"{where}: self interruption is not supported")
        condition = _text(rule.get("condition", ""), where + ".condition", empty=True)
        if condition and not FUNCTION.fullmatch(condition):
            raise RuleError(f"{where}.condition: expected Lua function name of 1..64 ASCII characters, not code")
        if condition and "luaFile" not in result:
            raise RuleError(f"{where}.condition: a named condition requires luaFile")
        clean["condition"] = condition
        clean["reason"] = _text(rule["reason"], where + ".reason")
        result["rules"].append(clean)
    _unique([rule["id"] for rule in result["rules"]], "rules.id")
    return result


def build_document(identifier, actions, rules, lua_file=None, registered_tags=None):
    """Public Python builder API. Script authors call this and write_document themselves."""
    document = {"schemaVersion": 1, "id": identifier, "actions": actions, "rules": rules}
    if lua_file:
        document["luaFile"] = lua_file
    return validate_document(document, registered_tags)


def _read(path):
    path = Path(path)
    if path.stat().st_size > MAX_FILE:
        raise RuleError(f"{path}: maximum input size is {MAX_FILE} bytes")
    return path.read_text(encoding="utf-8-sig")


def _json_pairs(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise RuleError(f"JSON duplicate key: {key}")
        out[key] = value
    return out


def read_json(path):
    return json.loads(_read(path), object_pairs_hook=_json_pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(RuleError(f"Invalid JSON number: {value}")))


def read_registered_tags(path):
    """Read newline tags, a JSON string array, or UE GameplayTagList INI (no execution)."""
    text = _read(path)
    if text.lstrip().startswith("[") and Path(path).suffix.lower() == ".json":
        tags = json.loads(text)
        if not isinstance(tags, list) or not all(isinstance(tag, str) for tag in tags):
            raise RuleError("tags-file: expected JSON array of tag names")
    elif re.search(r"(?m)^\s*\+?GameplayTagList\s*=", text):
        tags = re.findall(r'(?m)^\s*\+?GameplayTagList\s*=\s*\(\s*Tag="([^"]+)"', text)
    else:
        tags = [line.strip() for line in text.splitlines() if line.strip() and not line.lstrip().startswith("#")]
    if not tags or any(not TAG.fullmatch(tag) for tag in tags):
        raise RuleError("tags-file: empty or malformed tag dictionary")
    return set(tags)


def _rows(rows, columns, sheet):
    if not rows:
        raise RuleError(f"{sheet}: missing header")
    if tuple(rows[0]) != columns:
        raise RuleError(f"{sheet}: header must be exactly {','.join(columns)}")
    result = []
    for index, row in enumerate(rows[1:], 2):
        if not any(value not in ("", None) for value in row):
            continue
        if len(row) != len(columns):
            raise RuleError(f"{sheet} row {index}: expected {len(columns)} columns")
        if any(isinstance(value, str) and value.lstrip().startswith("=") for value in row):
            raise RuleError(f"{sheet} row {index}: formulas are not rule data; paste values")
        result.append(dict(zip(columns, row)))
    if len(result) > MAX_ROWS:
        raise RuleError(f"{sheet}: too many rows")
    return result


def _boolean(value, where):
    if type(value) is bool:
        return value
    if isinstance(value, str) and value.strip().lower() in ("true", "false"):
        return value.strip().lower() == "true"
    raise RuleError(f"{where}: use TRUE or FALSE, not 0/1, yes/no or blank")


def tables_document(tables, identifier, lua_file=None, registered_tags=None):
    actions = []
    for row in _rows(tables["Actions"], ACTION_COLUMNS, "Actions"):
        actions.append({"id": row["ID"], "displayName": row["DisplayName"],
                        "abilityTags": _text(row["AbilityTags"], "AbilityTags").split(";"),
                        "stateTags": _text(row["StateTags"], "StateTags").split(";")})
    rules = []
    for row in _rows(tables["Rules"], RULE_COLUMNS, "Rules"):
        rules.append({"id": row["ID"], "enabled": _boolean(row["Enabled"], str(row["ID"]) + ".Enabled"),
                      "subject": row["Subject"], "current": row["Current"], "request": row["Request"],
                      "effect": row["Effect"], "condition": row["Condition"] or "", "reason": row["Reason"] or ""})
    return build_document(identifier, actions, rules, lua_file, registered_tags)


def read_csv(path):
    return list(csv.reader(io.StringIO(_read(path)), strict=True))


def read_markdown(path):
    tables, active = {}, None
    for line in _read(path).splitlines():
        if line.startswith("## "):
            active = line[3:].strip()
            if active in tables:
                raise RuleError(f"Markdown: duplicate section {active}")
            if active in ("Actions", "Rules"):
                tables[active] = []
        elif active in ("Actions", "Rules") and line.strip().startswith("|"):
            if "\\|" in line:
                raise RuleError("Markdown escaped pipes are unsupported; use CSV for pipe text")
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            if all(re.fullmatch(r":?-{3,}:?", cell) for cell in cells):
                continue
            tables[active].append(cells)
    if set(tables) != {"Actions", "Rules"}:
        raise RuleError("Markdown requires ## Actions and ## Rules tables")
    return tables


def read_xlsx(path):
    """Small bounded OOXML reader. Reads cell values only; rejects formulas/macros."""
    path = Path(path)
    if path.stat().st_size > MAX_FILE:
        raise RuleError("XLSX input too large")
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        if len(set(names)) != len(names) or len(names) > 1000 or sum(i.file_size for i in archive.infolist()) > 32 * MAX_FILE:
            raise RuleError("XLSX contains duplicate members or exceeds size limits")
        if any("vbaproject" in name.lower() or name.startswith("xl/externalLinks/") for name in names):
            raise RuleError("XLSX macros/external links are not supported")
        def xml(name):
            if archive.getinfo(name).file_size > MAX_FILE:
                raise RuleError(f"XLSX member too large: {name}")
            value = archive.read(name)
            if b"\x00" in value or b"<!DOCTYPE" in value.upper() or b"<!ENTITY" in value.upper():
                raise RuleError("XLSX XML entity declarations are not supported")
            return ET.fromstring(value)
        rels = {item.attrib["Id"]: item.attrib for item in xml("xl/_rels/workbook.xml.rels")}
        shared = []
        if "xl/sharedStrings.xml" in names:
            shared = ["".join(node.text or "" for node in list(el.findall("m:t", NS)) + list(el.findall("m:r/m:t", NS)))
                      for el in xml("xl/sharedStrings.xml").findall("m:si", NS)]
        tables = {}
        for sheet in xml("xl/workbook.xml").findall("m:sheets/m:sheet", NS):
            name = sheet.attrib["name"]
            if name not in ("Actions", "Rules"):
                continue
            if name in tables:
                raise RuleError("XLSX duplicate named sheet")
            relation = rels[sheet.attrib[REL]]
            target = relation["Target"]
            if relation.get("TargetMode") == "External" or ".." in target.split("/") or "\\" in target:
                raise RuleError("XLSX unsafe sheet relationship")
            target = target.lstrip("/") if target.startswith("/") else "xl/" + target
            values = {}
            width = len(ACTION_COLUMNS if name == "Actions" else RULE_COLUMNS)
            for cell in xml(target).findall("m:sheetData/m:row/m:c", NS):
                match = re.fullmatch(r"([A-Z]+)([1-9][0-9]*)", cell.attrib["r"])
                if not match:
                    raise RuleError("XLSX invalid cell address")
                col = 0
                for char in match[1]:
                    col = col * 26 + ord(char) - 64
                row = int(match[2])
                if row > MAX_ROWS + 1:
                    raise RuleError("XLSX row limit exceeded")
                if cell.find("m:f", NS) is not None:
                    raise RuleError(f"{name}!{cell.attrib['r']}: formulas unsupported, even with cached values; paste values")
                kind = cell.get("t", "n")
                raw = cell.findtext("m:v", "", NS)
                if kind == "inlineStr":
                    value = "".join(el.text or "" for el in list(cell.findall("m:is/m:t", NS)) + list(cell.findall("m:is/m:r/m:t", NS)))
                elif kind == "s":
                    try:
                        if int(raw) < 0:
                            raise ValueError()
                        value = shared[int(raw)]
                    except (ValueError, IndexError):
                        raise RuleError("XLSX invalid shared string") from None
                elif kind == "b":
                    if raw not in ("0", "1"):
                        raise RuleError("XLSX invalid boolean")
                    value = raw == "1"
                elif kind == "e":
                    raise RuleError(f"{name}!{cell.attrib['r']}: Excel error {raw}")
                elif kind in ("n", "str"):
                    value = raw
                else:
                    raise RuleError(f"XLSX unsupported cell type: {kind}")
                if (row, col) in values:
                    raise RuleError("XLSX duplicate cell")
                if col > width and value not in (None, ""):
                    raise RuleError(f"{name}: unknown populated column {col}")
                if col <= width:
                    values[row, col] = value
            last = max((r for r, c in values), default=1)
            tables[name] = [[values.get((r, c), "") for c in range(1, width + 1)] for r in range(1, last + 1)]
        if set(tables) != {"Actions", "Rules"}:
            raise RuleError("XLSX requires Actions and Rules sheets")
        return tables


def _lua_tokens(text):
    """Lexical declaration precheck only. Strings/comments cannot invent exports."""
    tokens, index = [], 0
    while index < len(text):
        char = text[index]
        if char.isspace():
            index += 1
            continue
        if text.startswith("--", index):
            match = re.match(r"--\[(=*)\[", text[index:])
            if match:
                closing = "]" + match[1] + "]"
                end = text.find(closing, index + len(match[0]))
                if end < 0:
                    raise RuleError("Lua precheck: unterminated long comment")
                index = end + len(closing)
            else:
                end = text.find("\n", index)
                index = len(text) if end < 0 else end + 1
            continue
        match = re.match(r"\[(=*)\[", text[index:])
        if match:
            closing = "]" + match[1] + "]"
            end = text.find(closing, index + len(match[0]))
            if end < 0:
                raise RuleError("Lua precheck: unterminated long string")
            tokens.append("<string>")
            index = end + len(closing)
            continue
        if char in "\"'":
            quote = char
            index += 1
            while index < len(text) and text[index] != quote:
                index += 2 if text[index] == "\\" else 1
            if index >= len(text):
                raise RuleError("Lua precheck: unterminated string")
            index += 1
            tokens.append("<string>")
            continue
        match = re.match(r"[A-Za-z_][A-Za-z0-9_]*", text[index:])
        token = match[0] if match else char
        tokens.append(token)
        index += len(token)
    return tokens


def _lua_declared_functions(text):
    tokens = _lua_tokens(text)
    if tokens[:5] != ["return", "{", "functions", "=", "{"]:
        raise RuleError("Lua authoring check requires literal return { functions = { name = function(ctx) ... end } }")
    declared, braces, blocks = set(), 2, 0
    for index in range(5, len(tokens)):
        token = tokens[index]
        if braces == 2 and blocks == 0 and FUNCTION.fullmatch(token) and tokens[index + 1:index + 3] == ["=", "function"]:
            if token in declared:
                raise RuleError(f"Duplicate Lua condition declaration: {token}")
            declared.add(token)
        if token in ("function", "if", "do", "repeat"):
            blocks += 1
        elif token in ("end", "until"):
            blocks -= 1
        elif token == "{":
            braces += 1
        elif token == "}":
            braces -= 1
            if braces == 1:
                if blocks != 0:
                    raise RuleError("Lua precheck: unbalanced function body")
                break
        if blocks < 0:
            raise RuleError("Lua precheck: unmatched end")
    if braces != 1:
        raise RuleError("Lua precheck: missing functions table end")
    return declared


def validate_lua_references(document, output_dir):
    """Conservative authoring check only; the game VM validates actual exports before mount."""
    if "luaFile" not in document:
        return
    source = Path(output_dir) / document["luaFile"]
    if not source.is_file():
        raise RuleError(f"Lua sibling missing: {source}. No placeholder functions are generated.")
    if source.stat().st_size > MAX_LUA_SOURCE:
        raise RuleError("Lua source exceeds runtime limit of 64 KiB UTF-8")
    text = _read(source)
    # The authoring convention uses literal named functions in a returned functions table.
    # This is deliberately NOT a Lua parser/compiler and never executes Lua.
    declared = _lua_declared_functions(text)
    required = {rule["condition"] for rule in document["rules"] if rule["condition"]}
    if required - declared:
        raise RuleError(f"Lua condition declarations missing: {sorted(required - declared)}")


def write_document(document, output, *, force=False, registered_tags=None):
    clean = validate_document(document, registered_tags)
    output = Path(output)
    if output.suffix.lower() != ".json":
        raise RuleError("Output must be a .json file, not an authoring script or workbook")
    validate_lua_references(clean, output.parent)
    if output.exists() and not force:
        raise RuleError(f"Output already exists: {output}. Review it; use --force to replace explicitly.")
    payload = json.dumps(clean, ensure_ascii=False, indent=2, allow_nan=False) + "\n"
    if len(payload.encode("utf-8")) > MAX_RUNTIME_SOURCE:
        raise RuleError("Output JSON exceeds runtime limit of 1 MiB; split the profile")
    output.parent.mkdir(parents=True, exist_ok=True)
    handle, temporary = tempfile.mkstemp(prefix=".action-rules-", suffix=".tmp", dir=output.parent)
    try:
        with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        if force:
            os.replace(temporary, output)
        else:
            os.link(temporary, output)  # Atomic no-overwrite; never silently replaces another author's file.
    except FileExistsError:
        raise RuleError(f"Output appeared during conversion: {output}; not replaced") from None
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    return clean


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, help=".xlsx, .md or .json")
    parser.add_argument("--actions-csv", type=Path)
    parser.add_argument("--rules-csv", type=Path)
    parser.add_argument("--id", help="Stable rule-set ID (required for tables)")
    parser.add_argument("--lua-file", help="Existing .lua basename beside output JSON")
    parser.add_argument("--tags-file", type=Path, help="Registered tags: newline text, JSON array or UE tag INI")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--force", action="store_true", help="Explicitly replace an existing output after successful validation")
    args = parser.parse_args(argv)
    try:
        inputs = [path for path in (args.input, args.actions_csv, args.rules_csv, args.tags_file) if path]
        if any(path.resolve() == args.output.resolve() for path in inputs):
            raise RuleError("Output must not overwrite an authoring input, even with --force")
        tags = read_registered_tags(args.tags_file) if args.tags_file else None
        if bool(args.input) == bool(args.actions_csv or args.rules_csv):
            raise RuleError("Choose --input OR both --actions-csv and --rules-csv")
        if args.input and args.input.suffix.lower() == ".json":
            if args.id or args.lua_file:
                raise RuleError("JSON already owns id/luaFile; do not override them with CLI flags")
            document = validate_document(read_json(args.input), tags)
        else:
            if not args.id:
                raise RuleError("Table conversion requires --id")
            if args.input:
                readers = {".xlsx": read_xlsx, ".md": read_markdown}
                if args.input.suffix.lower() not in readers:
                    raise RuleError("Unsupported input; use .xlsx, .md, .json or paired CSV")
                tables = readers[args.input.suffix.lower()](args.input)
            else:
                if not args.actions_csv or not args.rules_csv:
                    raise RuleError("Both CSV files are required")
                tables = {"Actions": read_csv(args.actions_csv), "Rules": read_csv(args.rules_csv)}
            document = tables_document(tables, args.id, args.lua_file, tags)
        document = write_document(document, args.output, force=args.force, registered_tags=tags)
        print(f"Wrote {args.output}: {len(document['actions'])} actions, {len(document['rules'])} rules. Not mounted or published.")
        if tags is None:
            print("Tag registration not checked offline; use --tags-file. Runtime validation remains mandatory.")
        if document.get("luaFile"):
            print("Lua declarations checked only; syntax, exports and execution are validated by the runtime VM at mount.")
        return 0
    except (RuleError, OSError, ValueError, KeyError, zipfile.BadZipFile, ET.ParseError, csv.Error) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
