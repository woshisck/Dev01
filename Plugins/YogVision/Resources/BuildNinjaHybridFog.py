"""Build ONLY the owned Ninja 2D-driven hybrid fog master, in a commandlet.

Run UnrealEditor-Cmd <project> -run=pythonscript -script=<this file>
    -AllowCommandletRendering -unattended -nosourcecontrol

Preserves original Ninja six branches, licensing, ray-box/depth setup,
Beer-Lambert and point/directional lighting. Corrects the stopping-time units
for non-uniform container scale. No instances, actors, levels or
vendor assets are modified. The existing BP writes its live RG-signed-velocity /
B-density RT into the ORIGINAL VelocityDensityMap parameter. Continuous-space
mode uses one XYZ noise sample for soft billows and a bounded, rim-weighted
coordinate bend. Existing project fog hatching consumes actual RG.

HybridEnvironmentMask remains the external linear RGBA coefficient atlas for
reference materials and diagnostic tools. HybridEnvironmentVolume contains
identical coefficients, written by the same producer dispatch. Layout is
(XY resolution, Z slice count, columns, ready), default (128,16,4,0).
HybridMaskMin.xyz and HybridMaskInvSize.xyz are world centimetres and reciprocal
world size. Slice height is quadratic, matched by inverse sqrt in the reader.
Ready=0/outside use coefficient 1. The production reader performs one hardware
linear volume lookup instead of two atlas reads and a software Z lerp. No
per-view/light-step GDF queries are used. The original density math is retained.
The unused native GDF binding input declares material relevance for the external
precompute; its sample is intended to be eliminated from the pixel shader.
Verify completed platform compilation and GDF View resources in the real editor.
"""
import json
import re
from pathlib import Path
import unreal


SOURCE = '/Game/FluidNinja/Usecases/Volumetrics/BaseMaterials/Output/M_VolumeSmoke'
OWN = '/Game/Art/Rendering/Fog/Ninja/'
TARGET = OWN+'M_Corridor_FogHybrid'
L = unreal.MaterialEditingLibrary
PARAMETERS = [
    ('HybridAtmosphereAmount',0.,'全空间基础密度；大于0启用连续空间雾，0沿用原Ninja挤出',0.,1.),
    ('HybridAtmosphereVariation',.20,'三维雾团的厚薄对比；保留团块之间的环境底雾',0.,1.),
    ('HybridBoundaryDetail',.35,'清雾交界处云形细节；不改变空间雾的覆盖高度',0.,1.),
    ('HybridBoundaryMotion',1.,'清雾交界的三维翻卷与Ninja动画强度；0静止，1全强度',0.,1.),
    ('HybridNoiseAmount',.18,'少量三维纵深细节；0 原二维密度挤出',0.,1.),
    ('HybridNoiseSize',1400.,'三维雾团世界尺寸（厘米）；不改变容器高度',100.,5000.),
    ('HybridNoiseFlow',.15,'雾团弯曲与交界翻卷幅度；使用Ninja速度方向辅助扰动',0.,2.),
    ('HybridNoiseSpeed',.08,'清雾交界的翻卷速度；远处主体保持世界锚定',0.,1.),
    ('HybridTimeScale',1.,'细节与排线时钟；0 冻结、1 播放',0.,2.),
    ('HybridTimeOffset',0.,'细节与排线冻结时刻（秒）',0.,600.),
    ('HybridShadowSamples',6.,'独立自阴影采样数；总覆盖由Reach保持',1.,20.),
    ('HybridShadowReach',1.,'自阴影总覆盖相对原Ninja20步；1 原长度',.25,2.),
    ('FogHatchStrength',.45,'原雾内排线强度；只修饰RGB不改T',0.,2.),
    ('FogHatchSpacing',9.,'原雾排线参考像素间距',2.5,24.),
    ('FogHatchFlowStrength',1.,'排线跟随输入RT的真实RG方向；非世界速度量测',0.,2.),
    ('FogHatchRegularity',0.,'原排线规则度；0 保留当前风格',0.,1.),
    ('FogHatchDebugMode',0.,'0 成图；1 实际雾遮罩；2 RT流向；3 受光',0.,3.),
    ('WorldUnitsPerPixel',2.,'世界锚定排线的参考像素尺寸（厘米）',.1,8.),
]
VECTORS = [
    ('HybridMaskMin',(0.,0.,0.,0.),'遮罩世界最小点；Actor驱动'),
    ('HybridMaskInvSize',(0.,0.,0.,0.),'遮罩世界尺寸的倒数；Actor驱动'),
    ('HybridMaskLayout',(128.,16.,4.,0.),'XY分辨率/Z层数/列数/Ready；Actor驱动'),
    ('AnchorRight',(.6870875,.7265747,0.,0.),'原世界排线参考向右'),
    ('AnchorUp',(.4167488,-.3941015,.819152,0.),'原世界排线参考向上'),
]
SAMPLE_TAIL = (
    'HybridClock*HybridTimeScale+HybridTimeOffset,DensityPow,Extrude,ExtrudeSoften,'
    'DensityOffset,BaseDensity,FadeFalloff,HYBRID_Z_BIAS,HybridNoiseAmount,'
    'HybridNoiseSize,HybridNoiseFlow,HybridNoiseSpeed,'
    'HybridAtmosphereAmount,HybridAtmosphereVariation,HybridBoundaryDetail,HybridBoundaryMotion,'
    'HybridMaskMin,HybridMaskInvSize,HybridMaskLayout'
)
SAMPLE_RESOURCES = (
    'HybridOrigin,HybridDensityMap,HybridDensityMapSampler,HybridNoise,HybridNoiseSampler,'
    'HybridEnvironmentMask,HybridEnvironmentMaskSampler,'
    'HybridEnvironmentVolume,HybridEnvironmentVolumeSampler,'
)


