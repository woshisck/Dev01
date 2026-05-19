import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

WBP_META_UPGRADE_TREE = "/Game/UI/MetaProgression/WBP_MetaUpgradeTree"
WBP_META_NODE_CARD = "/Game/UI/MetaProgression/WBP_MetaNodeCard"
WBP_RUN_SUMMARY = "/Game/UI/MetaProgression/WBP_RunSummary"
BP_HUB_FACILITY_ACTOR = "/Game/World/Hub/BP_HubFacilityActor"
BP_HUB_UPGRADE_TERMINAL = "/Game/World/Hub/BP_HubUpgradeTerminal"

def split_asset_path(asset_path):
    package_path, asset_name = asset_path.rsplit("/", 1)
    return package_path, asset_name

def ensure_directory_for_asset(asset_path):
    package_path, _ = split_asset_path(asset_path)
    try:
        exists = unreal.EditorAssetLibrary.does_directory_exist(package_path)
    except Exception:
        exists = False

    if not exists:
        unreal.EditorAssetLibrary.make_directory(package_path)
        print("Directory ready:", package_path)

    return package_path

def load_required_class(label, candidate_paths):
    for class_path in candidate_paths:
        loaded_class = unreal.load_class(None, class_path)
        if loaded_class:
            print("Loaded parent class for", label + ":", class_path)
            return loaded_class

    raise RuntimeError("Failed to load parent class for {0}. Tried: {1}".format(label, ", ".join(candidate_paths)))

def set_editor_property_any(obj, property_names, value):
    for property_name in property_names:
        try:
            obj.set_editor_property(property_name, value)
            return property_name
        except Exception:
            pass
    return None

def compile_blueprint(asset, asset_path):
    try:
        unreal.KismetEditorUtilities.compile_blueprint(asset)
        print("Compiled:", asset_path)
        return
    except Exception as first_error:
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(asset)
            print("Compiled:", asset_path)
            return
        except Exception:
            print("Compile skipped or failed:", asset_path, "-", first_error)

def save_asset(asset_path):
    if unreal.EditorAssetLibrary.save_asset(asset_path, False):
        print("Saved:", asset_path)
    else:
        print("Save returned False:", asset_path)

def create_blueprint_asset(asset_path, parent_class, is_widget_blueprint):
    ensure_directory_for_asset(asset_path)

    package_path, asset_name = split_asset_path(asset_path)
    existing_asset = unreal.load_asset("{0}.{1}".format(asset_path, asset_name))
    if not existing_asset:
        existing_asset = unreal.load_asset(asset_path)

    if existing_asset or unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        print("Skip existing asset:", asset_path)
        return existing_asset if existing_asset else unreal.load_asset(asset_path), False

    factory = unreal.WidgetBlueprintFactory() if is_widget_blueprint else unreal.BlueprintFactory()
    asset_class = unreal.WidgetBlueprint if is_widget_blueprint else unreal.Blueprint
    asset_kind = "Widget Blueprint" if is_widget_blueprint else "Blueprint"

    if not set_editor_property_any(factory, ["parent_class", "ParentClass"], parent_class):
        raise RuntimeError("Failed to set parent class on factory for " + asset_path)

    print("Creating {0}: {1}".format(asset_kind, asset_path))
    asset = ASSET_TOOLS.create_asset(asset_name, package_path, asset_class, factory)

    if not asset:
        raise RuntimeError("Failed to create asset: " + asset_path)

    compile_blueprint(asset, asset_path)
    save_asset(asset_path)
    return asset, True

def load_blueprint_class_or_raise(asset_path):
    loaded_class = None

    try:
        loaded_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
    except Exception:
        loaded_class = None

    if loaded_class:
        return loaded_class

    asset = unreal.load_asset(asset_path)
    if asset:
        for property_name in ["generated_class", "GeneratedClass"]:
            try:
                loaded_class = asset.get_editor_property(property_name)
                if loaded_class:
                    return loaded_class
            except Exception:
                pass

        try:
            generated_class_attr = getattr(asset, "generated_class")
            loaded_class = generated_class_attr() if callable(generated_class_attr) else generated_class_attr
            if loaded_class:
                return loaded_class
        except Exception:
            pass

    raise RuntimeError("Failed to load generated Blueprint class: " + asset_path)

