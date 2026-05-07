import json
import os

import unreal


BROKEN_PACKAGES = [
    "/Game/Maps/B_TeleportGate",
    "/Game/Code/Environment/EnvModule/Data/Comp_ISM",
    "/Game/Code/GAS/Target/B_TT_Enemy",
    "/Game/Code/GAS/Abilities/DA/GA_WeaponAbility",
    "/Game/Code/GAS/Abilities/Template/GA_MobAbility",
    "/Game/Code/GAS/Abilities/Passive/GA_PlayHurtMontage",
    "/Game/Code/GAS/Abilities/Passive/GA_StackCoolDown",
    "/Game/Code/Animation/Anime_Notify/AN_ResetDmg",
    "/Game/Code/Animation/Anime_Notify/AN_PostAtkCheck",
    "/Game/Code/Animation/Anime_Notify/ANS_Rotate",
    "/Game/Code/Animation/Anime_Notify/ANS_DmgWindow",
    "/Game/CharacterBodyFX3/Demo/ThirdPersonBP/Blueprints/ThirdPersonCharacter",
    "/Market/OtherByMarket/EssentialGreatSwordAnimationPack/Blueprint/BP_ThirdPersonCharacter",
    "/Game/Animation/1H-2HSword/AnimBP_EssentialGreatSword",
]


def package_to_filename(package_name):
    if package_name.startswith("/Game/"):
        relative_path = package_name[len("/Game/"):] + ".uasset"
        return os.path.join(unreal.Paths.project_content_dir(), relative_path)
    if package_name.startswith("/Market/"):
        relative_path = package_name[len("/Market/"):] + ".uasset"
        return os.path.join(unreal.Paths.project_plugins_dir(), "Market", "Content", relative_path)
    return ""


def sort_names(values):
    if not values:
        return []
    return sorted(str(value) for value in values)


def get_direct_referencers(asset_registry, package_name):
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_searchable_names=True,
        include_soft_management_references=True,
        include_hard_management_references=True,
    )
    return sort_names(asset_registry.get_referencers(package_name, options))


def get_recursive_referencers(asset_registry, root_package, max_depth=4):
    visited = {root_package}
    frontier = [root_package]
    by_depth = {}

    for depth in range(1, max_depth + 1):
        next_frontier = []
        for package_name in frontier:
            for referencer in get_direct_referencers(asset_registry, package_name):
                if referencer in visited:
                    continue
                visited.add(referencer)
                next_frontier.append(referencer)
                by_depth.setdefault(depth, []).append(referencer)
        frontier = next_frontier
        if not frontier:
            break

    return {str(depth): sort_names(packages) for depth, packages in by_depth.items()}


def classify(package_name, direct_referencers, recursive_referencers):
    if package_name.startswith("/Market/") or "/Demo/" in package_name or "/ThirdPerson_Contents/" in package_name:
        if not direct_referencers:
            return "DELETE_CANDIDATE_DEMO_UNUSED"
        return "CHECK_DEMO_REFERENCERS"

    if package_name.startswith("/Game/CharacterBodyFX3/Demo/"):
        if not direct_referencers:
            return "DELETE_CANDIDATE_DEMO_UNUSED"
        return "CHECK_DEMO_REFERENCERS"

    project_refs = [
        ref for ref in direct_referencers
        if ref.startswith("/Game/")
        and "/Demo/" not in ref
        and "/ThirdPerson_Contents/" not in ref
    ]
    project_recursive_refs = [
        ref for refs in recursive_referencers.values()
        for ref in refs
        if ref.startswith("/Game/")
        and "/Demo/" not in ref
        and "/ThirdPerson_Contents/" not in ref
    ]

    if project_refs:
        return "KEEP_OR_MIGRATE_REFERENCED"
    if project_recursive_refs:
        return "REVIEW_TRANSITIVE_PROJECT_REFERENCES"
    return "REVIEW_UNREFERENCED_PROJECT_ASSET"


def main():
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_registry.search_all_assets(True)

    output = []
    for package_name in BROKEN_PACKAGES:
        asset_data = asset_registry.get_asset_by_object_path(f"{package_name}.{os.path.basename(package_name)}")
        direct_referencers = get_direct_referencers(asset_registry, package_name)
        recursive_referencers = get_recursive_referencers(asset_registry, package_name)
        filename = package_to_filename(package_name)
        exists_on_disk = bool(filename and os.path.exists(filename))
        output.append({
            "package": package_name,
            "exists_on_disk": exists_on_disk,
            "asset_class": str(asset_data.asset_class_path.asset_name) if asset_data and asset_data.is_valid() else "",
            "direct_referencer_count": len(direct_referencers),
            "direct_referencers": direct_referencers,
            "recursive_referencers": recursive_referencers,
            "classification": classify(package_name, direct_referencers, recursive_referencers),
        })

    report_dir = os.path.join(unreal.Paths.project_saved_dir(), "Balance")
    os.makedirs(report_dir, exist_ok=True)
    report_path = os.path.join(report_dir, "BrokenBlueprintReferenceAudit.json")
    with open(report_path, "w", encoding="utf-8") as fp:
        json.dump(output, fp, ensure_ascii=False, indent=2)

    unreal.log(f"Broken blueprint reference audit written to: {report_path}")
    for item in output:
        unreal.log(
            "AUDIT {classification} refs={refs} class={asset_class} package={package}".format(
                classification=item["classification"],
                refs=item["direct_referencer_count"],
                asset_class=item["asset_class"],
                package=item["package"],
            )
        )


main()
