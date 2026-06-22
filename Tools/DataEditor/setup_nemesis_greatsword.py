"""
Configure Nemesis to use the enemy greatsword weapon data.

This edits Unreal assets through the editor Python API:
  - DA_Nemesis points at DA_Nemesis_GreatSword and DA_Nemesis_GAS.
  - DA_Nemesis_GreatSword points at the enemy AbilityData and owns the AI attack profile.
  - DA_GreatSword_Ability gets Enemy.Melee.LAtk1-3 and Enemy.Skill.Skill1-3 montage rows.
  - DA_Nemesis_GAS grants the matching native enemy GA classes.
"""

from pathlib import Path

import unreal


ENEMY_DATA_PATH = "/Game/Docs/Data/Enemy/Rat/DA_Nemesis"
WEAPON_DEFINITION_PATH = "/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword"
ABILITY_DATA_PATH = "/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword_Ability"
GAS_TEMPLATE_PATH = "/Game/Docs/Data/Enemy/DA_Nemesis_GAS"

SCAN_PATHS = [
    "/Game/Docs/Data/Enemy",
    "/Game/Docs/Data/Enemy/Rat",
    "/Game/Code/Enemy/Weapon",
    "/Game/Code/Weapon/TwoHandedSword",
]

MONTAGE_ROWS = {
    "Enemy.Melee.LAtk1": "/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_01_L1_Combo",
    "Enemy.Melee.LAtk2": "/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_02_L2_Combo",
    "Enemy.Melee.LAtk3": "/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_03_L3_Finisher",
    "Enemy.Skill.Skill1": "/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_01_H1_Combo",
    "Enemy.Skill.Skill2": "/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_01_L2H_Finisher",
    "Enemy.Skill.Skill3": "/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_02_H2_Finisher",
}

ABILITY_CLASS_PATHS = [
    "/Script/DevKit.GA_Enemy_LAtk1",
    "/Script/DevKit.GA_Enemy_LAtk2",
    "/Script/DevKit.GA_Enemy_LAtk3",
    "/Script/DevKit.GA_Enemy_Skill1",
    "/Script/DevKit.GA_Enemy_Skill2",
    "/Script/DevKit.GA_Enemy_Skill3",
]

REPLACED_ABILITY_CLASS_NAMES = {
    "GA_Enemy_LAtk1",
    "GA_Enemy_LAtk2",
    "GA_Enemy_LAtk3",
    "GA_Enemy_LAtk4",
    "GA_Enemy_HAtk1",
    "GA_Enemy_HAtk2",
    "GA_Enemy_HAtk3",
    "GA_Enemy_HAtk4",
    "GA_Enemy_Skill1",
    "GA_Enemy_Skill2",
    "GA_Enemy_Skill3",
    "GA_Enemy_Skill4",
    "GA_WeaponSkill_Combo1",
    "GA_WeaponSkill_Combo2",
    "GA_WeaponSkill_Combo3",
    "GA_WeaponSkill_Combo4",
}


def object_path(obj):
    return obj.get_path_name() if obj else "None"


def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        object_name = path.rsplit("/", 1)[-1]
        asset = unreal.load_object(None, f"{path}.{object_name}")
    if not asset:
        raise RuntimeError(f"Failed to load asset: {path}")
    return asset


