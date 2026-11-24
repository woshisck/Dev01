"""Select the 2D-driven Ninja hybrid variant in the owned corridor preview.

The existing GroundMist asset remains an editable previous variant. This
creates one small owned MIC, reuses the native carrier and live input material,
and saves only the new MIC and the owned lighting sublevel. Vendor assets,
original artist palette and the accepted scene-finish material are untouched.
"""
import unreal,json,pathlib,runpy
OWN='/Game/Art/Rendering/Fog/Ninja/'
R=pathlib.Path(__file__).parent
P=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Rendering/Fog/NinjaSmokeExact'
L=unreal.MaterialEditingLibrary
TARGET={
 'MaxViewSteps':32.,'Density':5.,'Extrude':.34,'ExtrudeSoften':.14,
 'DensityPow':1.,'DensityOffset':-.055,'XBackgroundDensity':.06,
 'XBackgroundDensityNoise':0.,'XBackgroundDensityFalloff':1.,
 'Shadow':1.8,'ShadowStepSize':.01,'LightIntensity':1.8,'LightAttenuationRadius':5500.,
 'FadeDistance':1000.,'FadeDistanceFalloff':500.,
 'HybridShadowSamples':6.,'HybridShadowReach':1.,
 'HybridNoiseAmount':.3,'HybridNoiseSize':1400.,'HybridNoiseFlow':.15,'HybridNoiseSpeed':.08,
 'HybridTimeScale':1.,'HybridTimeOffset':0.,
 'FogHatchStrength':.3,'FogHatchSpacing':9.,'FogHatchFlowStrength':1.,'FogHatchRegularity':.6,'FogHatchDebugMode':0.,
 'WorldUnitsPerPixel':2.,'ColorPow':1.,'ColorMul':1.,'ColorAdd':0.,
}

GENTLE_ENV={
 'spatial_height_enabled':True,'near_fog_height':80.,'far_fog_height':140.,
 'wall_fog_height':180.,'height_feather':70.,'player_strength':.30,'light_strength':.35,
 'ground_amount':1.6,
}

# Full-space atmosphere and local thinning are independent. The old height
# profile remains available for low banks; it must not cap this environment.
ATMOSPHERE_MATERIAL={
 'Density':5.,'Extrude':.34,'ExtrudeSoften':.14,
 'HybridAtmosphereAmount':.32,'HybridAtmosphereVariation':.20,
 'HybridBoundaryDetail':.35,'HybridBoundaryMotion':1.,
}
ATMOSPHERE_ENV={
 'spatial_height_enabled':False,'ground_body_height':160.,
 'ground_amount':1.25,'upper_amount':.85,'contact_boost':.18,
 'player_radius':280.,'player_feather':550.,'player_height':650.,
 'player_strength':.72,'light_strength':.60,'light_feather':500.,
}

BILLOW_MATERIAL={
 'HybridAtmosphereVariation':.78,'HybridNoiseSize':1000.,
 'HybridNoiseFlow':.45,'HybridNoiseSpeed':.30,
 'HybridBoundaryDetail':.8,'HybridBoundaryMotion':1.,
}

def apply_billow_profile(carrier,material):
 """Restore large 3D forms while preserving current density/clear-zone edits."""
 dm=carrier.get_editor_property('DynamicMaterialInstance')
 for n,v in BILLOW_MATERIAL.items():
  L.set_material_instance_scalar_parameter_value(material,n,v)
  if dm:dm.set_scalar_parameter_value(n,v)
 L.update_material_instance(material)

def apply_atmosphere_profile(carrier,environment,material):
 """One full-space medium; retain the carrier transform and Ninja input RT."""
 dm=carrier.get_editor_property('DynamicMaterialInstance')
 for n,v in ATMOSPHERE_MATERIAL.items():
  L.set_material_instance_scalar_parameter_value(material,n,v)
  if dm:dm.set_scalar_parameter_value(n,v)
 L.update_material_instance(material)
 for n,v in ATMOSPHERE_ENV.items():environment.set_editor_property(n,v)
 environment.refresh_environment()
 carrier.set_actor_label('02 空间环境雾 · Ninja')

