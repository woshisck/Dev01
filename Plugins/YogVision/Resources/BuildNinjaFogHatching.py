"""Build owned in-pass world-anchored 2D hatching, preserving Ninja integration.

Run in UnrealEditor-Cmd with -run=pythonscript -AllowCommandletRendering,
with the live Editor closed. Requires the existing owned spatial fog master.
Only M_Corridor_FogSpatial is saved. No instance/level/vendor changes.
An empty immediate material-error list does not prove completed platform
shader compilation; validate the rendering Editor log and actual captures.
"""
import json
from pathlib import Path
import unreal

ROOT=Path(__file__).parent
L=unreal.MaterialEditingLibrary
OWN='/Game/Art/Rendering/Fog/Ninja/'
TARGET=OWN+'M_Corridor_FogSpatial'
SOURCE='/Game/FluidNinja/Usecases/Volumetrics/BaseMaterials/Output/M_VolumeSmoke'
PARAMETERS=[
    ('FogHatchStrength',0.65,'雾排线强度；只修饰雾颜色，保留真实透射率'),
    ('FogHatchSpacing',6.0,'雾排线间距（固定参考画面像素）'),
    ('FogHatchFlowStrength',1.0,'跟随当前 XYZ 密度动画方向的近似扰动强度'),
    ('FogHatchRegularity',0.0,'雾中排线规则度；0 保留原效果，1 使方向接近固定参考方向、减弱弯曲与交叉'),
    ('FogHatchDebugMode',0.0,'0 成图；1 雾遮罩；2 方向；3 受光（调试配合黑色场景）'),
    ('WorldUnitsPerPixel',2.0,'固定参考画面每像素对应的世界厘米；与场景排线保持相同'),
]
ANCHORS=[
    ('AnchorRight',(0.6870875,0.7265747,0.0,0.0),'固定参考画面向右的世界方向；与场景排线保持相同'),
    ('AnchorUp',(0.4167488,-0.3941015,0.8191520,0.0),'固定参考画面向上的世界方向；与场景排线保持相同'),
]


def canonical_spatial_helpers():
    # Load the canonical recipe without invoking its authoring main(). Original
    # licensed Ninja shader text is always read from the installed asset below.
    path=ROOT/'BuildNinjaSpatialFog.py'
    text=path.read_text(encoding='utf8')
    assert text.rstrip().endswith('\nmain()'), 'Canonical recipe entrypoint changed'
    namespace={'__file__':str(path),'__name__':'yog_spatial_authoring_helpers'}
    exec(compile(text.rsplit('\nmain()',1)[0],str(path),'exec'),namespace)
    return namespace


def hatching_code(spatial_code,helper_code,lit):
    """Append transport moments to the existing view march, never light march."""
    anchor='float absorptionfrommarch =previoustransmittance-transmittance;'
    assert spatial_code.count(anchor)==1, 'Native view absorption contract changed'
    code=spatial_code.replace(anchor,anchor+'''
fogHatchPositionSum+=absorptionfrommarch*RayOrigin;
fogHatchWeight+=absorptionfrommarch;
''')
    anchor='color+=absorptionfrommarch'
    assert code.count(anchor)==1
    light='saturate(lightvisibility)' if lit else '1.0'
    code=code.replace(anchor,'fogHatchLightSum+=absorptionfrommarch*'+light+';\n'+anchor)
    end='return float4(color,transmittance);'
    assert code.count(end)==1
    code=code.replace(end,'''
float fogHatchSafeWeight=max(fogHatchWeight,0.00001);
float3 fogHatchRepresentativeLocal=fogHatchPositionSum/fogHatchSafeWeight;
float3 fogHatchRepresentativeWorld=SpatialOrigin+TransformLocalVectorToWorld(Parameters,(fogHatchRepresentativeLocal-0.5)*100.0).xyz;
// One analytic direction at the absorption-weighted representative point;
// this is not the absorption-weighted mean velocity of all ray samples.
float3 fogHatchRepresentativeVelocity=FogHatching.AnimationVelocityApprox(fogHatchRepresentativeWorld,spatialClock,SpatialCloudSize,SpatialWarp,SpatialSpeed);
return FogHatching.Apply(Parameters,color,transmittance,fogHatchRepresentativeWorld,
    fogHatchRepresentativeVelocity,fogHatchLightSum/fogHatchSafeWeight,
    spatialClock,FogHatchStrength,FogHatchSpacing,FogHatchFlowStrength,FogDistanceWeight,FogHatchDebugMode,
    WorldUnitsPerPixel,AnchorRight.xyz,AnchorUp.xyz,FogHatchRegularity);
''')
    declarations='''
float3 fogHatchPositionSum=0;
float fogHatchLightSum=0;
float fogHatchWeight=0;
'''
    return helper_code+'\n'+declarations+'\n'+code