def load_or_create_enemy_ability_data(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = load_asset(path)
        if asset.get_class().get_name() != "EnemyAbilityMontageData":
            raise RuntimeError(
                f"{path} exists but class is {asset.get_class().get_name()}, expected EnemyAbilityMontageData"
            )
        return asset, False

    package_path, asset_name = path.rsplit("/", 1)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    asset_class = unreal.EnemyAbilityMontageData.static_class()
    factory.set_editor_property("data_asset_class", asset_class)
    asset = asset_tools.create_asset(asset_name, package_path, asset_class, factory)
    if not asset:
        raise RuntimeError(f"Failed to create enemy ability data asset: {path}")
    return asset, True


def load_class(path):
    cls = unreal.load_class(None, path)
    if not cls:
        raise RuntimeError(f"Failed to load class: {path}")
    return cls


def gameplay_tag_to_string(tag):
    return str(tag.get_editor_property("tag_name"))


def make_tag_container(tag):
    return unreal.GameplayTagLibrary.make_gameplay_tag_container_from_tag(tag)


def enum_value(enum_class, names, fallback):
    for name in names:
        if hasattr(enum_class, name):
            return getattr(enum_class, name)
    return fallback


def class_name(cls):
    return cls.get_name() if cls else "None"


def gameplay_tag_container_to_string(container):
    try:
        return unreal.GameplayTagLibrary.get_debug_string_from_gameplay_tag_container(container)
    except Exception:
        return str(container)


def ensure_montage_map(ability_data, rows):
    montage_map = dict(ability_data.get_editor_property("montage_map"))
    existing_tags_by_name = {
        gameplay_tag_to_string(tag): tag
        for tag in montage_map.keys()
    }
    changes = []

    for tag_name, montage_path in rows.items():
        montage = load_asset(montage_path)
        tag = existing_tags_by_name.get(tag_name)
        if not tag:
            raise RuntimeError(
                f"{object_path(ability_data)} is missing default MontageMap key {tag_name}. "
                "Create/resave it as Enemy Ability Montage Data so PostInitProperties adds enemy keys."
            )
        previous = montage_map.get(tag)
        if previous != montage:
            montage_map[tag] = montage
            changes.append((tag_name, object_path(previous), object_path(montage)))

    ability_data.set_editor_property("montage_map", montage_map)
    return changes, existing_tags_by_name


def make_attack_option(tag_name, tag, role, min_range, max_range, weight, cooldown):
    option = unreal.EnemyAIAttackOption()
    option.set_editor_property("attack_name", unreal.Name(tag_name.replace(".", "_")))
    option.set_editor_property("ability_tags", make_tag_container(tag))
    option.set_editor_property("min_range", min_range)
    option.set_editor_property("max_range", max_range)
    option.set_editor_property("weight", weight)
    option.set_editor_property("cooldown", cooldown)
    option.set_editor_property("pre_attack_flash", True)
    option.set_editor_property("attack_role", role)
    return option


def build_attack_profile(existing_profile, tags_by_name):
    role_enum = unreal.EnemyAIAttackRole
    close_melee_role = enum_value(role_enum, ["CLOSE_MELEE", "CloseMelee"], 0)
    skill_role = enum_value(role_enum, ["SKILL", "Skill"], 2)

    attacks = [
        make_attack_option("Enemy.Melee.LAtk1", tags_by_name["Enemy.Melee.LAtk1"], close_melee_role, 0.0, 280.0, 3.0, 1.0),
        make_attack_option("Enemy.Melee.LAtk2", tags_by_name["Enemy.Melee.LAtk2"], close_melee_role, 0.0, 300.0, 2.5, 1.15),
        make_attack_option("Enemy.Melee.LAtk3", tags_by_name["Enemy.Melee.LAtk3"], close_melee_role, 0.0, 320.0, 2.0, 1.3),
        make_attack_option("Enemy.Skill.Skill1", tags_by_name["Enemy.Skill.Skill1"], skill_role, 0.0, 420.0, 1.2, 7.0),
        make_attack_option("Enemy.Skill.Skill2", tags_by_name["Enemy.Skill.Skill2"], skill_role, 0.0, 480.0, 1.0, 10.0),
        make_attack_option("Enemy.Skill.Skill3", tags_by_name["Enemy.Skill.Skill3"], skill_role, 0.0, 540.0, 0.8, 13.0),
    ]

    existing_profile.set_editor_property("attacks", attacks)
    existing_profile.set_editor_property("recent_attack_memory_duration", 2.5)
    existing_profile.set_editor_property("repeat_attack_weight_multiplier", 0.35)
    return existing_profile


def ensure_gas_template(gas_template):
    existing_abilities = list(gas_template.get_editor_property("ability_map"))
    desired_abilities = [load_class(path) for path in ABILITY_CLASS_PATHS]
    desired_names = {class_name(cls) for cls in desired_abilities}

    filtered_abilities = [
        cls
        for cls in existing_abilities
        if class_name(cls) not in REPLACED_ABILITY_CLASS_NAMES
        and class_name(cls) not in desired_names
    ]
    new_abilities = filtered_abilities + desired_abilities

    if [object_path(cls) for cls in existing_abilities] != [object_path(cls) for cls in new_abilities]:
        gas_template.set_editor_property("ability_map", new_abilities)
        return True, [class_name(cls) for cls in existing_abilities], [class_name(cls) for cls in new_abilities]

    return False, [class_name(cls) for cls in existing_abilities], [class_name(cls) for cls in new_abilities]


def ensure_array_contains(asset, property_name, item):
    values = list(asset.get_editor_property(property_name))
    if item not in values:
        values.append(item)
        asset.set_editor_property(property_name, values)
        return True
    return False


def save_assets(assets):
    failed = []
    for asset in assets:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
            failed.append(object_path(asset))
    if failed:
        raise RuntimeError(f"Failed to save assets: {failed}")


def attack_profile_lines(profile):
    lines = []
    for option in profile.get_editor_property("attacks"):
        tags = option.get_editor_property("ability_tags")
        role = option.get_editor_property("attack_role")
        lines.append(
            "- `{}` role `{}` range `{:.0f}-{:.0f}` weight `{:.2f}` cooldown `{:.2f}`".format(
                gameplay_tag_container_to_string(tags),
                role,
                option.get_editor_property("min_range"),
                option.get_editor_property("max_range"),
                option.get_editor_property("weight"),
                option.get_editor_property("cooldown"),
            )
        )
    return lines


def write_report(lines):
    project_dir = Path(unreal.Paths.project_dir())
    report_path = project_dir / "Docs" / "GeneratedReports" / "CommandletReports" / "NemesisGreatSwordSetupReport.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return report_path


def main():
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_registry.scan_paths_synchronous(SCAN_PATHS, True)

    enemy_data = load_asset(ENEMY_DATA_PATH)
    weapon_definition = load_asset(WEAPON_DEFINITION_PATH)
    ability_data, created_ability_data = load_or_create_enemy_ability_data(ABILITY_DATA_PATH)
    gas_template = load_asset(GAS_TEMPLATE_PATH)

    report = [
        "# Nemesis GreatSword Setup",
        "",
        "## Assets",
        f"- EnemyData: `{object_path(enemy_data)}`",
        f"- WeaponDefinition: `{object_path(weapon_definition)}`",
        f"- AbilityData: `{object_path(ability_data)}` (created `{created_ability_data}`)",
        f"- GASTemplate: `{object_path(gas_template)}`",
        "",
    ]

    montage_changes, tags_by_name = ensure_montage_map(ability_data, MONTAGE_ROWS)

    weapon_definition.set_editor_property("ability_data", ability_data)
    weapon_definition.set_editor_property("override_attack_profile", True)
    attack_profile = build_attack_profile(weapon_definition.get_editor_property("attack_profile"), tags_by_name)
    weapon_definition.set_editor_property("attack_profile", attack_profile)

    enemy_data.set_editor_property("default_weapon_definition", weapon_definition)
    ensure_array_contains(enemy_data, "allowed_weapon_definitions", weapon_definition)
    enemy_data.set_editor_property("gas_template", gas_template)

    gas_changed, previous_abilities, new_abilities = ensure_gas_template(gas_template)

    save_assets([ability_data, weapon_definition, enemy_data, gas_template])

    report.extend([
        "## Montage Rows",
    ])
    for tag_name, montage_path in MONTAGE_ROWS.items():
        report.append(f"- `{tag_name}` -> `{montage_path}`")

    if montage_changes:
        report.append("")
        report.append("## Montage Changes")
        for tag_name, previous, current in montage_changes:
            report.append(f"- `{tag_name}`: `{previous}` -> `{current}`")

    report.extend([
        "",
        "## Weapon Attack Profile",
    ])
    report.extend(attack_profile_lines(weapon_definition.get_editor_property("attack_profile")))

    report.extend([
        "",
        "## GAS AbilityMap",
        f"- Changed: `{gas_changed}`",
        f"- Previous: `{', '.join(previous_abilities)}`",
        f"- Current: `{', '.join(new_abilities)}`",
        "",
        "## Enemy Data",
        f"- `default_weapon_definition` -> `{object_path(enemy_data.get_editor_property('default_weapon_definition'))}`",
        f"- `gas_template` -> `{object_path(enemy_data.get_editor_property('gas_template'))}`",
    ])

    report_path = write_report(report)
    unreal.log(f"Nemesis greatsword setup complete. Report: {report_path}")


if __name__ == "__main__":
    main()
