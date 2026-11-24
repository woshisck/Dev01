"""Rebuild the owned spatial-density master from the installed Ninja source.

Run with UnrealEditor-Cmd.exe <project> -run=pythonscript
    -script=<this file> -AllowCommandletRendering -unattended -nosourcecontrol

The project must already contain the original FluidNinja VolumeSmoke assets and
T_Corridor_FogBrushDensity. Check out the owned master before rebuilding it.
This recipe never reparents instances, loads a level, or edits vendor assets.
Live Editor execution is rejected because changing a rendered master's custom
input layout can race existing dynamic-material uniform-expression caches.

Original raymarch code and its licensing comments are read directly from
FluidNinja's M_VolumeSmoke on every run. Only the two density blocks in each
branch are replaced. The opt-in single-volume profile also resamples each
native light ray over its original total reach, without changing view steps.
Optional form lighting uses one local 3D macro-density direction difference
on lit view samples only; it is not an extra shadow ray or fluid simulation.
NinjaSpatialDensity.hlsl is this project's shared view/light extension.
"""

import json
import re
from pathlib import Path

import unreal


SOURCE = "/Game/FluidNinja/Usecases/Volumetrics/BaseMaterials/Output/M_VolumeSmoke"
OWN = "/Game/Art/Rendering/Fog/Ninja/"
TARGET = OWN + "M_Corridor_FogSpatial"
BRUSH = OWN + "T_Corridor_FogBrushDensity"
LIBRARY = unreal.MaterialEditingLibrary

PARAMETERS = [
    ("SpatialCloudSize", 1200.0, "云团尺寸（厘米）"),
    ("SpatialCoverage", 0.55, "云团覆盖"),
    ("SpatialWarp", 0.65, "翻卷幅度"),
    ("SpatialSpeed", 0.28, "翻卷速度"),
    ("SpatialDetail", 0.12, "体内笔触"),
    ("SpatialTimeScale", 1.0, "动画开关：0 冻结、1 播放"),
    ("SpatialTimeOffset", 0.0, "冻结时刻（秒）"),
    ("SpatialProfileEnabled", 0.0, "0 保留旧配方；1 单体积近地雾与稀疏上层云"),
    ("SpatialGroundHeight", 0.0, "世界地面高度（厘米）；不依容器中心"),
    ("SpatialGroundThickness", 240.0, "近地雾厚度（厘米）；此高度约剩一半地雾"),
    ("SpatialUpperAmount", 0.20, "稀疏上层云量；与近地雾共用一个密度场和受光"),
    ("SpatialVolumeContrast", 1.60, "体积明暗：云团密度反差，越大保留更明确的空隙"),
    ("SpatialBillowStrength", 0.0, "局部云核隆起；0 保留原形态，开启后有效云核增加一次宏观三维采样"),
    ("SpatialVerticalScale", 1.0, "云核纵向尺度；1 原样，小于 1 增加近地层的纵向变化"),
    ("SpatialAlbedoUniformity", 0.0, "受光层次：减弱密云自动趋亮；仅收敛调色，不改变密度和原灯光"),
    ("SpatialFormLighting", 0.0, "云体形态受光：沿真实入射方向估算三维宏密度梯度；仅有效云核的视线采样增加一次三维读取，不改雾密度或自阴影"),
    ("SpatialNearDistance", 1200.0, "相机近端清雾距离（世界厘米）"),
    ("SpatialNearFeather", 1000.0, "相机近端退散距离（世界厘米）"),
    ("SpatialCenterStrength", 0.65, "镜头中心清雾强度；0 关闭"),
    ("SpatialCenterRadius", 0.25, "镜头中心清雾半径（归一画面半径）"),
    ("SpatialCenterFeather", 0.35, "镜头中心清雾羽化（归一画面半径）"),
    ("SpatialCenterResidual", 0.10, "镜头中心最少保留的雾密度比例；避免全空硬洞"),
    ("SpatialLightSamples", 4.0, "单体积自阴影采样预算；1 至 20，4 为质量近似"),
    ("SpatialShadowReach", 1.0, "自阴影探测总长度相对原版 20 步；1 保持原覆盖"),
    ("SpatialJitterStrength", 1.0, "体积步进抖动；0 平滑稳定但可能分层，1 保留原版抖动"),
    ("SpatialEnvironmentEnabled", 0.0, "空间环境交互；由环境控制Actor在动态实例上启用"),
    ("EnvGroundHeight", 0.0, "环境地面高度（世界厘米）"),
    ("EnvGroundBodyHeight", 180.0, "实心近地雾向上衰减的半密度高度（厘米）"),
    ("EnvGroundAmount", 2.5, "近地密度倍率"),
    ("EnvUpperAmount", 0.55, "上层密度倍率"),
    ("EnvSurfaceFade", 40.0, "距离场固体边缘退散宽度（厘米）"),
    ("EnvSurfaceBand", 250.0, "距离场接触带范围（厘米）"),
    ("EnvContactBoost", 0.35, "近墙柱表面的雾体密度增强"),
    ("EnvPlayerStrength", 0.85, "玩家附近清雾强度"),
    ("EnvPlayerFeather", 220.0, "玩家清雾羽化（厘米）"),
    ("EnvPlayerHeight", 350.0, "玩家清雾竖向范围（厘米）"),
    ("EnvLightStrength", 0.8, "局部灯光附近清雾强度"),
    ("EnvLightFeather", 200.0, "灯光清雾羽化（厘米）"),
    ("EnvDebugMode", 0.0, "调试：0成图，1检查GDF视图资源（绿有效，红无效）"),
]

