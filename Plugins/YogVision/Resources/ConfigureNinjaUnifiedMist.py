"""Configure the ONE owned Ninja mist carrier in the already-open preview.

Run in the editor with /Game/Art/Rendering/Fog/Ninja/L_NinjaFogCorridor open,
its L_Corridor_NinjaLighting sublevel loaded, and PIE/SIE stopped. This does
not open maps, spawn carriers, edit vendor assets, or rebuild shaders.

    py ".../ConfigureNinjaUnifiedMist.py"
    py ".../ConfigureNinjaUnifiedMist.py" --reset-style
    py ".../ConfigureNinjaUnifiedMist.py" --save

Default: duplicate the old GroundMist instance only if UnifiedMist is missing,
apply the current candidate scalar recipe ONLY to that new instance, reuse the existing
tagged UnifiedMist/GroundMist actor, and remove only its tagged native FogWall
sibling in the same exact owned lighting level. Existing UnifiedMist artist
parameters are preserved. The canonical carrier layout/zero offset is applied
on every run. --reset-style explicitly reapplies the mist scalar recipe, zero
PositionOffset, and ONLY eight listed FogFinish stroke controls. It does not
reset colors, textures, outline, far-fog distances, or the old two instances.
This working recipe is still awaiting visual acceptance.

Changes stay unsaved unless --save is supplied; only the owned UnifiedMist,
lighting map, and explicitly reset FogFinish may be saved. A small pre-change
JSON in Saved/Rendering/Fog/NinjaSmokeExact/UnifiedMistRecipe records original
actor class/material/light/transform configuration for recovery. It is not a
project or binary backup. Removed siblings remain recoverable from their old
material/blueprint assets and this configuration (or the unsaved map/Undo).
Requires BuildNinjaSpatialFog.py followed by BuildNinjaFogHatching.py first.
"""
import argparse
import json
import sys
import time
from pathlib import Path

import unreal


OWN = '/Game/Art/Rendering/Fog/Ninja/'
PREVIEW = OWN+'L_NinjaFogCorridor.L_NinjaFogCorridor'
LIGHTING = OWN+'L_Corridor_NinjaLighting'
LEVEL_OBJECT = LIGHTING+'.L_Corridor_NinjaLighting:PersistentLevel'
NATIVE_CLASS = ('/Game/FluidNinja/Usecases/Volumetrics/'
                'BP_VolumeSmokeContainer.BP_VolumeSmokeContainer_C')
SMOKE_TAG = unreal.Name('YogNinjaSmokeExact')
GROUND = OWN+'MI_Corridor_GroundMist'
WALL = OWN+'MI_Corridor_FogWall'
UNIFIED = OWN+'MI_Corridor_UnifiedMist'
MASTER = OWN+'M_Corridor_FogSpatial'
FINISH = OWN+'MI_Corridor_FogFinish'
L = unreal.MaterialEditingLibrary

# Current Billow/Form/Jitter working candidate; visual acceptance is pending.
# Preserve the original clear-zone/density/color contract outside these values.
TARGET = {
    'SpatialProfileEnabled':1., 'SpatialGroundHeight':0., 'SpatialGroundThickness':360.,
    'SpatialUpperAmount':.15, 'SpatialVolumeContrast':1.55,
    'SpatialCloudSize':1500., 'SpatialCoverage':.40, 'SpatialWarp':.65,
    'SpatialSpeed':.14, 'SpatialDetail':0., 'SpatialTimeScale':1., 'SpatialTimeOffset':0.,
    'SpatialBillowStrength':1., 'SpatialVerticalScale':1., 'SpatialAlbedoUniformity':.85,
    'SpatialFormLighting':1., 'SpatialJitterStrength':.12,
    'Density':2.6, 'MaxViewSteps':96., 'Extrude':.48, 'ExtrudeSoften':.02,
    'XBackgroundDensity':.035, 'Shadow':2.2, 'ShadowStepSize':.01,
    'SpatialLightSamples':6., 'SpatialShadowReach':1.5, 'LightIntensity':2.,
    'FadeDistance':1000., 'FadeDistanceFalloff':500.,
    'SpatialNearDistance':1600., 'SpatialNearFeather':1000.,
    'SpatialCenterStrength':.88, 'SpatialCenterRadius':.26,
    'SpatialCenterFeather':.38, 'SpatialCenterResidual':.10,
    'FogHatchStrength':.12, 'FogHatchSpacing':9., 'FogHatchFlowStrength':.3,
    'FogHatchRegularity':.85, 'FogHatchDebugMode':0.,
}
FINISH_TARGET = {
    'OuterBrushClarity':.90, 'OuterBrushStrength':1., 'OuterBrushSpacing':16.,
    'OuterBrushWidth':2., 'OuterBrushDensity':.68, 'OuterBrushRegularity':.90,
    'OuterBrushBlend':.60, 'OuterBrushCrossStrength':.30,
}


