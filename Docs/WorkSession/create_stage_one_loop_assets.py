import os
import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

HUD_BP = "/Game/UI/BP_YogHUD"
GAME_MODE_BP = "/Game/Code/Core/GameMode/B_GameMode"
RUN_SUMMARY_WBP = "/Game/UI/MetaProgression/WBP_RunSummary"
TREE_WBP = "/Game/UI/MetaProgression/WBP_MetaUpgradeTree"
WBP_CARD = "/Game/UI/MetaProgression/WBP_MetaNodeCard"
TERMINAL_BP = "/Game/World/Hub/BP_HubUpgradeTerminal"
HUB_MAP = "/Game/World/Hub/L_HubTown"
CAMPAIGN_DA = "/Game/Docs/Map/DA_Campaign_Tutorial"
STORY_REGISTRY_DA = "/Game/Data/Story/DA_StoryEventRegistry_Tutorial"

ROOM_INITIAL = "/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom"
ROOM_PRISON_NORMAL = "/Game/Docs/Map/DA_Room_Prison_Normal"
ROOM_CORRIDOR_A = "/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a"
ROOM_CORRIDOR_B = "/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b"
ROOM_WATER_DUNGEON = "/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon"

REPORT = []


def note(text):
    REPORT.append(str(text))
    print(text)


def split_asset_path(asset_path):
    package_path, asset_name = asset_path.rsplit("/", 1)
    return package_path, asset_name


def object_path(asset_path):
    _, asset_name = split_asset_path(asset_path)
    return "{0}.{1}".format(asset_path, asset_name)


def ensure_dir_for_asset(asset_path):
    package_path, _ = split_asset_path(asset_path)
    if not unreal.EditorAssetLibrary.does_directory_exist(package_path):
        unreal.EditorAssetLibrary.make_directory(package_path)
        note("Created directory: {0}".format(package_path))
    return package_path


def load_asset(asset_path):
    asset = unreal.load_asset(object_path(asset_path))
    if not asset:
        asset = unreal.load_asset(asset_path)
    return asset


def load_asset_or_raise(asset_path):
    asset = load_asset(asset_path)
    if not asset:
        raise RuntimeError("Missing required asset: {0}".format(asset_path))
    return asset


def load_class_or_raise(label, class_paths):
    for class_path in class_paths:
        loaded = unreal.load_class(None, class_path)
        if loaded:
            note("Loaded class for {0}: {1}".format(label, class_path))
            return loaded
    raise RuntimeError("Failed to load class for {0}: {1}".format(label, ", ".join(class_paths)))


def load_bp_class(asset_path):
    loaded_class = None
    try:
        loaded_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
    except Exception:
        loaded_class = None
    if loaded_class:
        return loaded_class

    asset = load_asset(asset_path)
    if asset:
        for property_name in ["generated_class", "GeneratedClass"]:
            try:
                loaded_class = asset.get_editor_property(property_name)
                if loaded_class:
                    return loaded_class
            except Exception:
                pass
    raise RuntimeError("Failed to load generated Blueprint class: {0}".format(asset_path))


def get_cdo(asset_path):
    return unreal.get_default_object(load_bp_class(asset_path))


def set_property_any(obj, names, value, required=True):
    for property_name in names:
        try:
            obj.set_editor_property(property_name, value)
            return property_name
        except Exception:
            pass
    if required:
        raise RuntimeError("Failed to set any property {0} on {1}".format(names, obj))
    return None


def mark_dirty(obj):
    try:
        obj.modify()
    except Exception:
        pass
    try:
        obj.mark_package_dirty()
    except Exception:
        pass


def compile_blueprint(asset, asset_path):
    try:
        unreal.KismetEditorUtilities.compile_blueprint(asset)
        note("Compiled: {0}".format(asset_path))
    except Exception as first_error:
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(asset)
            note("Compiled: {0}".format(asset_path))
        except Exception:
            note("Compile skipped for {0}: {1}".format(asset_path, first_error))


def save_asset(asset_path):
    if unreal.EditorAssetLibrary.save_asset(asset_path, False):
        note("Saved: {0}".format(asset_path))
    else:
        note("Save returned False: {0}".format(asset_path))


def save_loaded_asset(asset, asset_path):
    saved = False
    try:
        saved = unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    except Exception as exc:
        note("save_loaded_asset failed for {0}: {1}".format(asset_path, exc))
    if not saved:
        save_asset(asset_path)
    else:
        note("Saved loaded asset: {0}".format(asset_path))


