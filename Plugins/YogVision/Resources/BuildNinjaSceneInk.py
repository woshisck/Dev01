"""Author one owned BeforeDOF scene-ink pass; never edit vendor fog assets.

First creation is safe before the material is attached to a scene. Rebuild an
existing master with UnrealEditor-Cmd.exe -run=pythonscript -script=<this file>
-AllowCommandletRendering -unattended -nosourcecontrol, with the live editor
closed. This recipe saves only the owned master and its owned instance.
An empty immediate error list is not a platform shader acceptance: verify the
rendering editor's completed shader jobs and final viewport before acceptance.
The level's post-process component/volume binding is intentionally separate.
Scene marks use a fixed world-anchored reference-camera drawing plane. The
legacy size names remain compatible; HatchWorldSize is reference-plane pixels.
Old brush texture/strength/size controls remain present but do not affect output.
"""
import json
from pathlib import Path
import unreal

L = unreal.MaterialEditingLibrary
OWN = '/Game/Art/Rendering/Fog/Ninja/'
MASTER = OWN+'M_Corridor_SceneInk'
INSTANCE = OWN+'MI_Corridor_SceneInk'
BRUSH = OWN+'T_Corridor_FogBrushDensity'

SCALARS = [
    ('ScreenBaseWeight',0.40,'01 修饰区域','全屏基础排线覆盖；外围和远处继续增强'),
    ('OutlineStrength',0.60,'00 描边与笔触','结构描边强度'),
    ('DetailStrength',0.18,'00 描边与笔触','细节描边强度'),
    ('BrushStrength',1.0,'90 旧参数（不参与成图）','兼容旧草稿；成图忽略，不再调制细碎明暗'),
    ('HatchStrength',0.40,'00 描边与笔触','铅笔排线强度'),
    ('PeripheralStart',0.45,'01 修饰区域','外围修饰起点'),
    ('FarStart',2200.0,'01 修饰区域','远处修饰起点（厘米）'),
    ('FarFeather',2000.0,'01 修饰区域','远处过渡（厘米）'),
    ('EdgeWidth',1.0,'02 纹理与线宽','描边宽度（像素）'),
    ('BrushWorldSize',180.0,'90 旧参数（不参与成图）','兼容旧草稿；成图忽略，不再取样噪点贴图'),
    ('HatchWorldSize',9.0,'02 纹理与线宽','排线间距（参考画面像素；兼容旧参数名）'),
    ('ScreenHatchAngle',-35.0,'02 纹理与线宽','固定参考画面内的排线方向（角度）'),
    ('ScreenPatchSize',160.0,'02 纹理与线宽','世界锚定笔触分块（参考画面像素）'),
    ('WorldUnitsPerPixel',2.0,'02 纹理与线宽','参考画面每像素对应的世界厘米；镜头移动不改变锚定'),
    ('DebugMode',0.0,'03 调试','0 成图；1 修饰区域；2 描边；3 笔触排线；4 雾数据黑底'),
]
ANCHORS = [
    ('AnchorRight',(0.6870875,0.7265747,0.0,0.0),'固定参考画面向右的世界方向'),
    ('AnchorUp',(0.4167488,-0.3941015,0.8191520,0.0),'固定参考画面向上的世界方向'),
]


