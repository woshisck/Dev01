"""
RuneID -> RuneIdTag migration script (modifies assets, run with care).

The migration is two-phase by design — Editor must be restarted between phases
because GameplayTagsManager only loads .ini files at startup.

Usage:
    Phase 1 (write Config/Tags/RuneIDs.ini, no asset change):
        Tools/DataEditor/run_migration.ps1 -Phase prepare

    -- THEN MANUALLY RESTART THE EDITOR --

    Phase 2 (apply tags to RuneDA assets, saves modified packages):
        Tools/DataEditor/run_migration.ps1 -Phase apply

You can re-run either phase any number of times. Both are idempotent:
    prepare -> "no new tags to add (ini up to date)"
    apply   -> "applied 0" once everything is migrated
"""

import unreal
import argparse


def save_loaded_assets(label, assets):
    saved = 0
    failed = 0
    for asset in assets:
        if not asset:
            continue
        if unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
            saved += 1
        else:
            failed += 1
            unreal.log_error(f"{label}: failed to save {asset.get_path_name()}")
    unreal.log(f"{label}: saved {saved} dirty assets, failed {failed}")
    if failed:
        raise RuntimeError(f"{label}: failed to save {failed} assets")
    return saved


def phase_prepare():
    unreal.log("=" * 78)
    unreal.log("PHASE 1: PrepareRuneIdTagIni")
    unreal.log("=" * 78)
    added = unreal.DataEditorLibrary.prepare_rune_id_tag_ini()
    unreal.log(f"Tags added to .ini: {added}")
    if added > 0:
        unreal.log_warning(
            "Tags written. RESTART THE EDITOR before running Phase 2."
        )
    else:
        unreal.log("No new tags. .ini already up to date.")
    return added


def phase_apply():
    unreal.log("=" * 78)
    unreal.log("PHASE 2: ApplyRuneIdTagsAfterRestart")
    unreal.log("=" * 78)
    applied = unreal.DataEditorLibrary.apply_rune_id_tags_after_restart()
    unreal.log(f"DA assets updated: {applied}")
    if applied > 0:
        targets = unreal.DataEditorLibrary.get_all_rune_d_as()
        save_loaded_assets("ApplyRuneIdTagsAfterRestart", targets)
    return applied


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--phase", choices=["prepare", "apply"], required=True)
    # ExecutePythonScript passes the arguments after the script path.
    args, _unknown = parser.parse_known_args()

    if args.phase == "prepare":
        phase_prepare()
    elif args.phase == "apply":
        phase_apply()


if __name__ == "__main__":
    main()
