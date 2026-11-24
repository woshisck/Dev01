"""Build the owned unbound global fog post-process; run in a commandlet.

Creates /YogVision/Global/M_NinjaGlobalFog and MI_NinjaGlobalFog only. Reads
the accepted corridor MIC for initial palette/quality defaults, without editing
that material, the Ninja vendor assets, a level, or any existing fog instance.
Runtime input/caches and all artist controls are supplied by the global Actor.
"""
import hashlib
import json
from pathlib import Path
import unreal

L=unreal.MaterialEditingLibrary
DEST='/YogVision/Global'
MASTER=DEST+'/M_NinjaGlobalFog'
INSTANCE=DEST+'/MI_NinjaGlobalFog'
ACCEPTED='/Game/Art/Rendering/Fog/Ninja/MI_Corridor_HybridMist'
NOISE='/Game/FluidNinja/Usecases/Volumetrics/VolumeNoises/VolumeNoise2'

# Actor uses the existing native/Hybrid parameter names, with only global
# rendering geometry added. Groups are shared with its Chinese Details view.
SCALARS=[
 ('GlobalFogEnabled',1.,'00 全局雾','全局雾开关；Actor 驱动',0.,1.),
 ('Density',5.,'00 全局雾','消光密度；保留已验收的 Ninja 参考尺度',0.,20.),
 ('GlobalFogHeight',450.,'00 全局雾','高度衰减尺度：每升高此距离密度系数减半，无固定雾顶（厘米）',50.,50000.),
 ('GlobalMaxDistance',10000.,'00 全局雾','最大视线积分距离（厘米），不是局部体积范围',100.,100000.),
 ('GlobalStartDistance',0.,'00 全局雾','镜头前的开始距离（厘米）',0.,10000.),
 ('GlobalNearFade',300.,'00 全局雾','镜头前渐入宽度（厘米）',1.,10000.),
 ('HybridAtmosphereAmount',.32,'01 云团与流动','全空间基础雾量',0.,2.),
 ('HybridAtmosphereVariation',.78,'01 云团与流动','三维云团的厚薄对比',0.,1.),
 ('HybridNoiseSize',1000.,'01 云团与流动','世界空间云团尺寸（厘米）',50.,10000.),
 ('HybridNoiseFlow',.45,'01 云团与流动','云团弯曲与局部翻卷幅度',0.,2.),
 ('HybridNoiseSpeed',.30,'01 云团与流动','玩家清雾交界的翻卷速度',0.,2.),
 ('HybridBoundaryDetail',.8,'01 云团与流动','清雾交界处的 Ninja 动画细节',0.,1.),
 ('HybridBoundaryMotion',1.,'01 云团与流动','清雾交界处的翻卷强度',0.,1.),
 ('LightIntensity',1.8,'02 体积光影','雾的艺术主光强度',0.,10.),
 ('LightAttenuationRadius',5500.,'02 体积光影','可选点光源的雾照明范围（厘米）',1.,100000.),
 ('Shadow',1.8,'02 体积光影','云团自身的阴影消光强度',0.,10.),
 ('MaxViewSteps',32.,'03 质量','每条视线的采样数',8.,128.),
 ('HybridShadowSamples',6.,'03 质量','每个有效雾样点的自阴影采样数',1.,20.),
 ('HybridShadowReach',1.,'03 质量','自阴影总覆盖相对原 Ninja 的比例',.05,4.),
 ('ShadowStepSize',.01,'03 质量','原 Ninja 阴影参考步长',.0001,.1),
 ('DensityReferenceSize',4375.,'03 质量','Ninja 消光参考世界尺寸（厘米）；独立于相机和覆盖',50.,20000.),
 ('GlobalInputWorldSize',4375.,'04 输入贴图','Ninja 密度/速度输入的世界平铺跨度（厘米）',50.,20000.),
 ('GlobalInputRotation',90.,'04 输入贴图','输入贴图的固定世界旋转（度）',-180.,180.),
 ('DensityPow',1.,'04 输入贴图','原 Ninja 输入密度指数',.01,8.),
 ('ColorPow',1.,'05 色板','原 Ninja 色板指数',.01,8.),
 ('ColorMul',1.,'05 色板','原 Ninja 色板倍率',0.,8.),
 ('ColorAdd',0.,'05 色板','原 Ninja 色板偏移',-2.,2.),
 ('FogHatchStrength',.3,'06 雾内排线','雾的局部排线强度',0.,2.),
 ('FogHatchSpacing',9.,'06 雾内排线','雾内排线间距',2.5,24.),
 ('FogHatchFlowStrength',1.,'06 雾内排线','排线跟随真实输入 RG 流向的权重',0.,2.),
 ('FogHatchRegularity',.6,'06 雾内排线','排线的整齐程度',0.,1.),
 ('WorldUnitsPerPixel',2.,'06 雾内排线','世界锚定笔触的参考像素尺寸（厘米）',.1,8.),
 ('FogHatchDebugMode',0.,'07 调试','0 成图；1 实际雾遮罩；2 雾流向；3 雾受光',0.,3.),
 ('HybridTimeScale',1.,'07 调试','动画时间倍率；0 冻结，1 正常',0.,2.),
 ('HybridTimeOffset',0.,'07 调试','动画时间偏移（秒）',0.,600.),
 ('SpatialEnvironmentEnabled',1.,'08 环境（Actor驱动）','环境系数开关',0.,1.),
 ('EnvGroundHeight',0.,'08 环境（Actor驱动）','地面世界高度',-1000000.,1000000.),
 ('EnvGroundBodyHeight',160.,'08 环境（Actor驱动）','贴地浓雾参考高度',1.,10000.),
 ('EnvGroundAmount',2.5,'08 环境（Actor驱动）','贴地雾密度乘数',0.,20.),
 ('EnvUpperAmount',.85,'08 环境（Actor驱动）','上层雾密度乘数',0.,20.),
 ('EnvPlayerStrength',.72,'08 环境（Actor驱动）','玩家清雾强度',0.,1.),
 ('EnvPlayerFeather',550.,'08 环境（Actor驱动）','玩家清雾羽化宽度',1.,10000.),
 ('EnvPlayerHeight',650.,'08 环境（Actor驱动）','玩家清雾高度',1.,10000.),
 ('EnvLightStrength',.6,'08 环境（Actor驱动）','灯光清雾强度',0.,1.),
 ('EnvLightFeather',500.,'08 环境（Actor驱动）','灯光清雾羽化宽度',1.,10000.),
 ('EnvSpatialHeightEnabled',0.,'08 环境（Actor驱动）','可选局部雾高压缩开关',0.,1.),
 ('EnvNearFogHeight',35.,'08 环境（Actor驱动）','可选清雾中心雾高',1.,10000.),
 ('EnvFarFogHeight',350.,'08 环境（Actor驱动）','可选远处雾高',1.,10000.),
 ('EnvHeightFeather',100.,'08 环境（Actor驱动）','可选局部雾高羽化',1.,5000.),
]
VECTORS=[
 ('GlobalInputOrigin',(0.,0.,0.,0.),'04 输入贴图','固定世界输入锚点；不随相机滑动'),
 ('Color1',(.025,.04,.052,1.),'05 色板','原 Ninja 深色板'),
 ('Color2',(.11,.15,.17,1.),'05 色板','原 Ninja 浅色板'),
 ('PointLightColor',(.65,.85,.95,1.),'02 体积光影','原 Ninja 主光色板'),
 ('GlobalLightPosition',(0.,0.,1000.,-1.),'02 体积光影','xyz 世界点光位置；w>0 点光，w<=0 使用方向光'),
 ('GlobalLightDirection',(.25,-.35,.9,0.),'02 体积光影','从雾指向艺术主光的世界方向'),
 ('AnchorRight',(.6870875,.7265747,0.,0.),'06 雾内排线','固定世界笔触参考向右'),
 ('AnchorUp',(.4167488,-.3941015,.819152,0.),'06 雾内排线','固定世界笔触参考向上'),
 ('HybridMaskMin',(0.,0.,0.,0.),'08 环境（Actor驱动）','缓存世界最小点'),
 ('HybridMaskInvSize',(0.,0.,0.,0.),'08 环境（Actor驱动）','缓存世界尺寸倒数'),
 ('HybridMaskLayout',(128.,16.,4.,0.),'08 环境（Actor驱动）','平面分辨率/高度层数/图集列数/Ready'),
 ('EnvObserver',(0.,0.,0.,0.),'08 环境（Actor驱动）','玩家世界位置与清雾半径'),
]+[(f'EnvLight{i}',(0.,0.,0.,0.),'08 环境（Actor驱动）','有效清雾灯位置与半径') for i in range(4)] \
 +[(f'EnvLightDirection{i}',(0.,0.,0.,-1.),'08 环境（Actor驱动）','清雾灯世界方向与聚光锥余弦') for i in range(4)]


