"""Build owned BeforeBloom structure and overlapping textured pencil hatching.

Run with UnrealEditor-Cmd -run=pythonscript -AllowCommandletRendering, with
the live Editor closed. Saves only M_Corridor_FogFinish, its instance and the
owned T_Corridor_PeripheralHatchAtlas imported from T_PeripheralHatchAtlas.png.
Never loads/edits a level or a vendor/original artist asset.
Platform shader compilation and final appearance still require actual
rendering-editor validation; immediate material errors alone are insufficient.
"""
import json
from pathlib import Path
import unreal

L=unreal.MaterialEditingLibrary
OWN='/Game/Art/Rendering/Fog/Ninja/'
MASTER=OWN+'M_Corridor_FogFinish'
INSTANCE=OWN+'MI_Corridor_FogFinish'
ATLAS=OWN+'T_Corridor_PeripheralHatchAtlas'
ATLAS_SOURCE=Path(__file__).with_name('T_PeripheralHatchAtlas.png')
SCALARS=[
    ('OutlineStrength',0.50,'00 透雾结构描边','整体结构线强度'),
    ('EdgeWidth',1.0,'00 透雾结构描边','结构线宽度（输出像素）'),
    ('NormalStrength',0.25,'00 透雾结构描边','仅大法线转折的描边强度；0 只保留深度结构'),
    ('FarFogBlend',0.15,'01 远景配色','远景艺术配色混合；轻度补充体积雾，0 关闭'),
    ('FarStart',2200.0,'01 远景配色','远景配色起点（厘米）'),
    ('FarFeather',2000.0,'01 远景配色','远景配色过渡（厘米）'),
    ('OuterBrushStrength',0.75,'02 外围暗部笔触','外围暗部粗笔触强度；0 关闭'),
    ('OuterBrushWidth',1.9,'02 外围暗部笔触','图集铅笔笔触的灰度压力；高值保留更浅的线条、增加粗细'),
    ('OuterBrushSpacing',11.0,'02 外围暗部笔触','整簇图集笔触尺度；单根排线的形状和间距来自源图'),
    ('OuterBrushDensity',0.56,'02 外围暗部笔触','提高实际笔簇覆盖、灰阶保留与第二方向交叠；0 关闭'),
    ('OuterBrushStart',0.58,'02 外围暗部笔触','外围笔触起点；高值更远离画面中心'),
    ('OuterBrushDarkThreshold',0.10,'02 外围暗部笔触','暗部亮度阈值；高值允许笔触覆盖稍亮区域'),
    ('WorldUnitsPerPixel',2.0,'03 固定世界锚定','共享参考画面像素对应的世界厘米；不随镜头滑动'),
    ('DebugMode',0.0,'04 调试','0 成图；1 结构描边区域；2 外围粗笔触遮罩'),
]
ANCHORS=[
    ('AnchorRight',(0.6870875,0.7265747,0.0,0.0),'共享固定参考画面向右的世界方向'),
    ('AnchorUp',(0.4167488,-0.3941015,0.8191520,0.0),'共享固定参考画面向上的世界方向'),
]
SLIDER_RANGES={
    'OuterBrushStrength':(0.0,2.0),
    'OuterBrushWidth':(0.5,8.0),
    'OuterBrushSpacing':(8.0,96.0),
    'OuterBrushDensity':(0.0,1.0),
    'OuterBrushStart':(0.05,0.95),
    'OuterBrushDarkThreshold':(0.01,0.30),
}


def resolved_parameters(instance):
    """Snapshot BEFORE deleting the master graph, including inherited values."""
    if instance is None:return {'scalars':{},'vectors':{},'textures':{}}
    return {
        'scalars':{str(name):L.get_material_instance_scalar_parameter_value(instance,name)
                   for name in L.get_scalar_parameter_names(instance)
                   if str(name) not in {'OuterBrushRegularity','OuterBrushBlend','OuterBrushCrossStrength','OuterBrushClarity'}},
        'vectors':{str(name):L.get_material_instance_vector_parameter_value(instance,name)
                   for name in L.get_vector_parameter_names(instance)},
        'textures':{str(name):L.get_material_instance_texture_parameter_value(instance,name)
                    for name in L.get_texture_parameter_names(instance)},
    }


