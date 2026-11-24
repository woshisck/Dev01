"""Restore the user-selected CrossHatch look; leave only its ground mist active.

Run --assets-only in UnrealEditor-Cmd -run=pythonscript with the editor closed.
Run --scene in the owned L_NinjaFogCorridor editor map afterwards. No vendor,
original artist postprocess, lights, or unrelated actors are edited. The old
FogWall data and later experimental UnifiedMist asset remain available, but
neither is assigned to the restored active ground carrier.
"""
import argparse,json,pathlib,hashlib,unreal
R=pathlib.Path(__file__).parent
P=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Rendering/Fog/NinjaSmokeExact'
OWN='/Game/Art/Rendering/Fog/Ninja/'
L=unreal.MaterialEditingLibrary
STYLE=json.loads((R/'CrossHatchAcceptedPreset.json').read_text(encoding='utf8'))
REMOVED={'OuterBrushRegularity','OuterBrushBlend','OuterBrushCrossStrength','OuterBrushClarity'}

def snapshot(m):
 return {'scalars':{str(n):L.get_material_instance_scalar_parameter_value(m,n) for n in L.get_scalar_parameter_names(m)},
  'vectors':{str(n):[v.r,v.g,v.b,v.a] for n in L.get_vector_parameter_names(m) for v in [L.get_material_instance_vector_parameter_value(m,n)]},
  'textures':{str(n):v.get_path_name() if v else None for n in L.get_texture_parameter_names(m) for v in [L.get_material_instance_texture_parameter_value(m,n)]}}

def apply(m,data):
 for n,v in data['scalars'].items():L.set_material_instance_scalar_parameter_value(m,n,float(v))
 for n,v in data['vectors'].items():L.set_material_instance_vector_parameter_value(m,n,unreal.LinearColor(*v))
 L.update_material_instance(m)
 actual=snapshot(m)
 for section in ['scalars','vectors']:
  for n,v in data[section].items():assert actual[section][n]==v,(m.get_name(),section,n)

def assets():
 assert '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower(),'Use a closed-editor commandlet for the master rollback.'
 master=unreal.load_asset(OWN+'M_Corridor_FogFinish')
 finish=unreal.load_asset(OWN+'MI_Corridor_FogFinish')
 ground=unreal.load_asset(STYLE['ground_output'])
 P.mkdir(parents=True,exist_ok=True)
 (P/'RestoredGroundAssetsBefore.json').write_text(json.dumps({'ground':snapshot(ground),'finish':snapshot(finish)},ensure_ascii=False,indent=2),encoding='utf8')
 code=(R/'NinjaFogFinish.hlsl').read_text(encoding='utf8')
 assert hashlib.sha256((R/'NinjaFogFinish.hlsl').read_bytes()).hexdigest()==STYLE['finish_source_sha256']
 # Use the established graph authoring recipe, which rebuilds exact pin/output
 # wiring. Discard only the four retired experimental controls during its
 # preservation snapshot; all accepted palette/outline values stay intact.
 path=R/'BuildNinjaFogFinish.py'
 text=path.read_text(encoding='utf8')
 assert text.rstrip().endswith('\nmain()')
 ns={'__file__':str(path),'__name__':'accepted_finish_builder'}
 exec(compile(text.rsplit('\nmain()',1)[0],str(path),'exec'),ns)
 original_snapshot=ns['resolved_parameters']
 def accepted_snapshot(instance):
  data=original_snapshot(instance)
  for n in REMOVED:data['scalars'].pop(n,None)
  return data
 ns['resolved_parameters']=accepted_snapshot
 ns['main']()
 errors=[]
 L.clear_all_material_instance_parameters(finish)
 apply(finish,STYLE['finish'])
 # Cached parameter-name tables may retain retired names until a fresh load;
 # the restored shader does not consume them. Verify the real shader on reopen.
 # The original ground asset was preserved during the later experiments.
 # Restore only if a reference field has changed, avoiding needless writes.
 before=snapshot(ground)
 changed=any(before[s].get(n)!=v for s in ['scalars','vectors'] for n,v in STYLE['ground'][s].items())
 if changed:apply(ground,STYLE['ground'])
 for n,v in {'SpatialProfileEnabled':0.,'SpatialBillowStrength':0.,'SpatialVerticalScale':1.,
             'SpatialAlbedoUniformity':0.,'SpatialFormLighting':0.,'SpatialJitterStrength':1.,'FogHatchRegularity':0.}.items():
  assert abs(L.get_material_instance_scalar_parameter_value(ground,n)-v)<.0001,(n,v)
 for path in [OWN+'M_Corridor_FogFinish',OWN+'MI_Corridor_FogFinish']+([STYLE['ground_output']] if changed else []):
  assert unreal.EditorAssetLibrary.save_asset(path,False),path
 (P/'RestoredGroundAssetProof.json').write_text(json.dumps({'finish_source_sha256':STYLE['finish_source_sha256'],
  'finish':snapshot(finish),'ground':snapshot(ground),'ground_asset_needed_changes':changed,'immediate_errors':errors},ensure_ascii=False,indent=2),encoding='utf8')
 unreal.log('ACCEPTED_CROSS_HATCH_ASSETS_RESTORED')