def asset_path(obj):
    return obj.get_path_name().split('.')[0] if obj else None


def vector(value):
    return [value.x, value.y, value.z]


def scalar_values(instance):
    return {str(name):L.get_material_instance_scalar_parameter_value(instance,name)
            for name in L.get_scalar_parameter_names(instance)}


def material_parameters(instance):
    return {
        'scalars':scalar_values(instance),
        'vectors':{str(name):[value.r,value.g,value.b,value.a]
                   for name in L.get_vector_parameter_names(instance)
                   for value in [L.get_material_instance_vector_parameter_value(instance,name)]},
        'textures':{str(name):asset_path(L.get_material_instance_texture_parameter_value(instance,name))
                    for name in L.get_texture_parameter_names(instance)},
    }


def check_parameters(instance, wanted):
    actual = scalar_values(instance)
    missing = set(wanted)-set(actual)
    assert not missing, ('Build the owned spatial/hatching masters first', sorted(missing))


def set_scalars(instance, values):
    for name,value in values.items():
        # UE 5.8 MaterialEditingLibrary.cpp leaves bResult=false in these
        # instance setters even after a successful write. Trust readback only.
        L.set_material_instance_scalar_parameter_value(instance,name,float(value))
    L.update_material_instance(instance)
    actual = scalar_values(instance)
    assert all(abs(actual[name]-value)<0.0001 for name,value in values.items())


