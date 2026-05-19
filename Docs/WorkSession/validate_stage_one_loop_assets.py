import os
import unreal

REPORT = []
FAILED = False


def note(text):
    REPORT.append(str(text))
    print(text)


def fail(text):
    global FAILED
    FAILED = True
    note("FAIL: {0}".format(text))


def object_path(asset_path):
    asset_name = asset_path.rsplit("/", 1)[1]
    return "{0}.{1}".format(asset_path, asset_name)


def load_asset(asset_path):
    asset = unreal.load_asset(object_path(asset_path))
    if not asset:
        asset = unreal.load_asset(asset_path)
    if not asset:
        fail("Missing asset {0}".format(asset_path))
    return asset


def load_bp_class(asset_path):
    try:
        loaded = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        if loaded:
            return loaded
    except Exception:
        pass
    asset = load_asset(asset_path)
    if asset:
        try:
            generated = asset.get_editor_property("generated_class")
            if generated:
                return generated
        except Exception:
            pass
    fail("Missing Blueprint generated class {0}".format(asset_path))
    return None


def get_cdo(asset_path):
    cls = load_bp_class(asset_path)
    return unreal.get_default_object(cls) if cls else None


def class_path(cls):
    return cls.get_path_name() if cls else ""


def name_text(value):
    return str(value)


def asset_path(asset):
    return asset.get_path_name() if asset else ""


def get_prop(obj, names):
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            pass
    fail("Missing property {0} on {1}".format(names, obj))
    return None


def tag_name(tag):
    return str(unreal.GameplayTagLibrary.get_tag_name(tag))


def container_tags(container):
    broken = unreal.GameplayTagLibrary.break_gameplay_tag_container(container)
    return [tag_name(tag) for tag in broken]


def row_names(table):
    names = []
    try:
        names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    except Exception:
        try:
            names = table.get_row_names()
        except Exception:
            names = []
    return [str(name) for name in names]


def check_widget(asset_path_value, widget_names):
    widget_bp = load_asset(asset_path_value)
    tree = None
    for property_name in ["widget_tree", "WidgetTree"]:
        try:
            tree = widget_bp.get_editor_property(property_name)
            if tree:
                break
        except Exception:
            pass
    if not tree:
        tree = getattr(widget_bp, "widget_tree", None) or getattr(widget_bp, "WidgetTree", None)
    if not tree:
        check(load_bp_class(asset_path_value) is not None, "{0} generated class exists".format(asset_path_value))
        note("INFO: {0} WidgetTree is not exposed to UE Python; required controls are built by MetaProgressionWidgetTreeSetup.".format(asset_path_value))
        return
    for widget_name in widget_names:
        found = None
        try:
            found = tree.find_widget(widget_name)
        except Exception:
            try:
                found = tree.find_widget(unreal.Name(widget_name))
            except Exception:
                found = None
        check(found is not None, "{0} contains widget {1}".format(asset_path_value, widget_name))


def enum_value(enum_class, names):
    for name in names:
        if hasattr(enum_class, name):
            return getattr(enum_class, name)
    fail("Missing enum value {0} on {1}".format(names, enum_class))
    return None


def check(condition, message):
    if condition:
        note("OK: {0}".format(message))
    else:
        fail(message)