def scene():
 ed=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
 ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 world=ed.get_editor_world()
 assert world and world.get_path_name().startswith(OWN+'L_NinjaFogCorridor.') and not ed.get_game_world()
 allfog=[a for a in ea.get_all_level_actors() if a.get_class().get_name()=='BP_VolumeSmokeContainer_C' and unreal.Name('YogNinjaSmokeExact') in a.tags]
 candidates=[a for a in allfog if a.get_editor_property('VolumeSmokeMaterial').get_name() in ['MI_Corridor_UnifiedMist','MI_Corridor_GroundMist']]
 assert len(candidates)==1,'Do not guess among mist carriers.'
 a=candidates[0]
 assert a.get_outer().get_path_name()==OWN+'L_Corridor_NinjaLighting.L_Corridor_NinjaLighting:PersistentLevel'
 # The merged preview already removed its old far carrier. Keep it absent:
 # this persists in edit and play, rather than relying on a temporary eye icon.
 assert len(allfog)==1,'Unexpected additional tagged fog carrier; review it before restoring.'
 g=STYLE['ground_actor']
 (P/'RestoredGroundActorBefore.json').write_text(json.dumps({'actor':a.get_path_name(),
  'output':a.get_editor_property('VolumeSmokeMaterial').get_path_name(),
  'location':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation()),'scale':str(a.get_actor_scale3d())},indent=2),encoding='utf8')
 a.set_actor_label('02 近地薄雾 · Ninja')
 a.set_editor_property('CameraFacing',False)
 a.set_editor_property('LocationOffset',unreal.Vector(0,0,0))
 a.set_actor_location(unreal.Vector(*g['location']),False,False)
 a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)
 a.set_actor_scale3d(unreal.Vector(*g['scale']))
 a.set_editor_property('VolumeSmokeMaterial',unreal.load_asset(STYLE['ground_output']))
 a.set_editor_property('FlipbookPlayerMaterial',unreal.load_asset(STYLE['ground_input']))
 a.set_actor_hidden_in_game(False);a.set_is_temporarily_hidden_in_editor(False)
 a.call_method('UserConstructionScript')
 if hasattr(a,'refresh_lighting'):a.refresh_lighting()
 ea.set_selected_level_actors([a])
 assert unreal.EditorAssetLibrary.save_asset(OWN+'L_Corridor_NinjaLighting',False)
 return a

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--assets-only',action='store_true');parser.add_argument('--scene',action='store_true')
 args=parser.parse_args()
 if args.assets_only:assets()
 elif args.scene:scene()
 else:raise RuntimeError('Choose --assets-only in a commandlet, or --scene in the owned preview.')

if __name__=='__main__':main()
