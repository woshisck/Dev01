"""
Fill the old-way explicit player action AbilityData assets.

This edits binary Unreal assets through the editor Python API:
  - DA_WeaponAttack: attack + dash rows
  - DA_WeaponSkill: weapon skill rows
  - DA_Special: special rows

It also wires the three typed AbilityData fields on the test weapon definition so
equipping that weapon merges all three buckets into the runtime CharacterData.
"""

from pathlib import Path

import unreal


ABILITY_DATA_DIR = "/Game/Docs/Data/Character/ExplicitActions"
WEAPON_DEFINITION_PATH = "/Game/Developers/sunchuankai/LootTest/DA_WPN_THSword_Test"

DASH_MONTAGE = "/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01"

ABILITY_DATA_SPECS = {
    "DA_WeaponAttack": {
        "expected_class": "WeaponAttackAbilityMontageData",
        "entries": {
            "PlayerState.AbilityCast.Attack.Combo1": "/Game/Animation/1H-2HSword/Montage/1H_Attack_01_Seq_Montage",
            "PlayerState.AbilityCast.Attack.Combo2": "/Game/Animation/1H-2HSword/Montage/1H_Attack_02_Seq_Montage",
            "PlayerState.AbilityCast.Attack.Combo3": "/Game/Animation/1H-2HSword/Montage/1H_Attack_03_Seq_Montage",
            "PlayerState.AbilityCast.Attack.Combo4": "/Game/Animation/1H-2HSword/Montage/1H_Attack_04_Seq_Montage",
            "PlayerState.AbilityCast.Dash.Combo1": DASH_MONTAGE,
            "PlayerState.AbilityCast.Dash.Combo2": DASH_MONTAGE,
            "PlayerState.AbilityCast.Dash.Combo3": DASH_MONTAGE,
            "PlayerState.AbilityCast.Dash.Combo4": DASH_MONTAGE,
            "PlayerState.AbilityCast.Dash": DASH_MONTAGE,
            "PlayerState.AbilityCast.Dash.Dash1": DASH_MONTAGE,
            "PlayerState.AbilityCast.Dash.DashATK1": DASH_MONTAGE,
            "PlayerState.AbilityCast.DashAtk": DASH_MONTAGE,
        },
    },
    "DA_WeaponSkill": {
        "expected_class": "WeaponSkillAbilityMontageData",
        "entries": {
            "PlayerState.AbilityCast.WeaponSkill.Combo1": "/Game/Animation/1H-2HSword/Montage/1H_Attack_05_Seq_Montage",
            "PlayerState.AbilityCast.WeaponSkill.Combo2": "/Game/Animation/1H-2HSword/Montage/1H_Attack_06_Seq_Montage",
            "PlayerState.AbilityCast.WeaponSkill.Combo3": "/Game/Animation/1H-2HSword/Montage/1H_Attack_07_Seq_Montage",
            "PlayerState.AbilityCast.WeaponSkill.Combo4": "/Game/Animation/1H-2HSword/Montage/1H_Attack_08_Seq_Montage",
        },
    },
    "DA_Special": {
        "expected_class": "SpecialAbilityMontageData",
        "entries": {
            "PlayerState.AbilityCast.Special.Combo1": "/Game/Animation/1H-2HSword/Montage/1H_Attack_09_Seq_Montage",
            "PlayerState.AbilityCast.Special.Combo2": "/Game/Animation/1H-2HSword/Montage/1H_Attack_10_Seq_Montage",
            "PlayerState.AbilityCast.Special.Combo3": "/Game/Animation/1H-2HSword/Montage/1H_Attack_11_Seq_Montage",
            "PlayerState.AbilityCast.Special.Combo4": "/Game/Animation/1H-2HSword/Montage/1H_Attack_12_Seq_Montage",
        },
    },
}

WEAPON_FIELDS = {
    "attack_ability_data": "DA_WeaponAttack",
    "weapon_skill_ability_data": "DA_WeaponSkill",
    "special_ability_data": "DA_Special",
}


def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Failed to load asset: {path}")
    return asset


def gameplay_tag_to_string(tag):
    return str(tag.get_editor_property("tag_name"))


def make_gameplay_tag(tag_name):
    try:
        return unreal.GameplayTag(tag_name=unreal.Name(tag_name))
    except Exception:
        return unreal.GameplayTag(tag_name=tag_name)


def object_path(asset):
    return asset.get_path_name() if asset else "None"


def ensure_montage_map(asset, entries):
    montage_map = dict(asset.get_editor_property("montage_map"))
    existing_tags_by_name = {
        gameplay_tag_to_string(tag): tag
        for tag in montage_map.keys()
    }
    added_keys = 0
    assigned_montages = 0

    for tag_name, montage_path in entries.items():
        montage = load_asset(montage_path)
        tag = existing_tags_by_name.get(tag_name)
        if not tag:
            tag = make_gameplay_tag(tag_name)

        if tag not in montage_map:
            added_keys += 1

        if montage_map.get(tag) != montage:
            assigned_montages += 1
            montage_map[tag] = montage

    asset.set_editor_property("montage_map", montage_map)
    return added_keys, assigned_montages


def save_assets(assets):
    failed = []
    for asset in assets:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
            failed.append(object_path(asset))
    if failed:
        raise RuntimeError(f"Failed to save assets: {failed}")


def write_report(lines):
    project_dir = Path(unreal.Paths.project_dir())
    report_path = project_dir / "Docs" / "GeneratedReports" / "CommandletReports" / "PlayerExplicitAbilityDataSetupReport.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return report_path


def main():
    report = ["# Player Explicit AbilityData Setup", ""]
    ability_assets = {}
    dirty_assets = []

    for asset_name, spec in ABILITY_DATA_SPECS.items():
        asset_path = f"{ABILITY_DATA_DIR}/{asset_name}"
        asset = load_asset(asset_path)
        ability_assets[asset_name] = asset

        actual_class = asset.get_class().get_name()
        expected_class = spec["expected_class"]
        if actual_class != expected_class:
            raise RuntimeError(f"{asset_path} class is {actual_class}, expected {expected_class}")

        added_keys, assigned_montages = ensure_montage_map(asset, spec["entries"])
        dirty_assets.append(asset)
        report.append(
            f"- `{asset_path}`: class `{actual_class}`, added keys `{added_keys}`, assigned montages `{assigned_montages}`"
        )

    weapon = load_asset(WEAPON_DEFINITION_PATH)
    dirty_assets.append(weapon)
    report.append("")
    report.append(f"- `{WEAPON_DEFINITION_PATH}` typed AbilityData:")
    for property_name, asset_name in WEAPON_FIELDS.items():
        ability_data = ability_assets[asset_name]
        weapon.set_editor_property(property_name, ability_data)
        report.append(f"  - `{property_name}` -> `{object_path(ability_data)}`")

    save_assets(dirty_assets)
    report_path = write_report(report)
    unreal.log(f"Player explicit AbilityData setup complete. Report: {report_path}")


if __name__ == "__main__":
    main()