def ensure_atlas(asset_tools):
    """Load/import only the owned atlas; missing art must fail before rebuild."""
    atlas=unreal.load_asset(ATLAS) if unreal.EditorAssetLibrary.does_asset_exist(ATLAS) else None
    imported=False
    if atlas is None:
        assert ATLAS_SOURCE.is_file(),(
            'Missing peripheral pencil atlas: provide '+str(ATLAS_SOURCE)
            +' or import the reviewed 2x2 atlas at '+ATLAS+'; no legacy texture fallback')
        task=unreal.AssetImportTask()
        task.set_editor_property('filename',str(ATLAS_SOURCE))
        task.set_editor_property('destination_path',OWN.rstrip('/'))
        task.set_editor_property('destination_name','T_Corridor_PeripheralHatchAtlas')
        task.set_editor_property('automated',True)
        task.set_editor_property('replace_existing',False)
        task.set_editor_property('save',False)
        asset_tools.import_asset_tasks([task])
        atlas=unreal.load_asset(ATLAS)
        imported=True
    assert isinstance(atlas,unreal.Texture2D),'Peripheral hatch atlas must be a Texture2D: '+ATLAS
    assert atlas.get_path_name().split('.')[0]==ATLAS
    atlas.set_editor_property('srgb',False)
    # G8 preserves subtle graphite pressure without RGB block-compression color
    # variation. MaterialExpressionUtils.cpp maps this contract to LinearGrayscale.
    atlas.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_GRAYSCALE)
    atlas.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    # The reviewed source is 1254 square. Resize the whole atlas uniformly at
    # build time; padding would move the half-UV quadrant boundaries. Keep PNG.
    resize_mode=getattr(unreal.TexturePowerOfTwoSetting,'RESIZE_TO_SPECIFIC_RESOLUTION',None)
    assert resize_mode is not None,'UE TexturePowerOfTwoSetting lacks ResizeToSpecificResolution'
    atlas.set_editor_property('power_of_two_mode',resize_mode)
    atlas.set_editor_property('resize_during_build_x',1024)
    atlas.set_editor_property('resize_during_build_y',1024)
    atlas.set_editor_property('max_texture_size',1024)
    atlas.set_editor_property('address_x',unreal.TextureAddress.TA_CLAMP)
    atlas.set_editor_property('address_y',unreal.TextureAddress.TA_CLAMP)
    atlas.set_editor_property('virtual_texture_streaming',False)
    assert unreal.EditorAssetLibrary.save_asset(ATLAS,False)
    return atlas,imported


