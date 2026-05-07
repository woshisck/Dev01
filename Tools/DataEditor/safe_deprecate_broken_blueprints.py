import json
import os
from datetime import datetime

import unreal


DEPRECATION_BATCH = "BrokenBlueprints_20260507"

MOVE_CANDIDATES = [
    # Unreferenced project assets from the reference audit.
    "/Game/Code/GAS/Target/B_TT_Enemy",
    "/Game/Code/GAS/Abilities/Template/GA_MobAbility",
    "/Game/Code/GAS/Abilities/Passive/GA_StackCoolDown",
    "/Game/Code/Animation/Anime_Notify/AN_ResetDmg",
    "/Game/Code/Animation/Anime_Notify/AN_PostAtkCheck",
    "/Game/Code/Animation/Anime_Notify/ANS_Rotate",
    "/Game/Code/Animation/Anime_Notify/ANS_DmgWindow",
    "/Game/Animation/1H-2HSword/AnimBP_EssentialGreatSword",

    # Demo-only assets. Move the small local reference chains together.
    "/Game/CharacterBodyFX3/Demo/ThirdPersonBP/Blueprints/ThirdPersonCharacter",
    "/Game/CharacterBodyFX3/Demo/ThirdPersonBP/Blueprints/ThirdPersonGameMode",

    # Marketplace demo-only assets mounted by the Market plugin.
    "/Market/OtherByMarket/EssentialGreatSwordAnimationPack/Blueprint/BP_ThirdPersonCharacter",
    "/Market/OtherByMarket/EssentialGreatSwordAnimationPack/Blueprint/BP_Hitbox",
    "/Market/OtherByMarket/EssentialGreatSwordAnimationPack/Blueprint/ThirdPersonGameMode",
]


def destination_for(source_path):
    if source_path.startswith("/Game/"):
        return f"/Game/_Deprecated/{DEPRECATION_BATCH}/{source_path[len('/Game/'):]}"
    if source_path.startswith("/Market/"):
        return f"/Market/_Deprecated/{DEPRECATION_BATCH}/{source_path[len('/Market/'):]}"
    raise ValueError(f"Unsupported mount point: {source_path}")


def object_path(asset_path):
    asset_name = asset_path.rsplit("/", 1)[-1]
    return f"{asset_path}.{asset_name}"


def make_parent_directory(asset_path):
    parent_path = asset_path.rsplit("/", 1)[0]
    if not unreal.EditorAssetLibrary.does_directory_exist(parent_path):
        unreal.EditorAssetLibrary.make_directory(parent_path)


def main():
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_registry.search_all_assets(True)

    results = []
    for source in MOVE_CANDIDATES:
        destination = destination_for(source)
        source_exists = unreal.EditorAssetLibrary.does_asset_exist(source)
        destination_exists = unreal.EditorAssetLibrary.does_asset_exist(destination)
        source_data = asset_registry.get_asset_by_object_path(object_path(source))

        result = {
            "source": source,
            "destination": destination,
            "source_exists": bool(source_exists),
            "destination_exists": bool(destination_exists),
            "source_class": str(source_data.asset_class_path.asset_name) if source_data and source_data.is_valid() else "",
            "status": "pending",
            "message": "",
        }

        if not source_exists:
            result["status"] = "skipped"
            result["message"] = "source asset does not exist or was already moved"
            results.append(result)
            continue

        if destination_exists:
            result["status"] = "skipped"
            result["message"] = "destination already exists; not overwriting"
            results.append(result)
            continue

        make_parent_directory(destination)
        moved = unreal.EditorAssetLibrary.rename_asset(source, destination)
        if moved:
            unreal.EditorAssetLibrary.save_asset(destination, only_if_is_dirty=False)
            result["status"] = "moved"
            result["message"] = "moved to deprecated folder"
        else:
            result["status"] = "failed"
            result["message"] = "EditorAssetLibrary.rename_asset returned false"

        results.append(result)

    report_dir = os.path.join(unreal.Paths.project_saved_dir(), "Balance")
    os.makedirs(report_dir, exist_ok=True)
    report_path = os.path.join(report_dir, f"SafeDeprecateBrokenBlueprints_{DEPRECATION_BATCH}.json")
    with open(report_path, "w", encoding="utf-8") as fp:
        json.dump(
            {
                "generated": datetime.now().isoformat(),
                "batch": DEPRECATION_BATCH,
                "results": results,
            },
            fp,
            ensure_ascii=False,
            indent=2,
        )

    moved_count = len([item for item in results if item["status"] == "moved"])
    failed_count = len([item for item in results if item["status"] == "failed"])
    skipped_count = len([item for item in results if item["status"] == "skipped"])
    unreal.log(f"Safe deprecate report written to: {report_path}")
    unreal.log(f"Safe deprecate summary: moved={moved_count}, skipped={skipped_count}, failed={failed_count}")
    for item in results:
        unreal.log(f"DEPRECATE {item['status']}: {item['source']} -> {item['destination']} ({item['message']})")

    if failed_count:
        raise RuntimeError(f"Failed to move {failed_count} assets. See {report_path}")


main()