ENVIRONMENT = {name for name,_,_ in PARAMETERS if name.startswith('Env') or name=='SpatialEnvironmentEnabled'}
ENV_VECTORS = ['EnvObserver']+['EnvLight'+str(i) for i in range(4)]+['EnvLightDirection'+str(i) for i in range(4)]
ENV_ARGUMENTS = (
    'SpatialEnvironmentEnabled,EnvGroundHeight,EnvGroundBodyHeight,EnvGroundAmount,EnvUpperAmount,'
    'EnvSurfaceFade,EnvSurfaceBand,EnvContactBoost,EnvObserver,EnvPlayerStrength,EnvPlayerFeather,EnvPlayerHeight,'
    'EnvLightStrength,EnvLightFeather,EnvLight0,EnvLight1,EnvLight2,EnvLight3,'
    'EnvLightDirection0,EnvLightDirection1,EnvLightDirection2,EnvLightDirection3'
)

PROFILE_SHAPE = {"SpatialProfileEnabled", "SpatialGroundHeight", "SpatialGroundThickness",
                 "SpatialUpperAmount", "SpatialVolumeContrast", "SpatialBillowStrength",
                 "SpatialVerticalScale"}
LIGHT_RESPONSE = {"SpatialAlbedoUniformity", "SpatialFormLighting"}
CAMERA_CLEAR = {"SpatialNearDistance", "SpatialNearFeather", "SpatialCenterStrength",
                "SpatialCenterRadius", "SpatialCenterFeather", "SpatialCenterResidual"}
QUALITY = {"SpatialLightSamples", "SpatialShadowReach", "SpatialJitterStrength"}
SLIDER_RANGES = {
    "SpatialProfileEnabled": (0.0, 1.0), "SpatialGroundHeight": (-500.0, 1000.0),
    "SpatialGroundThickness": (25.0, 1000.0), "SpatialUpperAmount": (0.0, 1.0),
    "SpatialBillowStrength": (0.0, 1.5), "SpatialVerticalScale": (0.35, 2.0),
    "SpatialAlbedoUniformity": (0.0, 1.0),
    "SpatialFormLighting": (0.0, 1.0),
    "SpatialVolumeContrast": (0.25, 4.0), "SpatialNearDistance": (0.0, 5000.0),
    "SpatialNearFeather": (1.0, 4000.0), "SpatialCenterStrength": (0.0, 1.0),
    "SpatialCenterRadius": (0.0, 1.2), "SpatialCenterFeather": (0.01, 1.0),
    "SpatialCenterResidual": (0.0, 1.0), "SpatialLightSamples": (1.0, 20.0),
    "SpatialShadowReach": (0.25, 2.0),
    "SpatialJitterStrength": (0.0, 1.0),
}

SAMPLE_PROFILE_ARGUMENTS = (
    "SpatialProfileEnabled,SpatialGroundHeight,SpatialGroundThickness,"
    "SpatialUpperAmount,SpatialVolumeContrast,SpatialNearDistance,SpatialNearFeather,"
    "SpatialCenterStrength,SpatialCenterRadius,SpatialCenterFeather,SpatialCenterResidual,"
    "SpatialBillowStrength,SpatialVerticalScale,SpatialAlbedoUniformity"
)