def main():
    assert '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower(), (
        'Rebuild with the live Editor closed in an UnrealEditor-Cmd commandlet')
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([OWN.rstrip('/')],True)
    material=unreal.load_asset(TARGET)
    source=unreal.load_asset(SOURCE)
    assert material and source,'Build owned spatial fog master first'
    helpers=canonical_spatial_helpers()
    shared=(ROOT/'NinjaSpatialDensity.hlsl').read_text(encoding='utf8')
    hatch=(ROOT/'NinjaFogHatching.hlsl').read_text(encoding='utf8')
    source_nodes={n.get_name():n for n in L.get_material_expressions(source)
                  if isinstance(n,unreal.MaterialExpressionCustom)}
    assert set(source_nodes)=={'MaterialExpressionCustom_'+str(i) for i in range(6)}
    nodes=list(L.get_material_expressions(material))
    by_name={n.get_name():n for n in nodes}
    find=helpers['find_parameter']
    controls={}
    for i,(name,value,description) in enumerate(PARAMETERS):
        node=find(nodes,unreal.MaterialExpressionScalarParameter,name)
        if node is None:
            node=L.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-4650,1950+i*150)
            node.set_editor_property('parameter_name',name)
            node.set_editor_property('default_value',value)
        node.set_editor_property('group','05 雾中排线')
        node.set_editor_property('desc',description)
        node.set_editor_property('sort_priority',i)
        if name=='FogHatchRegularity':
            node.set_editor_property('slider_min',0.0)
            node.set_editor_property('slider_max',1.0)
        controls[name]=node
    for i,(name,value,description) in enumerate(ANCHORS):
        node=find(nodes,unreal.MaterialExpressionVectorParameter,name)
        if node is None:
            node=L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-5000,2700+i*160)
            node.set_editor_property('parameter_name',name)
            node.set_editor_property('default_value',unreal.LinearColor(*value))
        node.set_editor_property('group','05 雾中排线')
        node.set_editor_property('desc',description)
        node.set_editor_property('sort_priority',len(PARAMETERS)+i)
        controls[name]=node

    # Reuse the original independent fade expression, not opacity (which would
    # introduce a cycle through the Custom's T). Mirror its static switch so
    # DistanceFade=False yields 1 exactly as in the native final RGB/opacity.
    native_fade=by_name['MaterialExpressionOneMinus_5']
    fade_switch=find(nodes,unreal.MaterialExpressionStaticSwitchParameter,'DistanceFade')
    assert fade_switch
    fade=next((n for n in nodes if n.get_editor_property('desc')=='Fog hatch native distance weight'),None)
    if fade is None:
        fade=L.create_material_expression(material,unreal.MaterialExpressionStaticSwitchParameter,-4250,2300)
        fade.set_editor_property('parameter_name','DistanceFade')
        fade.set_editor_property('default_value',fade_switch.get_editor_property('default_value'))
        fade.set_editor_property('group','XDistanceFade')
        fade.set_editor_property('desc','Fog hatch native distance weight')
    constant=helpers['ensure_auxiliary'](material,nodes,unreal.MaterialExpressionConstant,
                                       'Fog hatch unit distance weight',-4500,2450)
    constant.set_editor_property('r',1.0)
    # UMaterialExpressionStaticSwitchParameter::GetInputName (UE source
    # MaterialExpressions.cpp:8706) exposes True/False, not its A/B fields.
    fade_pins={str(name) for name in L.get_material_expression_input_names(fade)}
    assert {'True','False'}.issubset(fade_pins),fade_pins
    assert L.connect_material_expressions(native_fade,'',fade,'True')
    assert L.connect_material_expressions(constant,'',fade,'False')
    controls['FogDistanceWeight']=fade

    # Current spatial helper inputs are reconnected after code/inputs edits.
    spatial={name:find(nodes,unreal.MaterialExpressionScalarParameter,name)
             for name,_,_ in helpers['PARAMETERS']}
    spatial.update({
        'SpatialOrigin':next(n for n in nodes if n.get_editor_property('desc')=='Spatial field origin'),
        'SpatialClock':next(n for n in nodes if n.get_editor_property('desc')=='Spatial field clock'),
        'SpatialNoise':find(nodes,unreal.MaterialExpressionTextureObjectParameter,'NoiseVolume'),
        'SpatialBrush':find(nodes,unreal.MaterialExpressionTextureObjectParameter,'SpatialBrush'),
        'SpatialDistanceFieldHint':next(n for n in nodes if n.get_editor_property('desc')=='Spatial environment distance-field binding'),
    })
    for name in helpers['ENV_VECTORS']:
        spatial[name]=find(nodes,unreal.MaterialExpressionVectorParameter,name)
    assert all(spatial.values()),'Missing existing spatial inputs'
    links=dict(spatial,**controls)
    report=[]
    for name,native in source_nodes.items():
        custom=by_name[name]
        original=native.get_editor_property('code')
        code=helpers['replacement_code'](original,shared)
        code=hatching_code(code,hatch,'lightvisibility' in original)
        custom.set_editor_property('code',code)
        pins=list(custom.get_editor_property('inputs'))
        names={str(pin.get_editor_property('input_name')) for pin in pins}
        for input_name in links:
            if input_name not in names:
                pin=unreal.CustomInput();pin.set_editor_property('input_name',input_name);pins.append(pin)
        custom.set_editor_property('inputs',pins)
        for input_name,expression in links.items():
            output='RGBA' if input_name in helpers['ENV_VECTORS'] else ''
            assert L.connect_material_expressions(expression,output,custom,input_name),input_name
        current_names=L.get_material_expression_input_names(custom)
        current_sources=L.get_inputs_for_material_expression(material,custom)
        assert all(expression for input_name,expression in zip(current_names,current_sources)
                   if str(input_name) in links),name
        report.append({'branch':name,'original_transmittance_preserved':True,
                       'additional_texture_samples':0,'additional_raymarches':0})
    errors=list(L.recompile_material(material));assert not errors,errors
    assert unreal.EditorAssetLibrary.save_asset(TARGET,False)
    unreal.log('NINJA_FOG_SCREEN_HATCH_BUILT '+json.dumps({
        'saved_master':TARGET,'branches':report,'immediate_material_errors':errors,
        'motion':'first-order density-animation direction at the absorption-weighted representative point; not NS or an averaged velocity',
        'coordinates':'world-anchored fixed reference-camera 2D plane; matches scene ink',
        'regularity':'FogHatchRegularity: default 0 preserves legacy; 1 stabilizes axis and reduces bend/crossing',
        'debug':'on black scene: 1 final opacity, 2 opacity-weighted direction, 3 opacity-weighted lighting',
        'platform_shader_validation':'pending actual rendered viewport',
        'instances_modified':False,'levels_modified':False,'vendor_assets_modified':False,
    },ensure_ascii=False))


main()