def parameters(material):
    if material is None:return {'scalars':{},'vectors':{},'textures':{}}
    return {
        'scalars':{str(n):L.get_material_instance_scalar_parameter_value(material,n) for n in L.get_scalar_parameter_names(material)},
        'vectors':{str(n):L.get_material_instance_vector_parameter_value(material,n) for n in L.get_vector_parameter_names(material)},
        'textures':{str(n):L.get_material_instance_texture_parameter_value(material,n) for n in L.get_texture_parameter_names(material)},
    }


def main():
    assert '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower(),'Commandlet only; close rendering editor first'
    root=Path(__file__).parent
    source=(root/'NinjaGlobalFog.hlsl').read_text('utf8')
    hatch=(root/'NinjaFogHatching.hlsl').read_text('utf8')
    assert 'TransformLocalVectorToWorld' not in source
    assert 'GetDistanceToNearestSurfaceGlobal' not in source
    for expression in ('field.Sample(world','field.Sample(lightWorld','scene*fog.a','View.PreExposure'):
        assert expression in source,expression
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([DEST,'/Game/Art/Rendering/Fog/Ninja','/Game/FluidNinja/Usecases/Volumetrics'],True)
    accepted=unreal.load_asset(ACCEPTED)
    assert isinstance(accepted,unreal.MaterialInstanceConstant),'Accepted source MIC missing'
    initial=parameters(accepted)
    noise=unreal.load_asset(NOISE)
    assert isinstance(noise,unreal.VolumeTexture),NOISE
    input_texture=initial['textures'].get('VelocityDensityMap')
    if input_texture is None:
        input_texture=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
    assert input_texture is not None
    protected_root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))/'Art/Rendering/Fog/Ninja'
    protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected_root.glob('*') if p.suffix.lower() in {'.uasset','.umap'}}
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    material=unreal.load_asset(MASTER) if unreal.EditorAssetLibrary.does_asset_exist(MASTER) else None
    instance=unreal.load_asset(INSTANCE) if unreal.EditorAssetLibrary.does_asset_exist(INSTANCE) else None
    assert instance is None or instance.parent==material,'Global instance has an unexpected parent'
    preserved=parameters(instance) if instance is not None else initial
    created=[]
    if material is None:
        material=tools.create_asset('M_NinjaGlobalFog',DEST,unreal.Material,unreal.MaterialFactoryNew())
        created.append(MASTER)
    assert material and material.get_path_name().split('.')[0]==MASTER
    material.set_editor_property('material_domain',unreal.MaterialDomain.MD_POST_PROCESS)
    # Accumulate the stochastic march in TAA/TSR while keeping the fog after
    # opaque DOF. The sharp structural finish remains in BeforeBloom.
    material.set_editor_property('blendable_location',unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_DOF)
    material.set_editor_property('blendable_priority',60)
    material.set_editor_property('disable_pre_exposure_scale',True)
    L.delete_all_material_expressions(material)
    custom=L.create_material_expression(material,unreal.MaterialExpressionCustom,0,0)
    custom.set_editor_property('description','全局空间雾 / 无体积代理 / AfterDOF・BeforeTAA')
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property('code',hatch+'\n'+source)
    links={}
    for i,(name,scene_id) in enumerate([('SceneColor',unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0),('SceneDepth',unreal.SceneTextureId.PPI_SCENE_DEPTH)]):
        node=L.create_material_expression(material,unreal.MaterialExpressionSceneTexture,-1000,i*160)
        node.set_editor_property('scene_texture_id',scene_id)
        if name=='SceneDepth':
            mask=L.create_material_expression(material,unreal.MaterialExpressionComponentMask,-700,i*160)
            for component in ('r','g','b','a'):mask.set_editor_property(component,component=='r')
            assert L.connect_material_expressions(node,'Color',mask,'')
            links[name]=(mask,'')
        else:links[name]=(node,'Color')
    for i,(name,cls) in enumerate([
        ('WorldPosition',unreal.MaterialExpressionWorldPosition),
        ('CameraPosition',unreal.MaterialExpressionCameraPositionWS),
        ('CameraVector',unreal.MaterialExpressionCameraVectorWS),
        ('GlobalClock',unreal.MaterialExpressionTime),
    ]):
        links[name]=(L.create_material_expression(material,cls,-1000,450+i*160),'')
    # An intentionally unused native input declares GDF relevance for the
    # external mask producer; it must be DCE'd from the actual pixel shader.
    gdf=L.create_material_expression(material,unreal.MaterialExpressionDistanceToNearestSurface,-1000,1100)
    # Leave its optional world-position input unconnected, matching the
    # already validated Hybrid builder. This node only declares material
    # relevance; the custom code never reads HybridGDFBindingHint. The Python
    # connector's friendly input label is branch-dependent (not "Position").
    gdf.set_editor_property('desc','Global external mask GDF binding (unused named input)')
    links['HybridGDFBindingHint']=(gdf,'')
    for name,texture,sampler in [('GlobalInput',input_texture,unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
        ('HybridNoise',noise,unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
        ('HybridEnvironmentVolume',noise,unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)]:
        node=L.create_material_expression(material,unreal.MaterialExpressionTextureObjectParameter,-1800,len(links)*160)
        node.set_editor_property('parameter_name',name);node.set_editor_property('texture',texture)
        node.set_editor_property('sampler_type',sampler)
        node.set_editor_property('group','04 输入贴图' if name!='HybridEnvironmentVolume' else '08 环境（Actor驱动）')
        links[name]=(node,'')
    for i,(name,value,group,desc,minimum,maximum) in enumerate(SCALARS):
        node=L.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-2200,i*150)
        node.set_editor_property('parameter_name',name);node.set_editor_property('default_value',value)
        node.set_editor_property('group',group);node.set_editor_property('desc',desc)
        node.set_editor_property('sort_priority',i)
        node.set_editor_property('slider_min',minimum);node.set_editor_property('slider_max',maximum)
        links[name]=(node,'')
    for i,(name,value,group,desc) in enumerate(VECTORS):
        node=L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-2600,i*150)
        node.set_editor_property('parameter_name',name);node.set_editor_property('default_value',unreal.LinearColor(*value))
        node.set_editor_property('group',group);node.set_editor_property('desc',desc)
        node.set_editor_property('sort_priority',i)
        links[name]=(node,'RGBA')
    pins=[]
    for name in links:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    custom.set_editor_property('inputs',pins)
    for name,(node,output) in links.items(): assert L.connect_material_expressions(node,output,custom,name),name
    assert all(L.get_inputs_for_material_expression(material,custom))
    assert L.connect_material_property(custom,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    errors=list(L.recompile_material(material));assert not errors,errors
    assert unreal.EditorAssetLibrary.save_asset(MASTER,False)
    if instance is None:
        instance=tools.create_asset('MI_NinjaGlobalFog',DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        assert instance
        L.set_material_instance_parent(instance,material);created.append(INSTANCE)
    scalar_names={str(n) for n in L.get_scalar_parameter_names(instance)}
    vector_names={str(n) for n in L.get_vector_parameter_names(instance)}
    texture_names={str(n) for n in L.get_texture_parameter_names(instance)}
    # First build reads only matching artist controls from the accepted MIC;
    # subsequent rebuilds retain every authored global instance override.
    for name,value in preserved['scalars'].items():
        if name in scalar_names:L.set_material_instance_scalar_parameter_value(instance,name,value)
    for name,value in preserved['vectors'].items():
        if name in vector_names:L.set_material_instance_vector_parameter_value(instance,name,value)
    for name,value in preserved['textures'].items():
        if name in texture_names and value is not None:L.set_material_instance_texture_parameter_value(instance,name,value)
    L.update_material_instance(instance)
    assert unreal.EditorAssetLibrary.save_asset(INSTANCE,False)
    for path,digest in protected.items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest,('Protected asset changed',path)
    report={'status':'saved','master':MASTER,'instance':INSTANCE,'created_assets':created,
        'accepted_source_read_only':ACCEPTED,'protected_hashes':protected,
        'immediate_errors':errors,'scalar_parameters':[n for n,*_ in SCALARS],
        'vector_parameters':[n for n,*_ in VECTORS],'texture_parameters':['GlobalInput','HybridNoise','HybridEnvironmentVolume'],
        'placement':'unbound PostProcessComponent; no mesh or box required',
        'blendable_location':'AfterDOF (before TAA/TSR)','priority':60,'pre_exposure':'scene * transmittance + scatter * View.PreExposure',
        'validation_pending':['Rendered platform material compile','Editor/PIE global coverage','Scene depth occlusion and player/light clearing']}
    out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Rendering/Fog/NinjaSmokeExact/Global'
    out.mkdir(parents=True,exist_ok=True)
    (out/'GlobalMaterialBuild.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
    unreal.log('NINJA_GLOBAL_FOG_BUILT '+json.dumps(report,ensure_ascii=False))


if __name__=='__main__':main()