VIEW_DENSITY = """float3 spatialIncidentWorld=SPATIAL_INCIDENT_WORLD;
spatialIncidentWorld*=rsqrt(max(dot(spatialIncidentWorld,spatialIncidentWorld),0.000001));
float3 spatialView = SpatialField.Sample(Parameters,RayOrigin,SpatialOrigin,SpatialNoise,SpatialNoiseSampler,SpatialBrush,SpatialBrushSampler,spatialClock,SpatialCloudSize,SpatialCoverage,SpatialWarp,SpatialDetail,Extrude,ExtrudeSoften,BaseDensity,""" + SAMPLE_PROFILE_ARGUMENTS + """,spatialIncidentWorld,SPATIAL_EVALUATE_FORM,SpatialFormLighting,""" + ENV_ARGUMENTS + """);
float curtexture=spatialView.y;
float cursample=spatialView.x;
"""

LIGHT_DENSITY = """float3 spatialLight = SpatialField.Sample(Parameters,LightRay,SpatialOrigin,SpatialNoise,SpatialNoiseSampler,SpatialBrush,SpatialBrushSampler,spatialClock,SpatialCloudSize,SpatialCoverage,SpatialWarp,SpatialDetail,Extrude,ExtrudeSoften,BaseDensity,""" + SAMPLE_PROFILE_ARGUMENTS + """,float3(0.0,0.0,0.0),0.0,SpatialFormLighting,""" + ENV_ARGUMENTS + """);
float curlsample=spatialLight.x;
"""


def parameter_name(node):
    return str(node.get_editor_property("parameter_name"))


def replacement_code(native_code, shared_code):
    """Keep the source attribution and all native non-density integration."""
    assert "//LICENSING" in native_code, "Ninja licensing header missing"
    assert native_code.count("float curtexture =") == 1
    lit = "float curltexture =" in native_code
    directional = "LightRay-=LightDirection;" in native_code
    if directional:
        incident = "TransformLocalVectorToWorld(Parameters,-LightDirection).xyz"
    elif lit:
        assert "LD=normalize(RayOrigin-LightPosition);" in native_code
        incident = "TransformLocalVectorToWorld(Parameters,(LightPosition-RayOrigin)*100.0).xyz"
    else:
        incident = "float3(0.0,0.0,0.0)"
    view_density = VIEW_DENSITY.replace("SPATIAL_INCIDENT_WORLD", incident).replace(
        "SPATIAL_EVALUATE_FORM", "1.0" if lit else "0.0")
    begin = native_code.index("float curtexture =")
    end = native_code.index("if(cursample>.001)", begin)
    code = native_code[:begin] + view_density + native_code[end:]
    lighting_setup = ""
    if "float curltexture =" in code:
        begin = code.index("float curltexture =")
        end = code.index("lightvisibility*=", begin)
        code = code[:begin] + LIGHT_DENSITY + code[end:]
        # Keep the original point/directional light paths and Beer-Lambert
        # framework. A profile budget of 4 integrates over the SAME total
        # length as native 20 steps; it is a coarser quadrature, not a shorter
        # light ray disguised as a performance improvement.
        code, loops = re.subn(r"for\s*\(int l=0;20>l;l\+\+\)",
                             "for(int l=0;spatialLightBudget>l;l++)", code)
        assert loops == 1, "Native 20-sample light-loop contract changed"
        assert code.count("ShadowStepSize") == (1 if directional else 2), (
            "Native shadow distance use changed", directional)
        if directional:
            # Native directional inputs are ALREADY multiplied by the native
            # step in the material graph. Rescale that vector too, not only ds.
            assert code.count("LightRay-=LightDirection;") == 1
            code = code.replace("LightRay-=LightDirection;",
                                "LightRay-=LightDirection*spatialShadowRatio;")
        code = code.replace("ShadowStepSize", "spatialShadowStep")
        lighting_setup = (
            "int spatialLightBudget=(SpatialProfileEnabled>0.5 || SpatialEnvironmentEnabled>0.5) ? (int)clamp(SpatialLightSamples,1.0,20.0) : 20;\n"
            "float spatialShadowRatio=(SpatialProfileEnabled>0.5 || SpatialEnvironmentEnabled>0.5) ? 20.0/(float)spatialLightBudget*max(SpatialShadowReach,0.05) : 1.0;\n"
            "float spatialShadowStep=ShadowStepSize*spatialShadowRatio;\n"
        )
        # Point paths have a native radial attenuation phase; directional paths
        # do not. Apply the local form cue after the complete native visibility,
        # immediately before its existing single color integration in either.
        if directional:
            color_anchor = "color+=absorptionfrommarch"
            assert code.count(color_anchor) == 1
            code = code.replace(color_anchor,
                                "lightvisibility*=spatialView.z;\n" + color_anchor)
        else:
            phase_anchor = "lightvisibility*=phase;"
            assert code.count(phase_anchor) == 1
            code = code.replace(phase_anchor,
                                phase_anchor + "\nlightvisibility*=spatialView.z;")
    # Keep the native temporal random number and all existing uses. Scale its
    # amplitude once, before the view origin/t offset (and point shadow offset)
    # consumes it. Legacy mode and the default strength 1 are exact identities.
    jitter_pattern = (
        r"\bfloat\s+rand\s*=\s*float\s*\(\s*Rand3DPCG16\s*\(\s*randpos\s*\)"
        r"\s*\.\s*x\s*\)\s*/\s*0xffff\s*;"
    )
    code, jitter_count = re.subn(
        jitter_pattern,
        lambda match: match.group(0) + (
            "\nrand*=SpatialProfileEnabled>0.5 ? saturate(SpatialJitterStrength) : 1.0;"
        ),
        code,
        flags=re.IGNORECASE,
    )
    assert jitter_count == 1, "Native temporal rand declaration contract changed"
    return (
        shared_code
        + '\nif(EnvDebugMode>0.5) {\n#if PLATFORM_SUPPORTS_DISTANCE_FIELDS\n'
          'return float4((NumGlobalSDFClipmaps>0 ? float3(0.02,0.4,0.02) : float3(0.5,0.01,0.01))'
          '+min(abs(SpatialDistanceFieldHint),1000.0)*0.000001,0.0);\n#else\nreturn float4(0.5,0.01,0.01,0.0);\n#endif\n}\n'
        + "\nfloat spatialClock=(SpatialClock*SpatialTimeScale+SpatialTimeOffset)*SpatialSpeed;\n"
        + lighting_setup
        + code
    )