def replacement_code(native, shared, hatch):
    """Only density/quality/hatching change; native integration stays intact."""
    assert '//LICENSING' in native and native.count('float curtexture =')==1
    lit = 'float curltexture =' in native
    directional = 'LightRay-=LightDirection;' in native
    # Native point-no-flow has no -.01 Z adjustment; retain that branch detail.
    z_bias = '0.0' if ('single Point Light - module' in native) else '0.01'
    tail = SAMPLE_TAIL.replace('HYBRID_Z_BIAS',z_bias)
    view = ('float4 hybridView=HybridField.Sample(Parameters,RayOrigin,'+SAMPLE_RESOURCES+tail+');\n'
            'float curtexture=hybridView.y;\nfloat cursample=hybridView.x;\n')
    start=native.index('float curtexture =')
    end=native.index('if(cursample>.001)',start)
    code=native[:start]+view+native[end:]
    # Native lit branches also clip OUTSIDE the density function. A continuous
    # medium must reach the full analytic box, irrespective of cloud extrusion.
    # Keep the original intervals exactly when the atmosphere option is zero.
    cloud_top='.5+Extrude+ExtrudeSoften+.01'
    cloud_bottom='.5-Extrude-ExtrudeSoften-.01'
    code=code.replace('RayOrigin.z<'+cloud_top,
                      'RayOrigin.z<(HybridAtmosphereAmount>0.0001?1.0:'+cloud_top+')')
    code=code.replace('RayOrigin.z>'+cloud_bottom,
                      'RayOrigin.z>(HybridAtmosphereAmount>0.0001?0.0:'+cloud_bottom+')')
    if lit:
        start=code.index('float curltexture =')
        end=code.index('lightvisibility*=',start)
        light=('float4 hybridLight=HybridField.Sample(Parameters,LightRay,'+SAMPLE_RESOURCES+tail+');\n'
               'float curlsample=hybridLight.x;\n')
        code=code[:start]+light+code[end:]
        code=code.replace('LightRay.z >'+cloud_top,
                          'LightRay.z >(HybridAtmosphereAmount>0.0001?1.0:'+cloud_top+')')
        code=code.replace('LightRay.z <'+cloud_bottom,
                          'LightRay.z <(HybridAtmosphereAmount>0.0001?0.0:'+cloud_bottom+')')
        code,count=re.subn(r'for\s*\(int l=0;20>l;l\+\+\)',
                           'for(int l=0;hybridLightBudget>l;l++)',code)
        assert count==1
        assert code.count('ShadowStepSize')==(1 if directional else 2)
        if directional:
            code=code.replace('LightRay-=LightDirection;',
                              'LightRay-=LightDirection*hybridShadowRatio;')
        code=code.replace('ShadowStepSize','hybridShadowStep')
    absorption='float absorptionfrommarch =previoustransmittance-transmittance;'
    assert code.count(absorption)==1
    code=code.replace(absorption,absorption+'''
hybridPositionSum+=absorptionfrommarch*RayOrigin;
hybridFlowSum+=absorptionfrommarch*hybridView.zw;
hybridWeight+=absorptionfrommarch;
''')
    color='color+=absorptionfrommarch'
    assert code.count(color)==1
    code=code.replace(color,'hybridLightSum+=absorptionfrommarch*'
                      +('saturate(lightvisibility)' if lit else '1.0')+';\n'+color)
    output='return float4(color,transmittance);'
    assert code.count(output)==1
    code=code.replace(output,'''
float hybridSafeWeight=max(hybridWeight,0.00001);
float3 hybridRepresentativeLocal=hybridPositionSum/hybridSafeWeight;
float3 hybridRepresentativeWorld=HybridOrigin+TransformLocalVectorToWorld(Parameters,(hybridRepresentativeLocal-0.5)*100.0).xyz;
// The RG values are local input-UV velocity proxies, not measured centimetres/s.
// Transform their XY direction to world axes for the existing anchored marks.
float2 hybridMeanFlow=hybridFlowSum/hybridSafeWeight;
float3 hybridWorldFlow=TransformLocalVectorToWorld(Parameters,float3(hybridMeanFlow,0.0)*100.0).xyz;
return FogHatching.Apply(Parameters,color,transmittance,hybridRepresentativeWorld,
    hybridWorldFlow,hybridLightSum/hybridSafeWeight,
    HybridClock*HybridTimeScale+HybridTimeOffset,FogHatchStrength,FogHatchSpacing,
    FogHatchFlowStrength,FogDistanceWeight,FogHatchDebugMode,
    WorldUnitsPerPixel,AnchorRight.xyz,AnchorUp.xyz,FogHatchRegularity);
''')
    stop=re.search(r'if\s*\(\s*t\s*>\s*(\w+)\s*\)\s*break',native)
    assert stop, 'Native scene-depth stopping distance must be identifiable'
    # MF_RayBox returns LOCAL segment length / 100. Its ray direction retains
    # ScaleX * WorldToLocal(CameraVector), so non-uniform scale changes |R|.
    # Keep the native world stepping/extinction and convert its stopping time.
    setup=('// YOG_NINJA_NONUNIFORM_DEPTH_CLIP\n'
           +stop.group(1)+'/=max(length(RayDirection),0.00001);\n')+'''
float3 hybridPositionSum=0;
float2 hybridFlowSum=0;
float hybridWeight=0;
float hybridLightSum=0;
'''
    if lit:
        setup+='''
int hybridLightBudget=(int)clamp(floor(HybridShadowSamples+0.5),1.0,20.0);
float hybridShadowRatio=20.0/(float)hybridLightBudget*max(HybridShadowReach,0.05);
float hybridShadowStep=ShadowStepSize*hybridShadowRatio;
'''
    return shared+'\n'+hatch+'\n'+setup+'\n'+code


