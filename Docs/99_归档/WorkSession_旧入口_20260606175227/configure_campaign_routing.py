import os
import unreal


GAME_MODE_BP = "/Game/Code/Core/GameMode/B_GameMode"
MAIN_CAMPAIGN_DA = "/Game/Docs/Map/DA_Campaign_MainRun"
FIRST_RUN_CAMPAIGN_DA = "/Game/Docs/Map/DA_Campaign_Tutorial"

REPORT = []


def note(text):
    REPORT.append(str(text))
    print(text)


def object_path(asset_path):
    asset_name = asset_path.rsplit("/", 1)[1]
    return "{0}.{1}".format(asset_path, asset_name)


def load_asset_or_raise(asset_path):
    asset = unreal.load_asset(object_path(asset_path))
    if not asset:
        asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError("Missing required asset: {0}".format(asset_path))
    return asset


def load_bp_class(asset_path):
    try:
        loaded_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        if loaded_class:
            return loaded_class
    except Exception:
        pass

    asset = load_asset_or_raise(asset_path)
    for property_name in ["generated_class", "GeneratedClass"]:
        try:
            loaded_class = asset.get_editor_property(property_name)
            if loaded_class:
                return loaded_class
        except Exception:
            pass
    raise RuntimeError("Missing generated class: {0}".format(asset_path))


def get_cdo(asset_path):
    return unreal.get_default_object(load_bp_class(asset_path))


def set_property_any(obj, names, value):
    for property_name in names:
        try:
            obj.set_editor_property(property_name, value)
            note("Set {0} = {1}".format(property_name, value.get_path_name() if value else "None"))
            return
        except Exception:
            pass
    raise RuntimeError("Failed to set property {0} on {1}".format(names, obj))


def get_property_any(obj, names):
    for property_name in names:
        try:
            return obj.get_editor_property(property_name)
        except Exception:
            pass
    raise RuntimeError("Failed to get property {0} on {1}".format(names, obj))


def asset_path(asset):
    return asset.get_path_name() if asset else ""


def main():
    game_mode_bp = load_asset_or_raise(GAME_MODE_BP)
    game_mode_cdo = get_cdo(GAME_MODE_BP)
    main_campaign = load_asset_or_raise(MAIN_CAMPAIGN_DA)
    first_run_campaign = load_asset_or_raise(FIRST_RUN_CAMPAIGN_DA)

    set_property_any(game_mode_cdo, ["campaign_data", "CampaignData"], main_campaign)
    set_property_any(
        game_mode_cdo,
        ["first_run_tutorial_campaign_data", "FirstRunTutorialCampaignData"],
        first_run_campaign,
    )

    try:
        game_mode_bp.modify()
        game_mode_bp.mark_package_dirty()
    except Exception:
        pass

    try:
        unreal.KismetEditorUtilities.compile_blueprint(game_mode_bp)
        note("Compiled: {0}".format(GAME_MODE_BP))
    except Exception as first_error:
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(game_mode_bp)
            note("Compiled: {0}".format(GAME_MODE_BP))
        except Exception:
            note("Compile skipped: {0}".format(first_error))

    if not unreal.EditorAssetLibrary.save_asset(GAME_MODE_BP, False):
        raise RuntimeError("Failed to save {0}".format(GAME_MODE_BP))
    note("Saved: {0}".format(GAME_MODE_BP))

    actual_main = get_property_any(game_mode_cdo, ["campaign_data", "CampaignData"])
    actual_first_run = get_property_any(
        game_mode_cdo,
        ["first_run_tutorial_campaign_data", "FirstRunTutorialCampaignData"],
    )

    if asset_path(actual_main) != asset_path(main_campaign):
        raise RuntimeError("CampaignData mismatch: {0}".format(asset_path(actual_main)))
    if asset_path(actual_first_run) != asset_path(first_run_campaign):
        raise RuntimeError("FirstRunTutorialCampaignData mismatch: {0}".format(asset_path(actual_first_run)))

    note("Verified CampaignData -> {0}".format(asset_path(actual_main)))
    note("Verified FirstRunTutorialCampaignData -> {0}".format(asset_path(actual_first_run)))

    report_path = os.path.join(unreal.Paths.project_saved_dir(), "CampaignRoutingSetupReport.txt")
    with open(report_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))
    note("Report: {0}".format(report_path))


main()