def main():
    assert '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower(), (
        'Rebuild this master in a separate commandlet with the live Editor closed')
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([OWN.rstrip('/')],True)
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    # Validate/import source before deleting any existing material expressions.
    atlas,atlas_imported=ensure_atlas(tools)
    material=unreal.load_asset(MASTER) if unreal.EditorAssetLibrary.does_asset_exist(MASTER) else None
    if material is None:
        material=tools.create_asset('M_Corridor_FogFinish',OWN.rstrip('/'),unreal.Material,unreal.MaterialFactoryNew())
    assert material and material.get_path_name().split('.')[0]==MASTER
    instance=unreal.load_asset(INSTANCE) if unreal.EditorAssetLibrary.does_asset_exist(INSTANCE) else None
    assert instance is None or instance.parent==material,'Existing owned instance has another parent'
    preserved=resolved_parameters(instance)
    # Read the established anchor basis only; never edit the accepted SceneInk.
    scene_ink=(unreal.load_asset(OWN+'MI_Corridor_SceneInk')
               if unreal.EditorAssetLibrary.does_asset_exist(OWN+'MI_Corridor_SceneInk') else None)
    shared_anchor=resolved_parameters(scene_ink)
    material.set_editor_property('material_domain',unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property('blendable_location',unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)
    material.set_editor_property('blendable_priority',80)
    material.set_editor_property('disable_pre_exposure_scale',True)
    L.delete_all_material_expressions(material)
    custom=L.create_material_expression(material,unreal.MaterialExpressionCustom,0,0)
    custom.set_editor_property('description','雾后结构描边 / BeforeBloom')
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property('code',Path(__file__).with_name('NinjaFogFinish.hlsl').read_text(encoding='utf8'))
    links={}
    for i,(name,scene_id) in enumerate([
        ('SceneColor',unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0),
        ('SceneDepth',unreal.SceneTextureId.PPI_SCENE_DEPTH),
        ('SceneNormal',unreal.SceneTextureId.PPI_WORLD_NORMAL),
    ]):
        node=L.create_material_expression(material,unreal.MaterialExpressionSceneTexture,-1000,i*170)
        node.set_editor_property('scene_texture_id',scene_id)
        if name=='SceneDepth':
            scalar=L.create_material_expression(material,unreal.MaterialExpressionComponentMask,-700,i*170)
            scalar.set_editor_property('r',True);scalar.set_editor_property('g',False)
            scalar.set_editor_property('b',False);scalar.set_editor_property('a',False)
            assert L.connect_material_expressions(node,'Color',scalar,'')
            links[name]=(scalar,'')
        else:links[name]=(node,'Color')
    position=L.create_material_expression(material,unreal.MaterialExpressionWorldPosition,-1000,600)
    links['WorldPosition']=(position,'')
    hatch=L.create_material_expression(material,unreal.MaterialExpressionTextureObjectParameter,-1000,800)
    hatch.set_editor_property('parameter_name','OuterHatchAtlas')
    hatch.set_editor_property('texture',atlas)
    hatch.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    hatch.set_editor_property('group','02 外围暗部笔触')
    hatch.set_editor_property('desc','2x2 白底黑铅笔笔簇 atlas；线性灰阶、mip、白边；8 次采样')
    links['OuterHatchAtlas']=(hatch,'')
    for i,(name,value,group,description) in enumerate(SCALARS):
        node=L.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-1400,i*150)
        node.set_editor_property('parameter_name',name)
        node.set_editor_property('default_value',value)
        node.set_editor_property('group',group);node.set_editor_property('desc',description)
        node.set_editor_property('sort_priority',i)
        if name in SLIDER_RANGES:
            # Artist drag bounds, not a migration/clamp of existing values.
            # Out-of-range typed values are preserved by the snapshot below.
            slider_min,slider_max=SLIDER_RANGES[name]
            node.set_editor_property('slider_min',slider_min)
            node.set_editor_property('slider_max',slider_max)
        links[name]=(node,'')
    palette=L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-1400,1200)
    palette.set_editor_property('parameter_name','FarFogColor')
    palette.set_editor_property('default_value',unreal.LinearColor(0.078,0.100,0.118,1.0))
    palette.set_editor_property('group','01 远景配色')
    palette.set_editor_property('desc','远景艺术雾配色；实例可读取原艺术 MI 或当前体积雾配色')
    links['FarFogColor']=(palette,'')
    for i,(name,value,description) in enumerate(ANCHORS):
        anchor=L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-1700,2400+i*170)
        anchor.set_editor_property('parameter_name',name)
        anchor.set_editor_property('default_value',unreal.LinearColor(*value))
        anchor.set_editor_property('group','03 固定世界锚定')
        anchor.set_editor_property('desc',description)
        links[name]=(anchor,'')
    pins=[]
    for name in links:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    custom.set_editor_property('inputs',pins)
    for name,(node,output) in links.items():
        assert L.connect_material_expressions(node,output,custom,name),name
    assert L.connect_material_property(custom,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert all(L.get_inputs_for_material_expression(material,custom))
    errors=list(L.recompile_material(material));assert not errors,errors
    assert unreal.EditorAssetLibrary.save_asset(MASTER,False)
    if instance is None:
        instance=tools.create_asset('MI_Corridor_FogFinish',OWN.rstrip('/'),unreal.MaterialInstanceConstant,
                                    unreal.MaterialInstanceConstantFactoryNew())
        L.set_material_instance_parent(instance,material)
    assert instance and instance.parent==material,'Existing owned instance has another parent'
    # Restore every pre-existing effective parameter, rather than silently
    # replacing the user's approved outline, palette, distance or debug values.
    scalar_names={str(name) for name in L.get_scalar_parameter_names(instance)}
    vector_names={str(name) for name in L.get_vector_parameter_names(instance)}
    texture_names={str(name) for name in L.get_texture_parameter_names(instance)}
    assert set(preserved['scalars']).issubset(scalar_names),preserved['scalars'].keys()
    assert set(preserved['vectors']).issubset(vector_names),preserved['vectors'].keys()
    assert set(preserved['textures']).issubset(texture_names),preserved['textures'].keys()
    for name,value in preserved['scalars'].items():
        L.set_material_instance_scalar_parameter_value(instance,name,value)
    for name,value in preserved['vectors'].items():
        L.set_material_instance_vector_parameter_value(instance,name,value)
    for name,value in preserved['textures'].items():
        assert value is not None,'Existing texture parameter is missing: '+name
        # UE 5.8's implementation leaves its boolean return false even after
        # applying the value. Verify the effective texture below instead.
        L.set_material_instance_texture_parameter_value(instance,name,value)
    if 'WorldUnitsPerPixel' not in preserved['scalars'] and 'WorldUnitsPerPixel' in shared_anchor['scalars']:
        L.set_material_instance_scalar_parameter_value(instance,'WorldUnitsPerPixel',shared_anchor['scalars']['WorldUnitsPerPixel'])
    for name,_,_ in ANCHORS:
        if name not in preserved['vectors'] and name in shared_anchor['vectors']:
            L.set_material_instance_vector_parameter_value(instance,name,shared_anchor['vectors'][name])
    L.update_material_instance(instance)
    for name,value in preserved['scalars'].items():
        assert abs(L.get_material_instance_scalar_parameter_value(instance,name)-value)<0.00001,name
    for name,value in preserved['vectors'].items():
        actual=L.get_material_instance_vector_parameter_value(instance,name)
        assert all(abs(getattr(actual,c)-getattr(value,c))<0.00001 for c in ['r','g','b','a']),name
    for name,value in preserved['textures'].items():
        actual=L.get_material_instance_texture_parameter_value(instance,name)
        assert actual and actual.get_path_name()==value.get_path_name(),name
    assert unreal.EditorAssetLibrary.save_asset(INSTANCE,False)
    unreal.log('NINJA_FOG_FINISH_BUILT '+json.dumps({
        'master':MASTER,'instance':INSTANCE,'location':'BL_SceneColorBeforeBloom',
        'immediate_material_errors':errors,'platform_shader_validation':'pending rendered viewport',
        'source':'composited scene color + original opaque SceneDepth/WorldNormal; fixed shared world-plane strokes',
        'roughness_used':False,'procedural_pixel_grain_used':False,
        'preserved_existing_scalar_parameters':sorted(preserved['scalars']),
        'preserved_existing_vector_parameters':sorted(preserved['vectors']),
        'preserved_existing_texture_parameters':sorted(preserved['textures']),
        'peripheral_stroke_parameters':{
            name:L.get_material_instance_scalar_parameter_value(instance,name)
            for name,_,_,_ in SCALARS if name.startswith('OuterBrush')},
        'additional_passes':0,'stroke_texture_samples':8,'stroke_animation':False,
        'stroke_composition':'restored CrossHatch original atlas; 2 layers x 2x2 stable world-plane stamps; eight atlas reads; multiplicative dark-ink union',
        'stroke_color':'original CrossHatch dark ink; color multiplied by 1 minus the selected stroke mask',
        'stroke_reference':'project-authored pencil atlas; not the original Dota 2 shader',
        'atlas_default':ATLAS,'atlas_imported':atlas_imported,
        'atlas_instance':L.get_material_instance_texture_parameter_value(instance,'OuterHatchAtlas').get_path_name(),
        'atlas_contract':'white-paper 2x2 atlas; linear G8; 1024 square build resize; FromTextureGroup mips; Clamp XY; non-VT',
        'world_anchor_input':'AbsoluteWorldPosition reconstructed from current scene depth',
        'level_modified':False,'vendor_and_original_artist_assets_modified':False,
    },ensure_ascii=False))


main()