def main():
    # Commandlets may run before the async asset registry has discovered newly
    # authored packages. Scan this owned folder before deciding create/rebuild.
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([OWN.rstrip('/')],True)
    exists = unreal.EditorAssetLibrary.does_asset_exist(MASTER)
    assert not exists or '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower(), (
        'An existing scene-ink master must be rebuilt in a separate commandlet with the live editor closed.'
    )
    texture = unreal.load_asset(BRUSH)
    assert texture, BRUSH
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = unreal.load_asset(MASTER) if exists else tools.create_asset(
        'M_Corridor_SceneInk',OWN.rstrip('/'),unreal.Material,unreal.MaterialFactoryNew())
    assert material and material.get_path_name().split('.')[0] == MASTER
    material.set_editor_property('material_domain',unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property('blendable_location',unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)
    material.set_editor_property('blendable_priority',40)
    # HLSL explicitly handles raw pre-exposed scene color and output.
    material.set_editor_property('disable_pre_exposure_scale',True)
    L.delete_all_material_expressions(material)

    custom = L.create_material_expression(material,unreal.MaterialExpressionCustom,0,0)
    custom.set_editor_property('description','场景排线 / 真实体积雾合成前')
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    shader = Path(__file__).with_name('NinjaSceneInk.hlsl')
    if not shader.exists():
        shader = Path(unreal.Paths.project_dir())/'Plugins/YogVision/Resources/NinjaSceneInk.hlsl'
    custom.set_editor_property('code',shader.read_text(encoding='utf8'))
    links = {}
    scene_specs = [
        ('SceneColor',unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0,'Color'),
        ('SceneDepth',unreal.SceneTextureId.PPI_SCENE_DEPTH,'scalar'),
        ('SceneNormal',unreal.SceneTextureId.PPI_WORLD_NORMAL,'Color'),
        ('SceneRoughness',unreal.SceneTextureId.PPI_ROUGHNESS,'scalar'),
    ]
    for index,(name,scene_id,output) in enumerate(scene_specs):
        node = L.create_material_expression(material,unreal.MaterialExpressionSceneTexture,-1000,index*150)
        node.set_editor_property('scene_texture_id',scene_id)
        if output == 'scalar':
            scalar = L.create_material_expression(material,unreal.MaterialExpressionComponentMask,-720,index*150)
            scalar.set_editor_property('r',True)
            scalar.set_editor_property('g',False)
            scalar.set_editor_property('b',False)
            scalar.set_editor_property('a',False)
            assert L.connect_material_expressions(node,'Color',scalar,'')
            links[name] = (scalar,'')
        else:
            links[name] = (node,output)
    position = L.create_material_expression(material,unreal.MaterialExpressionWorldPosition,-1000,650)
    links['WorldPosition'] = (position,'')
    brush = L.create_material_expression(material,unreal.MaterialExpressionTextureObjectParameter,-1000,820)
    brush.set_editor_property('parameter_name','BrushTexture')
    brush.set_editor_property('texture',texture)
    brush.set_editor_property('group','90 旧参数（不参与成图）')
    brush.set_editor_property('desc','兼容旧草稿；成图不取样此纹理')
    links['BrushTexture'] = (brush,'')
    for index,(name,value,group,description) in enumerate(SCALARS):
        node = L.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-1400,index*145)
        node.set_editor_property('parameter_name',name)
        node.set_editor_property('default_value',value)
        node.set_editor_property('group',group)
        node.set_editor_property('desc',description)
        node.set_editor_property('sort_priority',index)
        links[name] = (node,'')
    for index,(name,value,description) in enumerate(ANCHORS):
        node = L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-1800,2250+index*160)
        node.set_editor_property('parameter_name',name)
        node.set_editor_property('default_value',unreal.LinearColor(*value))
        node.set_editor_property('group','02 纹理与线宽')
        node.set_editor_property('desc',description)
        node.set_editor_property('sort_priority',len(SCALARS)+index)
        links[name] = (node,'')
    ink = L.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-1400,1700)
    ink.set_editor_property('parameter_name','InkColor')
    ink.set_editor_property('default_value',unreal.LinearColor(0.007,0.011,0.014,1.0))
    ink.set_editor_property('group','00 描边与笔触')
    ink.set_editor_property('desc','墨线颜色；只压暗场景，不抬亮暗部')
    links['InkColor'] = (ink,'')

    pins=[]
    for name in links:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    custom.set_editor_property('inputs',pins)
    for name,(node,output) in links.items():
        assert L.connect_material_expressions(node,output,custom,name), name
    assert L.connect_material_property(custom,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert all(L.get_inputs_for_material_expression(material,custom)), 'Disconnected input'
    errors=list(L.recompile_material(material))
    assert not errors, errors
    assert all(L.get_inputs_for_material_expression(material,custom)), 'Input lost during compile'
    assert unreal.EditorAssetLibrary.save_asset(MASTER,False), MASTER

    instance = unreal.load_asset(INSTANCE) if unreal.EditorAssetLibrary.does_asset_exist(INSTANCE) else None
    if instance is None:
        instance = tools.create_asset('MI_Corridor_SceneInk',OWN.rstrip('/'),unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        L.set_material_instance_parent(instance,material)
    assert instance.parent == material, 'Existing owned instance has another parent; preserve it for review'
    L.update_material_instance(instance)
    assert unreal.EditorAssetLibrary.save_asset(INSTANCE,False), INSTANCE
    unreal.log('NINJA_SCENE_INK_BUILT '+json.dumps({
        'master':MASTER,'instance':INSTANCE,'location':'BL_SceneColorBeforeDOF',
        'inputs':len(links),'immediate_material_errors':errors,'platform_shader_validation':'pending rendered viewport',
        'coordinates':'world-anchored fixed reference-camera 2D plane',
        'brush_texture_samples':0,'legacy_brush_controls_ignored':True,
        'vendor_assets_modified':False,
        'level_modified':False,'fog_mask_generated':False,
    },ensure_ascii=False))


main()
