"""
Resave assets that used to carry player-runtime ComboGraph references.

After the C++ compatibility fields are marked Transient, this script resaves
known migrated data assets and selected player Blueprints so stale serialized
references can be dropped by the editor safely.
"""

from pathlib import Path
import argparse

import unreal


WEAPON_DEFINITION_ASSETS = [
    "/Game/Code/Weapon/UnArmed/DA_WPN_UnArm",
    "/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword",
    "/Game/Code/Weapon/TwoHandedSword/DA_Weapon_FirstRun_DemoSword",
    "/Game/Code/Weapon/GreatSword/DA_WPN_RedSword",
    "/Game/Developers/sunchuankai/ComboGraph/DA_BlackSword",
]

SPECIAL_ATTACK_ASSETS = []

PLAYER_BLUEPRINT_ASSETS = [
    "/Game/Code/Characters/B_PlayerOne",
    "/Game/Developers/sunchuankai/LootTest/DebugPlayer",
]

REFERENCE_SCAN_ROOTS = [
    "/Game/Code",
    "/Game/Docs",
    "/Game/Developers",
]

REFERENCE_NEEDLES = [
    "ComboRuntimeComponent",
    "GameplayAbilityComboGraph",
    "WeaponSkillComboGraph",
    "YogComboGraph",
]


def object_path(asset):
    return asset.get_path_name() if asset else "None"


def load_asset(path, report):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        report.append(f"- Missing asset `{path}`.")
    return asset


def mark_for_resave(asset, apply, reason, report):
    report.append(f"- {'Resaved' if apply else 'Would resave'} `{object_path(asset)}` ({reason}).")
    if apply:
        asset.modify()
    return True


def compile_blueprint(asset, apply, report):
    if not isinstance(asset, unreal.Blueprint):
        report.append(f"- `{object_path(asset)}` is not a Blueprint.")
        return False

    report.append(f"- {'Compiled/resaved' if apply else 'Would compile/resave'} Blueprint `{object_path(asset)}`.")
    if apply:
        unreal.KismetEditorUtilities.compile_blueprint(asset)
        asset.modify()
    return True


def save_assets(assets, report):
    failed = []
    saved = 0
    for asset in assets:
        if unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
            saved += 1
        else:
            failed.append(object_path(asset))

    report.append(f"- Saved dirty assets: `{saved}`.")
    if failed:
        raise RuntimeError(f"Failed to save assets: {failed}")


def scan_package_references(report):
    matches = []

    for root in REFERENCE_SCAN_ROOTS:
        asset_paths = unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False)
        for asset_path in asset_paths:
            package_name = asset_path.split(".")[0]
            if not package_name.startswith("/Game/"):
                continue

            relative_package = package_name[len("/Game/"):]
            content_dir = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
            uasset_path = content_dir / f"{relative_package}.uasset"
            umap_path = content_dir / f"{relative_package}.umap"
            disk_path = uasset_path if uasset_path.exists() else umap_path
            if not disk_path.exists():
                continue

            try:
                data = disk_path.read_bytes()
            except OSError:
                continue

            found = [needle for needle in REFERENCE_NEEDLES if needle.encode("utf-8") in data]
            if found:
                matches.append((asset_path, found))

    if matches:
        report.append("")
        report.append("## Remaining binary reference strings")
        for asset_path, found in matches:
            report.append(f"- `{asset_path}`: {', '.join(found)}")
        report.append(
            "- Notes: actual ComboGraph assets and intentionally archived developer assets may remain. "
            "Player Blueprints should be opened/resaved if `ComboRuntimeComponent` still appears here."
        )

    return matches


def write_report(report):
    project_dir = Path(unreal.Paths.project_dir())
    report_path = project_dir / "Docs" / "GeneratedReports" / "CommandletReports" / "ComboGraphReferenceCleanupReport.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(report) + "\n", encoding="utf-8")
    return report_path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args, _unknown = parser.parse_known_args()
    apply = args.apply and not args.dry_run

    report = [
        "# ComboGraph Reference Cleanup",
        "",
        f"- Mode: `{'apply' if apply else 'dry-run'}`",
        "",
        "## Migrated Data Assets",
    ]

    dirty_assets = []

    for asset_path in WEAPON_DEFINITION_ASSETS:
        asset = load_asset(asset_path, report)
        if not asset:
            continue
        if mark_for_resave(asset, apply, "drop transient legacy weapon ComboGraph fields after compile", report):
            dirty_assets.append(asset)

    for asset_path in SPECIAL_ATTACK_ASSETS:
        asset = load_asset(asset_path, report)
        if not asset:
            continue
        if mark_for_resave(asset, apply, "drop transient legacy special ComboGraph field after compile", report):
            dirty_assets.append(asset)

    report.append("")
    report.append("## Player Blueprints")
    for asset_path in PLAYER_BLUEPRINT_ASSETS:
        asset = load_asset(asset_path, report)
        if not asset:
            continue
        if compile_blueprint(asset, apply, report):
            dirty_assets.append(asset)

    if apply and dirty_assets:
        save_assets(dirty_assets, report)
    elif not dirty_assets:
        report.append("- No dirty assets detected.")

    scan_package_references(report)

    report_path = write_report(report)
    unreal.log(f"ComboGraph reference cleanup {'applied' if apply else 'dry-run'} complete. Report: {report_path}")


if __name__ == "__main__":
    main()
