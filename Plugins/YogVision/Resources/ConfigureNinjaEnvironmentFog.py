"""Enable scene-space interaction on the owned, accepted single ground volume.

Use in L_NinjaFogCorridor with PIE stopped, after rebuilding the spatial/hatch
master and YogVisionRuntime. No vendor or original art material is modified.
Ground/finish colors, XYZ noise, brush placement and old far-volume-off state
are preserved. Only four native integration controls are changed on GroundMist;
the added environment actor owns all interaction settings in its Details panel.
"""
import json,pathlib,time,unreal
OWN='/Game/Art/Rendering/Fog/Ninja/'
P=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Rendering/Fog/NinjaSmokeExact'
TARGET={'Density':1.1,'Shadow':1.8,'LightIntensity':1.15,'SpatialLightSamples':6.0}

def configure(save=False):
 ed=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
 ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
 assert ed.get_editor_world().get_path_name()==OWN+'L_NinjaFogCorridor.L_NinjaFogCorridor' and not ed.get_game_world()
 fog=[a for a in ea.get_all_level_actors() if a.get_class().get_name()=='BP_VolumeSmokeContainer_C' and unreal.Name('YogNinjaSmokeExact') in a.tags]
 assert len(fog)==1 and fog[0].get_editor_property('VolumeSmokeMaterial').get_name()=='MI_Corridor_GroundMist'
 carrier=fog[0];level=carrier.get_outer()
 assert level.get_path_name()==OWN+'L_Corridor_NinjaLighting.L_Corridor_NinjaLighting:PersistentLevel'
 env=[a for a in ea.get_all_level_actors() if a.get_class().get_name()=='YogNinjaFogEnvironmentActor']
 assert len(env)<=1,'Review multiple environment controllers before proceeding'
 if not env:
  assert le.set_current_level_by_name('L_Corridor_NinjaLighting')
  a=ea.spawn_actor_from_class(unreal.YogNinjaFogEnvironmentActor,carrier.get_actor_location())
  assert a and a.get_outer()==level
  a.set_actor_label('空间雾 · 环境与清雾')
  a.set_editor_property('fog_carrier',carrier)
  a.sample_ground_height()
 else:
  a=env[0];assert a.get_outer()==level
  assert a.get_editor_property('fog_carrier')==carrier
 L=unreal.MaterialEditingLibrary;m=carrier.get_editor_property('VolumeSmokeMaterial')
 for n,v in TARGET.items():
  L.set_material_instance_scalar_parameter_value(m,n,v)
  assert abs(L.get_material_instance_scalar_parameter_value(m,n)-v)<.0001,n
 L.update_material_instance(m)
 assert L.get_material_instance_scalar_parameter_value(m,'SpatialProfileEnabled')==0
 carrier.call_method('UserConstructionScript')
 if hasattr(carrier,'refresh_lighting'):carrier.refresh_lighting()
 a.set_editor_property('environment_enabled',True);a.refresh_environment()
 ea.set_selected_level_actors([a])
 if save:
  assert unreal.EditorAssetLibrary.save_asset(OWN+'MI_Corridor_GroundMist',False)
  assert unreal.EditorAssetLibrary.save_asset(OWN+'L_Corridor_NinjaLighting',False)
 return carrier,a

if __name__=='__main__':configure(save=True)