def get_default_object_or_raise(loaded_class, asset_path):
    try:
        cdo = unreal.get_default_object(loaded_class)
        if cdo:
            return cdo
    except Exception:
        pass

    try:
        cdo = loaded_class.get_default_object()
        if cdo:
            return cdo
    except Exception:
        pass

    raise RuntimeError("Failed to get class default object for: " + asset_path)

def make_soft_class_path(asset_path):
    asset_name = asset_path.rsplit("/", 1)[1]
    generated_class_path = "{0}.{1}_C".format(asset_path, asset_name)

    try:
        return unreal.SoftClassPath(generated_class_path)
    except Exception:
        pass

    try:
        return unreal.SoftObjectPath(generated_class_path)
    except Exception:
        pass

    return None

def set_widget_class_on_blueprint(blueprint_asset_path, widget_blueprint_asset_path):
    print("Setting WidgetClass:", blueprint_asset_path, "->", widget_blueprint_asset_path)

    blueprint_asset = unreal.load_asset(blueprint_asset_path)
    if not blueprint_asset:
        raise RuntimeError("Failed to load Blueprint asset: " + blueprint_asset_path)

    widget_class = load_blueprint_class_or_raise(widget_blueprint_asset_path)
    blueprint_class = load_blueprint_class_or_raise(blueprint_asset_path)
    cdo = get_default_object_or_raise(blueprint_class, blueprint_asset_path)

    try:
        blueprint_asset.modify()
    except Exception:
        pass

    try:
        cdo.modify()
    except Exception:
        pass

    property_name = set_editor_property_any(cdo, ["widget_class", "WidgetClass"], widget_class)

    if not property_name:
        soft_class_path = make_soft_class_path(widget_blueprint_asset_path)
        if soft_class_path:
            property_name = set_editor_property_any(cdo, ["widget_class", "WidgetClass"], soft_class_path)

    if not property_name:
        raise RuntimeError("Failed to set WidgetClass on " + blueprint_asset_path)

    try:
        cdo.post_edit_change()
    except Exception:
        pass

    try:
        blueprint_asset.mark_package_dirty()
    except Exception:
        pass

    save_asset(blueprint_asset_path)
    print("WidgetClass set using property:", property_name)

meta_upgrade_tree_parent = load_required_class(
    "WBP_MetaUpgradeTree",
    ["/Script/DevKit.YogMetaUpgradeTreeWidgetBase", "/Script/DevKit.UYogMetaUpgradeTreeWidgetBase"]
)

meta_node_card_parent = load_required_class(
    "WBP_MetaNodeCard",
    ["/Script/DevKit.MetaNodeCardWidgetBase", "/Script/DevKit.UMetaNodeCardWidgetBase"]
)

run_summary_parent = load_required_class(
    "WBP_RunSummary",
    ["/Script/DevKit.YogRunSummaryWidgetBase", "/Script/DevKit.UYogRunSummaryWidgetBase"]
)

hub_facility_parent = load_required_class(
    "BP_HubFacilityActor",
    ["/Script/DevKit.HubFacilityActor", "/Script/DevKit.AHubFacilityActor"]
)

create_blueprint_asset(WBP_META_UPGRADE_TREE, meta_upgrade_tree_parent, True)
print("BindWidgetOptional expected:", "TxtCurrencyAmount, BtnFleshSide, BtnMysticSide, BtnClose")

create_blueprint_asset(WBP_META_NODE_CARD, meta_node_card_parent, True)
print("BindWidgetOptional expected:", "TxtNodeName, TxtLevelProgress, TxtCost, ProgressLevel, BtnPurchase")

create_blueprint_asset(WBP_RUN_SUMMARY, run_summary_parent, True)
print("BindWidgetOptional expected:", "TxtFloorReached, TxtEnemiesKilled, BtnReturnToHub")

create_blueprint_asset(BP_HUB_FACILITY_ACTOR, hub_facility_parent, False)

hub_facility_blueprint_class = load_blueprint_class_or_raise(BP_HUB_FACILITY_ACTOR)
_, terminal_created = create_blueprint_asset(BP_HUB_UPGRADE_TERMINAL, hub_facility_blueprint_class, False)

if terminal_created:
    set_widget_class_on_blueprint(BP_HUB_UPGRADE_TERMINAL, WBP_META_UPGRADE_TREE)
else:
    print("Skip WidgetClass update because asset already exists:", BP_HUB_UPGRADE_TERMINAL)

print("Done.")