def create_blueprint(asset_path, parent_class):
    ensure_dir_for_asset(asset_path)
    existing = load_asset(asset_path)
    if existing:
        note("Blueprint already exists: {0}".format(asset_path))
        return existing

    package_path, asset_name = split_asset_path(asset_path)
    factory = unreal.BlueprintFactory()
    set_property_any(factory, ["parent_class", "ParentClass"], parent_class)
    asset = ASSET_TOOLS.create_asset(asset_name, package_path, unreal.Blueprint, factory)
    if not asset:
        raise RuntimeError("Failed to create Blueprint: {0}".format(asset_path))
    compile_blueprint(asset, asset_path)
    save_asset(asset_path)
    return asset


def create_data_asset(asset_path, data_asset_class):
    ensure_dir_for_asset(asset_path)
    existing = load_asset(asset_path)
    if existing:
        note("DataAsset already exists: {0}".format(asset_path))
        return existing

    package_path, asset_name = split_asset_path(asset_path)
    factory = unreal.DataAssetFactory()
    set_property_any(factory, ["data_asset_class", "DataAssetClass"], data_asset_class)
    try:
        asset = ASSET_TOOLS.create_asset(asset_name, package_path, data_asset_class, factory)
    except Exception:
        asset = ASSET_TOOLS.create_asset(asset_name, package_path, unreal.DataAsset, factory)
    if not asset:
        raise RuntimeError("Failed to create DataAsset: {0}".format(asset_path))
    save_asset(asset_path)
    return asset


def make_tag(tag_name):
    tag = unreal.GameplayTag()
    if not tag.import_text(tag_name):
        raise RuntimeError("Failed to import GameplayTag: {0}".format(tag_name))
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        raise RuntimeError("Invalid GameplayTag: {0}".format(tag_name))
    return tag


def make_tag_container(tag_names):
    container = unreal.GameplayTagContainer()
    if not tag_names:
        return container
    entries = ",".join(['(TagName="{0}")'.format(tag_name) for tag_name in tag_names])
    text = "(GameplayTags=({0}))".format(entries)
    if not container.import_text(text):
        raise RuntimeError("Failed to import GameplayTagContainer: {0}".format(text))
    return container


def enum_value(enum_class, names):
    for name in names:
        if hasattr(enum_class, name):
            return getattr(enum_class, name)
    raise RuntimeError("Missing enum value {0} on {1}".format(names, enum_class))


def create_floor(score, stage_tag, event_tag):
    floor = unreal.FloorConfig()
    floor.set_editor_property("total_difficulty_score", score)
    floor.set_editor_property("force_elite", False)
    floor.set_editor_property("elite_chance", 0.0)
    floor.set_editor_property("shop_chance", 0.0)
    floor.set_editor_property("event_chance", 0.0)
    floor.set_editor_property("global_stage_tag", make_tag(stage_tag))
    floor.set_editor_property("story_event_tags", make_tag_container([event_tag]))
    return floor


def create_story_entry(event_tag, action_type, tutorial_event_id, pause_game, note_text):
    entry = unreal.StoryEventEntry()
    entry.set_editor_property("event_tag", make_tag(event_tag))
    entry.set_editor_property("action_type", action_type)
    if tutorial_event_id:
        entry.set_editor_property("tutorial_event_id", unreal.Name(tutorial_event_id))
    entry.set_editor_property("pause_game", pause_game)
    entry.set_editor_property("only_when_tutorial_incomplete", True)
    entry.set_editor_property("fire_once_per_run", True)
    entry.set_editor_property("designer_note", unreal.Text(note_text))
    return entry


def configure_hud_and_game_mode(campaign=None, registry=None):
    hud_parent = load_class_or_raise("BP_YogHUD", ["/Script/DevKit.YogHUD", "/Script/DevKit.AYogHUD"])
    hud_bp = create_blueprint(HUD_BP, hud_parent)
    hud_cdo = get_cdo(HUD_BP)
    run_summary_class = load_bp_class(RUN_SUMMARY_WBP)
    set_property_any(hud_cdo, ["run_summary_widget_class", "RunSummaryWidgetClass"], run_summary_class)
    mark_dirty(hud_bp)
    compile_blueprint(hud_bp, HUD_BP)
    save_asset(HUD_BP)

    game_mode_bp = load_asset_or_raise(GAME_MODE_BP)
    game_mode_cdo = get_cdo(GAME_MODE_BP)
    hud_class = load_bp_class(HUD_BP)
    set_property_any(game_mode_cdo, ["hud_class", "HUDClass"], hud_class)
    if campaign:
        set_property_any(game_mode_cdo, ["campaign_data", "CampaignData"], campaign)
    if registry:
        set_property_any(game_mode_cdo, ["story_event_registry", "StoryEventRegistry"], registry)
    set_property_any(
        game_mode_cdo,
        ["dispatch_story_events_from_campaign", "bDispatchStoryEventsFromCampaign", "DispatchStoryEventsFromCampaign"],
        True,
        required=False,
    )
    mark_dirty(game_mode_bp)
    compile_blueprint(game_mode_bp, GAME_MODE_BP)
    save_asset(GAME_MODE_BP)