def apply_gentle_profile(carrier,environment,material):
 """Apply the low, continuous art preset without rebuilding the live input RT."""
 dm=carrier.get_editor_property('DynamicMaterialInstance')
 for n in ['Density','Extrude','ExtrudeSoften']:
  L.set_material_instance_scalar_parameter_value(material,n,TARGET[n])
  if dm:dm.set_scalar_parameter_value(n,TARGET[n])
 L.update_material_instance(material)
 for n,v in GENTLE_ENV.items():environment.set_editor_property(n,v)
 environment.refresh_environment()

def configure(save=False):
 ed=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 assert ed.get_editor_world().get_path_name()==OWN+'L_NinjaFogCorridor.L_NinjaFogCorridor' and not ed.get_game_world()
 fog=[a for a in ea.get_all_level_actors() if a.get_class().get_name()=='BP_VolumeSmokeContainer_C' and unreal.Name('YogNinjaSmokeExact') in a.tags]
 env=[a for a in ea.get_all_level_actors() if a.get_class().get_name()=='YogNinjaFogEnvironmentActor']
 assert len(fog)==1 and len(env)==1
 a=fog[0];e=env[0]
 assert a.get_outer().get_path_name()==OWN+'L_Corridor_NinjaLighting.L_Corridor_NinjaLighting:PersistentLevel'
 assert e.get_editor_property('fog_carrier')==a
 m=unreal.load_asset(OWN+'MI_Corridor_HybridMist');created=not bool(m)
 if not m:
  m=unreal.EditorAssetLibrary.duplicate_asset(OWN+'MI_Corridor_GroundMist',OWN+'MI_Corridor_HybridMist')
  L.set_material_instance_parent(m,unreal.load_asset(OWN+'M_Corridor_FogHybrid'))
  L.clear_all_material_instance_parameters(m)
  H=runpy.run_path(str(R/'RestoreNinjaAcceptedFog.py'),run_name='hybrid_palette_helpers')
  original=H['STYLE']['ground']
  available={str(n) for n in L.get_vector_parameter_names(m)}
  for n,v in original['vectors'].items():
   if n in available:L.set_material_instance_vector_parameter_value(m,n,unreal.LinearColor(*v))
  available={str(n) for n in L.get_scalar_parameter_names(m)}
  for n,v in TARGET.items():
   assert n in available,n
   L.set_material_instance_scalar_parameter_value(m,n,v)
  # Preserve the original owned texture selection (the BP replaces its RT).
  old=unreal.load_asset(OWN+'MI_Corridor_GroundMist')
  texture=L.get_material_instance_texture_parameter_value(old,'NoiseVolume')
  if texture:L.set_material_instance_texture_parameter_value(m,'NoiseVolume',texture)
  L.update_material_instance(m)
 assert m.parent==unreal.load_asset(OWN+'M_Corridor_FogHybrid')
 a.set_editor_property('VolumeSmokeMaterial',m)
 a.set_actor_label('02 近地雾 · Ninja 2D混合')
 a.call_method('UserConstructionScript')
 if hasattr(a,'refresh_lighting'):a.refresh_lighting()
 e.set_editor_property('environment_enabled',True);e.set_editor_property('use_cached_environment',True)
 if created:apply_atmosphere_profile(a,e,m)
 e.refresh_environment();e.enable_editor_preview()
 ea.set_selected_level_actors([e])
 if save:
  assert unreal.EditorAssetLibrary.save_asset(OWN+'MI_Corridor_HybridMist',False)
  assert unreal.EditorAssetLibrary.save_asset(OWN+'L_Corridor_NinjaLighting',False)
 return a,e,m

if __name__=='__main__':configure(save=True)