def actor_configuration(actor):
    rotation = actor.get_actor_rotation()
    return {
        'path':actor.get_path_name(), 'class':actor.get_class().get_path_name(),
        'label':actor.get_actor_label(), 'level':actor.get_outer().get_path_name(),
        'location':vector(actor.get_actor_location()),
        'rotation':[rotation.pitch,rotation.yaw,rotation.roll],
        'scale':vector(actor.get_actor_scale3d()),
        'tags':[str(tag) for tag in actor.get_editor_property('tags')],
        'CameraFacing':bool(actor.get_editor_property('CameraFacing')),
        'LocationOffset':vector(actor.get_editor_property('LocationOffset')),
        'hidden_editor':bool(actor.is_hidden_ed()),
        'hidden_game':bool(actor.get_editor_property('hidden')),
        'input':asset_path(actor.get_editor_property('FlipbookPlayerMaterial')),
        'output':asset_path(actor.get_editor_property('VolumeSmokeMaterial')),
        'tracked_light':actor.get_editor_property('TrackThisActorAsPointLight').get_path_name()
            if actor.get_editor_property('TrackThisActorAsPointLight') else None,
        'tick_light':bool(actor.get_editor_property('TickPointLightPositionPerFrame')),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reset-style',action='store_true',
                        help='Explicitly apply candidate mist scalars and eight Finish stroke controls; visual acceptance is pending.')
    parser.add_argument('--save',action='store_true',
                        help='Save only the changed owned instance(s) and owned lighting level.')
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)

    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = editor.get_editor_world()
    assert world and world.get_path_name()==PREVIEW, 'Open ONLY the owned L_NinjaFogCorridor preview first.'
    assert not editor.get_game_world(), 'Stop PIE/SIE before configuring the owned mist.'
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([OWN.rstrip('/')],True)

    owned = [actor for actor in actors.get_all_level_actors()
             if actor.get_outer().get_path_name()==LEVEL_OBJECT
             and actor.get_class().get_path_name()==NATIVE_CLASS
             and SMOKE_TAG in actor.get_editor_property('tags')]
    unified_actors = [a for a in owned if asset_path(a.get_editor_property('VolumeSmokeMaterial'))==UNIFIED]
    ground_actors = [a for a in owned if asset_path(a.get_editor_property('VolumeSmokeMaterial'))==GROUND]
    candidates = unified_actors or ground_actors
    assert len(candidates)==1, 'Need exactly one owned UnifiedMist or GroundMist carrier; do not guess among duplicates.'
    assert not (unified_actors and ground_actors), 'Both ground and unified carriers exist; resolve that ambiguity first.'
    carrier = candidates[0]
    siblings = [a for a in owned if a!=carrier and asset_path(a.get_editor_property('VolumeSmokeMaterial'))==WALL]
    assert all(a==carrier or a in siblings for a in owned), 'Unexpected tagged mist actor; no automatic cleanup.'

    source = unreal.load_asset(GROUND)
    instance = unreal.load_asset(UNIFIED) if unreal.EditorAssetLibrary.does_asset_exist(UNIFIED) else None
    assert isinstance(source,unreal.MaterialInstanceConstant) and asset_path(source.parent)==MASTER
    assert instance is None or (isinstance(instance,unreal.MaterialInstanceConstant) and asset_path(instance.parent)==MASTER)
    check_parameters(instance or source,TARGET)
    finish = unreal.load_asset(FINISH) if args.reset_style else None
    if args.reset_style:
        assert isinstance(finish,unreal.MaterialInstanceConstant) and asset_path(finish)==FINISH
        check_parameters(finish,FINISH_TARGET)
    if instance is not None and not args.reset_style:
        offset = L.get_material_instance_vector_parameter_value(instance,'PositionOffset')
        assert max(abs(offset.r),abs(offset.g),abs(offset.b))<0.0001, (
            'Existing UnifiedMist PositionOffset must be zero; preserve its values or explicitly --reset-style.')

    # Store a small exact actor configuration before editing or removing any
    # actor. No old material, original artist asset, or vendor asset is written.
    record = {
        'timestamp_local':time.strftime('%Y-%m-%d %H:%M:%S'), 'world':PREVIEW,
        'lighting':LIGHTING, 'unified_preexisting':instance is not None,
        'reset_style':args.reset_style, 'actors':[actor_configuration(a) for a in [carrier]+siblings],
        'unified_parameters_before':material_parameters(instance) if instance else None,
        'finish_scalars_before':{n:L.get_material_instance_scalar_parameter_value(finish,n)
                                 for n in FINISH_TARGET} if finish else None,
    }
    directory = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Rendering/Fog/NinjaSmokeExact/UnifiedMistRecipe'
    directory.mkdir(parents=True,exist_ok=True)
    snapshot = directory/('ActorsBefore-'+time.strftime('%Y%m%d-%H%M%S')+'-'+str(time.time_ns()%1000000000)+'.json')
    snapshot.write_text(json.dumps(record,ensure_ascii=False,indent=2),encoding='utf8')

    created = instance is None
    if created:
        instance = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            'MI_Corridor_UnifiedMist',OWN.rstrip('/'),source)
        assert instance and asset_path(instance)==UNIFIED
    if created or args.reset_style:
        set_scalars(instance,TARGET)
        # Same UE 5.8 false return-value bug as the scalar setter above.
        L.set_material_instance_vector_parameter_value(instance,'PositionOffset',unreal.LinearColor(0,0,0,1))
        L.update_material_instance(instance)
        offset = L.get_material_instance_vector_parameter_value(instance,'PositionOffset')
        assert all(abs(actual-expected)<0.0001 for actual,expected in
                   zip([offset.r,offset.g,offset.b,offset.a],[0.,0.,0.,1.])), 'PositionOffset readback failed'
    if args.reset_style:
        set_scalars(finish,FINISH_TARGET)

    assert levels.set_current_level_by_name('L_Corridor_NinjaLighting')
    with unreal.ScopedEditorTransaction('配置自有单体积近地云雾'):
        carrier.modify()
        carrier.set_actor_label('01 单体积近地云雾 · Ninja')
        carrier.set_editor_property('CameraFacing',False)
        carrier.set_editor_property('LocationOffset',unreal.Vector(0,0,0))
        carrier.set_actor_location(unreal.Vector(1600,-450,650),False,False)
        carrier.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)
        carrier.set_actor_scale3d(unreal.Vector(44,44,20))
        carrier.set_editor_property('VolumeSmokeMaterial',instance)
        carrier.set_is_temporarily_hidden_in_editor(False)
        carrier.set_actor_hidden_in_game(False)
        carrier.call_method('UserConstructionScript')
        if hasattr(carrier,'refresh_lighting'):
            carrier.refresh_lighting()
        dynamic = carrier.get_editor_property('DynamicMaterialInstance')
        assert dynamic and asset_path(dynamic.parent)==UNIFIED
        actual_offset = dynamic.get_vector_parameter_value('PositionOffset')
        assert max(abs(actual_offset.r),abs(actual_offset.g),abs(actual_offset.b))<0.0001
        for sibling in siblings:
            assert sibling.get_outer()==carrier.get_outer()
            sibling.modify()
            assert actors.destroy_actor(sibling),sibling.get_path_name()
    actors.set_selected_level_actors([carrier])
    if not created and not args.reset_style:
        assert material_parameters(instance)==record['unified_parameters_before'], 'Existing UnifiedMist parameters changed unexpectedly.'
    if args.save:
        # A preceding unsaved run may have created the instance, so --save also
        # persists that already-existing owned asset without resetting values.
        assert unreal.EditorAssetLibrary.save_asset(UNIFIED,False)
        if args.reset_style:
            assert unreal.EditorAssetLibrary.save_asset(FINISH,False)
        assert unreal.EditorAssetLibrary.save_asset(LIGHTING,False)
    result = {
        'snapshot':str(snapshot), 'unified_created':created,
        'style_reset':args.reset_style, 'removed_owned_wall_carriers':len(siblings),
        'saved':args.save, 'carrier':carrier.get_path_name(),
        'existing_user_parameters_preserved':not args.reset_style,
        'vendor_and_original_artist_assets_modified':False,
    }
    unreal.log('NINJA_UNIFIED_MIST_CONFIGURED '+json.dumps(result,ensure_ascii=False))
    return result


if __name__=='__main__':
    main()