def configure_terminal():
    terminal_bp = load_asset_or_raise(TERMINAL_BP)
    terminal_cdo = get_cdo(TERMINAL_BP)
    tree_class = load_bp_class(TREE_WBP)
    set_property_any(terminal_cdo, ["widget_class", "WidgetClass"], tree_class)
    set_property_any(terminal_cdo, ["facility_display_name", "FacilityDisplayName"], unreal.Text("升级终端"))
    mark_dirty(terminal_bp)
    compile_blueprint(terminal_bp, TERMINAL_BP)
    save_asset(TERMINAL_BP)


def configure_meta_widgets():
    tree_bp = load_asset_or_raise(TREE_WBP)
    tree_cdo = get_cdo(TREE_WBP)
    card_class = load_bp_class(WBP_CARD)
    set_property_any(tree_cdo, ["node_card_widget_class", "NodeCardWidgetClass"], card_class)
    mark_dirty(tree_bp)
    compile_blueprint(tree_bp, TREE_WBP)
    save_asset(TREE_WBP)

    summary_bp = load_asset_or_raise(RUN_SUMMARY_WBP)
    summary_cdo = get_cdo(RUN_SUMMARY_WBP)
    set_property_any(summary_cdo, ["hub_level_name", "HubLevelName"], unreal.Name("L_HubTown"))
    mark_dirty(summary_bp)
    compile_blueprint(summary_bp, RUN_SUMMARY_WBP)
    save_asset(RUN_SUMMARY_WBP)


def configure_campaign():
    campaign = create_data_asset(CAMPAIGN_DA, unreal.CampaignDataAsset)
    campaign.modify()

    initial = load_asset_or_raise(ROOM_INITIAL)
    rooms = [
        load_asset_or_raise(ROOM_PRISON_NORMAL),
        load_asset_or_raise(ROOM_CORRIDOR_A),
        load_asset_or_raise(ROOM_CORRIDOR_B),
        load_asset_or_raise(ROOM_WATER_DUNGEON),
    ]

    floors = [
        create_floor(0, "Level.Stage.WeaponPickup", "Tutorial.WeaponPickup"),
        create_floor(10, "Level.Stage.FirstCombat", "Tutorial.CardConsume"),
        create_floor(10, "Level.Stage.ShuffleRoom", "Tutorial.Shuffle"),
        create_floor(10, "Level.Stage.Reward", "Tutorial.RewardToDeck"),
        create_floor(0, "Level.Stage.Backpack", "Tutorial.BackpackArrange"),
        create_floor(15, "Level.Stage.LinkCard", "Tutorial.LinkCard"),
        create_floor(20, "Level.Stage.Finisher", "Tutorial.Finisher"),
        create_floor(0, "Level.Stage.RouteChoice", "Tutorial.RouteRewardChoice"),
    ]

    campaign.set_editor_property("default_starting_room", initial)
    campaign.set_editor_property("room_pool", rooms)
    campaign.set_editor_property("floor_table", floors)
    mark_dirty(campaign)
    save_asset(CAMPAIGN_DA)
    return campaign


def configure_room_rewards():
    room = load_asset_or_raise(ROOM_PRISON_NORMAL)
    reward = unreal.MetaCurrencyCost()
    reward.set_editor_property("currency_tag", make_tag("Currency.Meta.Common.A"))
    reward.set_editor_property("amount", 25)
    room.modify()
    room.set_editor_property("meta_currency_rewards", [reward])
    rewards = room.get_editor_property("meta_currency_rewards")
    note("Configured {0} meta currency reward row(s) on {1}".format(len(rewards), ROOM_PRISON_NORMAL))
    mark_dirty(room)
    save_loaded_asset(room, ROOM_PRISON_NORMAL)