def main():
    tree_class = load_bp_class("/Game/UI/MetaProgression/WBP_MetaUpgradeTree")
    card_class = load_bp_class("/Game/UI/MetaProgression/WBP_MetaNodeCard")
    summary_class = load_bp_class("/Game/UI/MetaProgression/WBP_RunSummary")
    hud_class = load_bp_class("/Game/UI/BP_YogHUD")
    terminal_class = load_bp_class("/Game/World/Hub/BP_HubUpgradeTerminal")

    currency_table = load_asset("/Game/MetaProgression/DT_MetaCurrencyRules")
    node_table = load_asset("/Game/MetaProgression/DT_MetaUpgradeNodes")
    check("Common_A" in row_names(currency_table), "DT_MetaCurrencyRules has Common_A")
    node_rows = row_names(node_table)
    check("Node.Flesh.HP.Basic" in node_rows, "DT_MetaUpgradeNodes has Node.Flesh.HP.Basic")
    check("Node.Flesh.Attack.Basic" in node_rows, "DT_MetaUpgradeNodes has Node.Flesh.Attack.Basic")

    check_widget(
        "/Game/UI/MetaProgression/WBP_MetaNodeCard",
        ["TxtNodeName", "TxtLevelProgress", "TxtCost", "ProgressLevel", "BtnPurchase"],
    )
    check_widget(
        "/Game/UI/MetaProgression/WBP_MetaUpgradeTree",
        ["TxtCurrencyAmount", "BtnFleshSide", "BtnMysticSide", "BtnClose", "NodeList"],
    )
    check_widget(
        "/Game/UI/MetaProgression/WBP_RunSummary",
        ["TxtFloorReached", "TxtEnemiesKilled", "BtnReturnToHub"],
    )

    hud_cdo = get_cdo("/Game/UI/BP_YogHUD")
    run_summary = get_prop(hud_cdo, ["run_summary_widget_class", "RunSummaryWidgetClass"])
    check(class_path(run_summary) == class_path(summary_class), "BP_YogHUD RunSummaryWidgetClass points to WBP_RunSummary")

    terminal_cdo = get_cdo("/Game/World/Hub/BP_HubUpgradeTerminal")
    widget_class = get_prop(terminal_cdo, ["widget_class", "WidgetClass"])
    check(class_path(widget_class) == class_path(tree_class), "BP_HubUpgradeTerminal WidgetClass points to WBP_MetaUpgradeTree")

    tree_cdo = get_cdo("/Game/UI/MetaProgression/WBP_MetaUpgradeTree")
    node_card = get_prop(tree_cdo, ["node_card_widget_class", "NodeCardWidgetClass"])
    check(class_path(node_card) == class_path(card_class), "WBP_MetaUpgradeTree NodeCardWidgetClass points to WBP_MetaNodeCard")

    summary_cdo = get_cdo("/Game/UI/MetaProgression/WBP_RunSummary")
    hub_level = get_prop(summary_cdo, ["hub_level_name", "HubLevelName"])
    check(name_text(hub_level) == "L_HubTown", "WBP_RunSummary HubLevelName is L_HubTown")

    game_mode_cdo = get_cdo("/Game/Code/Core/GameMode/B_GameMode")
    hud_assigned = get_prop(game_mode_cdo, ["hud_class", "HUDClass"])
    check(class_path(hud_assigned) == class_path(hud_class), "B_GameMode HUDClass points to BP_YogHUD")

    campaign = load_asset("/Game/Docs/Map/DA_Campaign_Tutorial")
    registry = load_asset("/Game/Data/Story/DA_StoryEventRegistry_Tutorial")
    campaign_assigned = get_prop(game_mode_cdo, ["campaign_data", "CampaignData"])
    registry_assigned = get_prop(game_mode_cdo, ["story_event_registry", "StoryEventRegistry"])
    check(asset_path(campaign_assigned) == asset_path(campaign), "B_GameMode CampaignData points to DA_Campaign_Tutorial")
    check(asset_path(registry_assigned) == asset_path(registry), "B_GameMode StoryEventRegistry points to DA_StoryEventRegistry_Tutorial")
    dispatch = get_prop(
        game_mode_cdo,
        ["dispatch_story_events_from_campaign", "bDispatchStoryEventsFromCampaign", "DispatchStoryEventsFromCampaign"],
    )
    check(bool(dispatch) is True, "B_GameMode dispatches story events from campaign")

    floors = campaign.get_editor_property("floor_table")
    expected = [
        ("Level.Stage.WeaponPickup", "Tutorial.WeaponPickup", 0),
        ("Level.Stage.FirstCombat", "Tutorial.CardConsume", 10),
        ("Level.Stage.ShuffleRoom", "Tutorial.Shuffle", 10),
        ("Level.Stage.Reward", "Tutorial.RewardToDeck", 10),
        ("Level.Stage.Backpack", "Tutorial.BackpackArrange", 0),
        ("Level.Stage.LinkCard", "Tutorial.LinkCard", 15),
        ("Level.Stage.Finisher", "Tutorial.Finisher", 20),
        ("Level.Stage.RouteChoice", "Tutorial.RouteRewardChoice", 0),
    ]
    check(len(floors) == len(expected), "DA_Campaign_Tutorial has 8 tutorial floors")
    for index, (stage, event, score) in enumerate(expected):
        if index >= len(floors):
            continue
        floor = floors[index]
        check(floor.get_editor_property("total_difficulty_score") == score, "Floor {0} difficulty score {1}".format(index, score))
        check(tag_name(floor.get_editor_property("global_stage_tag")) == stage, "Floor {0} stage {1}".format(index, stage))
        check(container_tags(floor.get_editor_property("story_event_tags")) == [event], "Floor {0} event {1}".format(index, event))

    entries = registry.get_editor_property("entries")
    action = unreal.StoryEventActionType
    tutorial_popup = enum_value(action, ["TUTORIAL_POPUP", "TutorialPopup"])
    broadcast_only = enum_value(action, ["BROADCAST_ONLY", "BroadcastOnly"])
    expected_events = [
        ("Tutorial.WeaponPickup", tutorial_popup, "tutorial_weapon_pickup", True),
        ("Tutorial.CardConsume", broadcast_only, "", False),
        ("Tutorial.Shuffle", tutorial_popup, "tutorial_shuffle_hint", False),
        ("Tutorial.RewardToDeck", tutorial_popup, "tutorial_first_rune", True),
        ("Tutorial.BackpackArrange", tutorial_popup, "tutorial_backpack", True),
        ("Tutorial.LinkCard", tutorial_popup, "tutorial_card_link", True),
        ("Tutorial.Finisher", broadcast_only, "", False),
        ("Tutorial.RouteRewardChoice", broadcast_only, "", False),
        ("Tutorial.RunSummary", broadcast_only, "", False),
    ]
    entry_events = [tag_name(entry.get_editor_property("event_tag")) for entry in entries]
    check(entry_events == [event for event, _, _, _ in expected_events], "Story registry has the expected tutorial event order")
    for index, (event, action_type, tutorial_id, pause_game) in enumerate(expected_events):
        if index >= len(entries):
            continue
        entry = entries[index]
        check(entry.get_editor_property("action_type") == action_type, "Story entry {0} action type matches".format(event))
        actual_tutorial_id = str(entry.get_editor_property("tutorial_event_id"))
        tutorial_id_matches = actual_tutorial_id == tutorial_id or (tutorial_id == "" and actual_tutorial_id in ["", "None"])
        check(tutorial_id_matches, "Story entry {0} tutorial id matches".format(event))
        check(bool(entry.get_editor_property("pause_game")) == pause_game, "Story entry {0} pause flag matches".format(event))
        check(bool(entry.get_editor_property("only_when_tutorial_incomplete")) is True, "Story entry {0} only when tutorial incomplete".format(event))
        check(bool(entry.get_editor_property("fire_once_per_run")) is True, "Story entry {0} fires once per run".format(event))

    room = load_asset("/Game/Docs/Map/DA_Room_Prison_Normal")
    rewards = room.get_editor_property("meta_currency_rewards")
    reward_ok = False
    for reward in rewards:
        if tag_name(reward.get_editor_property("currency_tag")) == "Currency.Meta.Common.A" and reward.get_editor_property("amount") == 25:
            reward_ok = True
    check(reward_ok, "DA_Room_Prison_Normal rewards 25 Currency.Meta.Common.A")

    check(unreal.EditorAssetLibrary.does_asset_exist("/Game/World/Hub/L_HubTown"), "L_HubTown map exists")
    loaded_hub = unreal.EditorLevelLibrary.load_level("/Game/World/Hub/L_HubTown")
    check(bool(loaded_hub), "L_HubTown map loads")
    labels = []
    terminal_found = False
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        label = actor.get_actor_label() if hasattr(actor, "get_actor_label") else actor.get_name()
        labels.append(label)
        actor_class_path = actor.get_class().get_path_name() if actor.get_class() else ""
        if label == "BP_HubUpgradeTerminal_Upgrade" and actor_class_path == class_path(terminal_class):
            terminal_found = True
    check("PlayerStart_Hub" in labels, "L_HubTown contains PlayerStart_Hub")
    check(terminal_found, "L_HubTown contains BP_HubUpgradeTerminal_Upgrade")

    out_path = os.path.join(unreal.Paths.project_saved_dir(), "StageOneLoopValidationReport.txt")
    if FAILED:
        note("VALIDATION_FAILED")
        with open(out_path, "w", encoding="utf-8") as handle:
            handle.write("\n".join(REPORT))
        raise RuntimeError("Stage one loop validation failed. See {0}".format(out_path))
    note("VALIDATION_PASSED")
    with open(out_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))


main()