def find_parameter(nodes, expression_type, name):
    return next(
        (node for node in nodes if isinstance(node, expression_type)
         and parameter_name(node) == name),
        None,
    )


def ensure_auxiliary(material, nodes, expression_type, description, x, y):
    node = next(
        (node for node in nodes if isinstance(node, expression_type)
         and node.get_editor_property("desc") == description),
        None,
    )
    if node is None:
        node = LIBRARY.create_material_expression(material, expression_type, x, y)
        node.set_editor_property("desc", description)
    return node


def assert_spatial_connections(material, custom):
    names = LIBRARY.get_material_expression_input_names(custom)
    expressions = LIBRARY.get_inputs_for_material_expression(material, custom)
    assert len(names) == len(expressions), custom.get_name()
    missing = [
        str(name) for name, expression in zip(names, expressions)
        if str(name).startswith("Spatial") and expression is None
    ]
    assert not missing, (custom.get_name(), missing)
    assert sum(str(name).startswith("Spatial") or str(name).startswith('Env') for name in names) == len(PARAMETERS) + 5 + len(ENV_VECTORS)


def main():
    command_line = unreal.SystemLibrary.get_command_line().lower()
    assert "-run=pythonscript" in command_line, (
        "Use -run=pythonscript in a separate UnrealEditor-Cmd process; "
        "do not rebuild a rendered master inside the live Editor."
    )
    shared = Path(__file__).with_name("NinjaSpatialDensity.hlsl").read_text(encoding="utf8")
    source = unreal.load_asset(SOURCE)
    assert source, "Missing original Ninja M_VolumeSmoke"
    source_customs = {
        node.get_name(): node
        for node in LIBRARY.get_material_expressions(source)
        if isinstance(node, unreal.MaterialExpressionCustom)
    }
    expected_names = {"MaterialExpressionCustom_" + str(index) for index in range(6)}
    assert set(source_customs) == expected_names, "Unexpected Ninja source revision"
    codes = {
        name: replacement_code(node.get_editor_property("code"), shared)
        for name, node in source_customs.items()
    }
    brush_texture = unreal.load_asset(BRUSH)
    assert brush_texture, "Missing owned fog brush density texture"

    material = unreal.load_asset(TARGET)
    if material is None:
        material = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, TARGET)
    assert material and material.get_path_name().split(".")[0] == TARGET
    nodes = list(LIBRARY.get_material_expressions(material))

    controls = {}
    for index, (name, default, description) in enumerate(PARAMETERS):
        node = find_parameter(nodes, unreal.MaterialExpressionScalarParameter, name)
        if node is None:
            node = LIBRARY.create_material_expression(
                material, unreal.MaterialExpressionScalarParameter, -4200, index * 160
            )
            node.set_editor_property("parameter_name", name)
            node.set_editor_property("default_value", default)
        group = ("05 环境交互（Actor驱动）" if name in ENVIRONMENT else
                 "02 单体积近地形态" if name in PROFILE_SHAPE else
                 "02 单体积受光层次" if name in LIGHT_RESPONSE else
                 "03 镜头清雾" if name in CAMERA_CLEAR else
                 "04 单体积质量" if name in QUALITY else
                 "00 空间云团" if index < 5 else "01 动画预览")
        node.set_editor_property("group", group)
        node.set_editor_property("desc", description)
        node.set_editor_property("sort_priority", index)
        if name in SLIDER_RANGES:
            node.set_editor_property("slider_min", SLIDER_RANGES[name][0])
            node.set_editor_property("slider_max", SLIDER_RANGES[name][1])
        controls[name] = node

    origin = ensure_auxiliary(material, nodes, unreal.MaterialExpressionActorPositionWS,
                              "Spatial field origin", -4400, 1200)
    clock = ensure_auxiliary(material, nodes, unreal.MaterialExpressionTime,
                             "Spatial field clock", -4400, 1400)
    noise = find_parameter(nodes, unreal.MaterialExpressionTextureObjectParameter, "NoiseVolume")
    assert noise, "Original volume-noise parameter missing"
    brush = find_parameter(nodes, unreal.MaterialExpressionTextureObjectParameter, "SpatialBrush")
    if brush is None:
        brush = LIBRARY.create_material_expression(
            material, unreal.MaterialExpressionTextureObjectParameter, -4400, 1600
        )
        brush.set_editor_property("parameter_name", "SpatialBrush")
        brush.set_editor_property("group", "00 空间云团")
        brush.set_editor_property("texture", brush_texture)
    links = dict(controls, SpatialOrigin=origin, SpatialClock=clock,
                 SpatialNoise=noise, SpatialBrush=brush)
    # A real native expression declares GDF usage to the material compiler and
    # view relevance. Custom HLSL alone would not declare this resource contract.
    distance = ensure_auxiliary(material,nodes,unreal.MaterialExpressionDistanceToNearestSurface,
                                'Spatial environment distance-field binding',-4600,1800)
    links['SpatialDistanceFieldHint']=distance
    for i,name in enumerate(ENV_VECTORS):
        node=find_parameter(nodes,unreal.MaterialExpressionVectorParameter,name)
        if node is None:
            node=LIBRARY.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-4700,2000+i*160)
            node.set_editor_property('parameter_name',name)
            node.set_editor_property('default_value',unreal.LinearColor(0,0,0,0))
        node.set_editor_property('group','05 环境交互（Actor驱动）')
        links[name]=node

    customs = [node for node in nodes if isinstance(node, unreal.MaterialExpressionCustom)
               and node.get_name() in codes]
    assert len(customs) == 6
    for custom in customs:
        # Code/property edits can reconstruct graph pins: finish them BEFORE wiring.
        custom.set_editor_property("code", codes[custom.get_name()])
        inputs = list(custom.get_editor_property("inputs"))
        names = {str(pin.get_editor_property("input_name")) for pin in inputs}
        for name in links:
            if name not in names:
                pin = unreal.CustomInput()
                pin.set_editor_property("input_name", name)
                inputs.append(pin)
        custom.set_editor_property("inputs", inputs)
        for name, expression in links.items():
            output = 'RGBA' if name in ENV_VECTORS else ''
            assert LIBRARY.connect_material_expressions(expression, output, custom, name), name
        assert_spatial_connections(material, custom)

    errors = list(LIBRARY.recompile_material(material))
    assert not errors, errors
    for custom in customs:
        assert_spatial_connections(material, custom)
    assert unreal.EditorAssetLibrary.save_asset(TARGET, False), TARGET
    unreal.log("NINJA_SPATIAL_MASTER_BUILT " + json.dumps({
        "source_preserved": SOURCE,
        "saved_master": TARGET,
        "branches": len(customs),
        "compile_errors": errors,
        "instances_reparented": False,
        "levels_modified": False,
        "profile_default": "legacy off; explicitly enable on a single owned container",
        "single_volume_density": "ground body + sparse upper clouds + camera clear in identical view/light queries",
        "billow_quality": "optional one macro 3D read per effective cloud-core query; zero preserves the existing shape",
        "form_lighting_quality": "optional one ahead macro-density read per lit view cloud-core query only; no density/transmittance change or extra light-ray samples",
        "light_quality": "profile budget resamples native 20-step total distance; default 4, reach 1",
    }, ensure_ascii=False))


main()