def configure_story_registry():
    registry = create_data_asset(STORY_REGISTRY_DA, unreal.StoryEventRegistryDA)
    action = unreal.StoryEventActionType
    tutorial_popup = enum_value(action, ["TUTORIAL_POPUP", "TutorialPopup"])
    broadcast_only = enum_value(action, ["BROADCAST_ONLY", "BroadcastOnly"])

    entries = [
        create_story_entry("Tutorial.WeaponPickup", tutorial_popup, "tutorial_weapon_pickup", True, "First run weapon pickup tutorial."),
        create_story_entry("Tutorial.CardConsume", broadcast_only, "", False, "First combat stage broadcast; combat HUD teaches card consume."),
        create_story_entry("Tutorial.Shuffle", tutorial_popup, "tutorial_shuffle_hint", False, "First shuffle hint."),
        create_story_entry("Tutorial.RewardToDeck", tutorial_popup, "tutorial_first_rune", True, "First reward enters deck tutorial."),
        create_story_entry("Tutorial.BackpackArrange", tutorial_popup, "tutorial_backpack", True, "First backpack arrangement tutorial."),
        create_story_entry("Tutorial.LinkCard", tutorial_popup, "tutorial_card_link", True, "First link card tutorial."),
        create_story_entry("Tutorial.Finisher", broadcast_only, "", False, "Finisher stage broadcast placeholder."),
        create_story_entry("Tutorial.RouteRewardChoice", broadcast_only, "", False, "Route choice stage broadcast placeholder."),
        create_story_entry("Tutorial.RunSummary", broadcast_only, "", False, "Run summary is handled by WBP_RunSummary."),
    ]

    registry.modify()
    registry.set_editor_property("entries", entries)
    mark_dirty(registry)
    save_asset(STORY_REGISTRY_DA)
    return registry


def spawn_actor(actor_class, location, rotation):
    if hasattr(unreal.EditorLevelLibrary, "spawn_actor_from_class"):
        return unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return actor_subsystem.spawn_actor_from_class(actor_class, location, rotation)


def destroy_actor(actor):
    if hasattr(unreal.EditorLevelLibrary, "destroy_actor"):
        unreal.EditorLevelLibrary.destroy_actor(actor)
    else:
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actor_subsystem.destroy_actor(actor)


def configure_hub_map():
    ensure_dir_for_asset(HUB_MAP)
    if unreal.EditorAssetLibrary.does_asset_exist(HUB_MAP):
        loaded = unreal.EditorLevelLibrary.load_level(HUB_MAP)
        note("Loaded hub map {0}: {1}".format(HUB_MAP, loaded))
    else:
        created = unreal.EditorLevelLibrary.new_level(HUB_MAP)
        note("Created hub map {0}: {1}".format(HUB_MAP, created))
        if not created:
            raise RuntimeError("Failed to create hub map: {0}".format(HUB_MAP))

    world = unreal.EditorLevelLibrary.get_editor_world()
    world_settings = world.get_world_settings()
    game_mode_class = load_bp_class(GAME_MODE_BP)
    set_property_any(world_settings, ["default_game_mode", "DefaultGameMode"], game_mode_class, required=False)
    mark_dirty(world_settings)

    for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
        label = actor.get_actor_label() if hasattr(actor, "get_actor_label") else actor.get_name()
        if label in ["PlayerStart_Hub", "BP_HubUpgradeTerminal_Upgrade"]:
            destroy_actor(actor)

    player_start = spawn_actor(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0))
    if player_start and hasattr(player_start, "set_actor_label"):
        player_start.set_actor_label("PlayerStart_Hub")

    terminal_class = load_bp_class(TERMINAL_BP)
    terminal = spawn_actor(terminal_class, unreal.Vector(250.0, 0.0, 0.0), unreal.Rotator(0.0, 180.0, 0.0))
    if terminal and hasattr(terminal, "set_actor_label"):
        terminal.set_actor_label("BP_HubUpgradeTerminal_Upgrade")

    unreal.EditorLevelLibrary.save_current_level()
    note("Saved hub map: {0}".format(HUB_MAP))


def main():
    configure_meta_widgets()
    configure_terminal()
    campaign = configure_campaign()
    configure_room_rewards()
    registry = configure_story_registry()
    configure_hud_and_game_mode(campaign, registry)
    configure_hub_map()

    out_path = os.path.join(unreal.Paths.project_saved_dir(), "StageOneLoopAssetsReport.txt")
    with open(out_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))
    note("Report: {0}".format(out_path))


main()
