import os
import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

META_DIR = "/Game/MetaProgression"
DT_NODES = META_DIR + "/DT_MetaUpgradeNodes"
DT_CURRENCY = META_DIR + "/DT_MetaCurrencyRules"
WBP_TREE = "/Game/UI/MetaProgression/WBP_MetaUpgradeTree"
WBP_CARD = "/Game/UI/MetaProgression/WBP_MetaNodeCard"
BP_TERMINAL = "/Game/World/Hub/BP_HubUpgradeTerminal"

def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)

def create_data_table(asset_path, struct_path):
    ensure_dir(asset_path.rsplit("/", 1)[0])
    asset_name = asset_path.rsplit("/", 1)[1]
    existing = unreal.load_asset("{0}.{1}".format(asset_path, asset_name))
    if not existing:
        existing = unreal.load_asset(asset_path)
    if existing:
        return existing

    factory = unreal.DataTableFactory()
    row_struct = unreal.load_object(None, struct_path)
    if not row_struct:
        raise RuntimeError("Missing row struct: " + struct_path)
    factory.set_editor_property("struct", row_struct)
    package_path, asset_name = asset_path.rsplit("/", 1)
    asset = ASSET_TOOLS.create_asset(asset_name, package_path, unreal.DataTable, factory)
    if not asset:
        raise RuntimeError("Failed to create DataTable: " + asset_path)
    return asset

def fill_table_from_csv(table, csv_text, asset_path):
    imported = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, csv_text)
    if not imported:
        raise RuntimeError("CSV import failed for {0}".format(asset_path))
    unreal.EditorAssetLibrary.save_asset(asset_path, False)

currency_csv = """---,CurrencyTag,DisplayName,ShortName,Icon,MaxCapacity
Common_A,Currency.Meta.Common.A,星骸碎片,碎片,,0
"""

node_csv = """---,DisplayName,Side,MaxLevel,MysticLevelRequired,Prerequisites,CostsPerLevel,EffectType,StatAttribute,StatValuePerLevel,FeatureTag,StarterRuneToGrant,EditorPositionX,EditorPositionY
Node.Flesh.HP.Basic,生命强化Ⅰ,Flesh,3,0,,"((CurrencyTag=Currency.Meta.Common.A,Amount=10))",StatBoost,,0.0,,,0,0
Node.Flesh.Attack.Basic,攻击强化Ⅰ,Flesh,3,0,,"((CurrencyTag=Currency.Meta.Common.A,Amount=15))",StatBoost,,0.0,,,320,0
"""

currency_table = create_data_table(DT_CURRENCY, "/Script/DevKit.MetaCurrencyRow")
node_table = create_data_table(DT_NODES, "/Script/DevKit.MetaUpgradeNodeRow")
fill_table_from_csv(currency_table, currency_csv, DT_CURRENCY)
fill_table_from_csv(node_table, node_csv, DT_NODES)

def load_bp_class(asset_path):
    loaded_class = None
    try:
        loaded_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
    except Exception:
        pass
    if loaded_class:
        return loaded_class

    asset_name = asset_path.rsplit("/", 1)[1]
    asset = unreal.load_asset("{0}.{1}".format(asset_path, asset_name))
    if not asset:
        asset = unreal.load_asset(asset_path)
    if asset:
        for property_name in ["generated_class", "GeneratedClass"]:
            try:
                loaded_class = asset.get_editor_property(property_name)
                if loaded_class:
                    return loaded_class
            except Exception:
                pass
    raise RuntimeError("Failed to load Blueprint class: " + asset_path)

def get_cdo(asset_path):
    cls = load_bp_class(asset_path)
    return unreal.get_default_object(cls)

tree_asset = unreal.load_asset(WBP_TREE)
terminal_asset = unreal.load_asset(BP_TERMINAL)

if tree_asset:
    tree_cdo = get_cdo(WBP_TREE)
    card_class = load_bp_class(WBP_CARD)
    tree_cdo.set_editor_property("node_card_widget_class", card_class)
    tree_asset.mark_package_dirty()
    unreal.KismetEditorUtilities.compile_blueprint(tree_asset)
    unreal.EditorAssetLibrary.save_asset(WBP_TREE, False)

if terminal_asset:
    terminal_cdo = get_cdo(BP_TERMINAL)
    terminal_cdo.set_editor_property("facility_display_name", unreal.Text("升级终端"))
    terminal_asset.mark_package_dirty()
    unreal.KismetEditorUtilities.compile_blueprint(terminal_asset)
    unreal.EditorAssetLibrary.save_asset(BP_TERMINAL, False)

out_path = os.path.join(unreal.Paths.project_saved_dir(), "MetaProgressionEditorAssets.txt")
with open(out_path, "w", encoding="utf-8") as handle:
    handle.write("Created/updated:\n")
    handle.write(DT_NODES + "\n")
    handle.write(DT_CURRENCY + "\n")
    handle.write(WBP_TREE + " created by create_hub_blueprints.py; runtime fallback builds required controls\n")
    handle.write(BP_TERMINAL + " created by create_hub_blueprints.py; native default FacilityDisplayName is 升级终端\n")
