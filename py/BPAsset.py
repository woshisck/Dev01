
import unreal 
import time

asset_datas_arry = unreal.EditorUtilityLibrary.get_selected_asset_data()

def get_component_handles(blueprint_asset_path):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

    blueprint_asset = unreal.load_asset(blueprint_asset_path)
    subobject_data_handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint_asset)
    return subobject_data_handles


def get_component_objects(blueprint_asset_path):
    objects = []
    handles = get_component_handles(blueprint_asset_path)
    for handle in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        object = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        objects.append(object)
    return objects    
    
def get_assets_by_class_in_folder(search_in, assetClass, assetRealClass):
    assetRegistry = unreal.AssetRegistryHelpers.get_asset_registry()
    assetsData = assetRegistry.get_assets_by_class(assetClass, True)
    inFolder = [x for x in assetsData if search_in in str(x.package_name)]
    found = [x for x in inFolder if assetRealClass in x.get_tag_value("ParentClass")]
    return found


def print_asset_comp(assetdata):
    components = get_component_objects(assetdata.package_name)
    for comp in components:
        if "InstancedStaticMeshComponent" in str(comp.static_class()):
            # comp.set_cull_distances(minCullDistance, maxCullDistance)
            ISM_component = unreal.InstancedStaticMeshComponent.cast(comp)
            #ISM_component.set_editor_property('collision_profile_name', unreal.CollisionProfileName('NoCollision'))
            ISM_component.set_collision_profile_name("NoCollision")
        elif "HierarchicalInstancedStaticMesh" in str(comp.static_class()):
            # comp.set_cull_distances(minCullDistance, maxCullDistance)
            HISM_component = unreal.InstancedStaticMeshComponent.cast(comp)
            HISM_component.set_collision_profile_name("NoCollision")
            #HISM_component.set_editor_property('collision_profile_name', unreal.CollisionProfileName('NoCollision'))


for asset in asset_datas_arry:
    print_asset_comp(asset)
    # unreal.EditorAssetLibrary.save_loaded_assets(asset_datas_arry, only_if_is_dirty=False)  
    # package_name = asset.get_editor_property('package_name')