def parameter(nodes, expression_type, name):
    return next((n for n in nodes if isinstance(n,expression_type)
                 and str(n.get_editor_property('parameter_name'))==name),None)


def auxiliary(material, nodes, expression_type, desc, x, y):
    n=next((n for n in nodes if isinstance(n,expression_type)
            and n.get_editor_property('desc')==desc),None)
    if n is None:
        n=L.create_material_expression(material,expression_type,x,y)
        n.set_editor_property('desc',desc)
    return n


def main():
    assert '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower(), (
        'Commandlet only: do not rebuild a rendered material in the live editor')
    root=Path(__file__).parent
    shared=(root/'NinjaHybridDensity.hlsl').read_text(encoding='utf8')
    hatch=(root/'NinjaFogHatching.hlsl').read_text(encoding='utf8')
    assert 'GetDistanceToNearestSurfaceGlobal' not in shared
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([OWN.rstrip('/')],True)
    source=unreal.load_asset(SOURCE)
    assert source
    natives={n.get_name():n for n in L.get_material_expressions(source)
             if isinstance(n,unreal.MaterialExpressionCustom)}
    assert set(natives)=={'MaterialExpressionCustom_'+str(i) for i in range(6)}
    codes={name:replacement_code(n.get_editor_property('code'),shared,hatch)
           for name,n in natives.items()}
    material=unreal.load_asset(TARGET)
    if material is None:
        material=unreal.EditorAssetLibrary.duplicate_asset(SOURCE,TARGET)
    assert material and material.get_path_name().split('.')[0]==TARGET
    nodes=list(L.get_material_expressions(material))
    controls={}
    for i,(name,value,desc,minimum,maximum) in enumerate(PARAMETERS):
        node=parameter(nodes,unreal.MaterialExpressionScalarParameter,name)
        if node is None:
            node=L.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-4300,i*150)
            node.set_editor_property('parameter_name',name)
            node.set_editor_property('default_value',value)
        node.set_editor_property('group','02 雾内排线' if name.startswith('Fog') or name=='WorldUnitsPerPixel'
                                else '00 空间雾体' if name.startswith('HybridAtmosphere') or name.startswith('HybridBoundary') or name in ('HybridNoiseSize','HybridNoiseFlow','HybridNoiseSpeed')
                                else '01 混合雾质量')
        node.set_editor_property('desc',desc)
        node.set_editor_property('slider_min',minimum)
        node.set_editor_property('slider_max',maximum)
        controls[name]=node
    for i,(name,value,desc) in enumerate(VECTORS):
        node=parameter(nodes,unreal.MaterialExpressionVectorParameter,name)
        if node is None:
            node=L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-4800,2100+i*150)
            node.set_editor_property('parameter_name',name)
            node.set_editor_property('default_value',unreal.LinearColor(*value))
        node.set_editor_property('group','00 环境遮罩（Actor驱动）' if name.startswith('Hybrid') else '02 雾内排线')
        node.set_editor_property('desc',desc)
        controls[name]=node
    original_density=parameter(nodes,unreal.MaterialExpressionTextureObjectParameter,'VelocityDensityMap')
    original_noise=parameter(nodes,unreal.MaterialExpressionTextureObjectParameter,'NoiseVolume')
    assert original_density and original_noise
    mask=parameter(nodes,unreal.MaterialExpressionTextureObjectParameter,'HybridEnvironmentMask')
    if mask is None:
        mask=L.create_material_expression(material,unreal.MaterialExpressionTextureObjectParameter,-4800,3000)
        mask.set_editor_property('parameter_name','HybridEnvironmentMask')
        # Ready=0 bypasses mask reads. Reuse the original linear float RT as a
        # type-correct placeholder, replaced by the actor's coefficient atlas.
        mask.set_editor_property('texture',original_density.get_editor_property('texture'))
        mask.set_editor_property('sampler_type',original_density.get_editor_property('sampler_type'))
    assert mask.get_editor_property('texture')
    mask.set_editor_property('group','00 环境遮罩（Actor驱动）')
    mask_volume=parameter(nodes,unreal.MaterialExpressionTextureObjectParameter,'HybridEnvironmentVolume')
    if mask_volume is None:
        mask_volume=L.create_material_expression(material,unreal.MaterialExpressionTextureObjectParameter,-4800,3150)
        mask_volume.set_editor_property('parameter_name','HybridEnvironmentVolume')
        # The existing native volume-noise texture is a type-correct placeholder.
        # Ready=0 bypasses the read until the actor binds its linear RGBA16F RT.
        mask_volume.set_editor_property('texture',original_noise.get_editor_property('texture'))
        mask_volume.set_editor_property('sampler_type',original_noise.get_editor_property('sampler_type'))
    assert mask_volume.get_editor_property('texture')
    mask_volume.set_editor_property('group','00 环境遮罩（Actor驱动）')
    actor_origin=auxiliary(material,nodes,unreal.MaterialExpressionActorPositionWS,'Hybrid field origin',-4800,3200)
    origin=auxiliary(material,nodes,unreal.MaterialExpressionAdd,'Hybrid field offset origin',-4500,3200)
    position_offset=parameter(nodes,unreal.MaterialExpressionVectorParameter,'PositionOffset')
    assert position_offset
    assert L.connect_material_expressions(actor_origin,'',origin,'A')
    assert L.connect_material_expressions(position_offset,'RGB',origin,'B')
    clock=auxiliary(material,nodes,unreal.MaterialExpressionTime,'Hybrid field clock',-4800,3350)
    # Legacy Custom compilation visits named inputs before HLSL optimisation.
    # Intentionally unused in code: declares bUsesGlobalDistanceField for the
    # external compute but must not sample GDF once for every rendered pixel.
    hint=auxiliary(material,nodes,unreal.MaterialExpressionDistanceToNearestSurface,'Hybrid external mask GDF binding',-4800,3500)
    native_fade=next(n for n in nodes if n.get_name()=='MaterialExpressionOneMinus_5')
    original_switch=parameter(nodes,unreal.MaterialExpressionStaticSwitchParameter,'DistanceFade')
    assert original_switch
    fade=auxiliary(material,nodes,unreal.MaterialExpressionStaticSwitchParameter,'Hybrid hatch native distance weight',-4500,3650)
    fade.set_editor_property('parameter_name','DistanceFade')
    fade.set_editor_property('default_value',original_switch.get_editor_property('default_value'))
    unit=auxiliary(material,nodes,unreal.MaterialExpressionConstant,'Hybrid hatch unit distance weight',-4800,3800)
    unit.set_editor_property('r',1.)
    assert L.connect_material_expressions(native_fade,'',fade,'True')
    assert L.connect_material_expressions(unit,'',fade,'False')
    links=dict(controls,HybridDensityMap=original_density,HybridNoise=original_noise,
               HybridEnvironmentMask=mask,HybridEnvironmentVolume=mask_volume,HybridOrigin=origin,HybridClock=clock,
               HybridGDFBindingHint=hint,FogDistanceWeight=fade)
    vector_names={n for n,_,_ in VECTORS}
    customs={n.get_name():n for n in nodes if isinstance(n,unreal.MaterialExpressionCustom)
             and n.get_name() in codes}
    assert len(customs)==6
    for name,custom in customs.items():
        custom.set_editor_property('code',codes[name])
        pins=list(custom.get_editor_property('inputs'))
        names={str(p.get_editor_property('input_name')) for p in pins}
        for input_name in links:
            if input_name not in names:
                pin=unreal.CustomInput();pin.set_editor_property('input_name',input_name);pins.append(pin)
        custom.set_editor_property('inputs',pins)
        for input_name,node in links.items():
            assert L.connect_material_expressions(node,'RGBA' if input_name in vector_names else '',custom,input_name),input_name
        current=L.get_material_expression_input_names(custom)
        expressions=L.get_inputs_for_material_expression(material,custom)
        assert len(current)==len(expressions)
        actual={str(k):v for k,v in zip(current,expressions)}
        assert all(actual.get(k) for k in links),name
    view_steps=parameter(nodes,unreal.MaterialExpressionScalarParameter,'MaxViewSteps')
    assert view_steps
    view_steps.set_editor_property('default_value',32.)
    errors=list(L.recompile_material(material));assert not errors,errors
    assert unreal.EditorAssetLibrary.save_asset(TARGET,False)
    unreal.log('NINJA_HYBRID_MASTER_BUILT '+json.dumps({
        'target':TARGET,'native_source_preserved':SOURCE,'branches':len(customs),
        'additional_instances_or_levels_modified':False,'immediate_errors':errors,
        'density':'continuous atmosphere with soft XYZ billows and rim-weighted 3D coordinate rolling; Ninja B supplies bounded rim detail; amount0 preserves native extrusion',
        'velocity':'live Ninja RT signed RG; no second decode; ambient branch weights motion and hatch flow by actual cached rim',
        'environment':'same-dispatch RGBA16F atlas and volume coefficients; quadratic Z, one hardware-linear volume lookup; no step GDF query',
        'quality':'master 32 view / 6 shadow defaults; original total light reach preserved',
        'gdf_hint':'unused named native input declares relevance; verify pixel DCE and actual View resource',
        'platform_compile_validation':'pending actual editor log and captures',
    },ensure_ascii=False))


if __name__=='__main__':
    main()
