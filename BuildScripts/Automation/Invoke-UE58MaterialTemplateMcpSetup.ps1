param(
    [string]$RepoRoot = "",
    [string]$ServerUrl = "http://127.0.0.1:8765/mcp",
    [string]$OutputRoot = "",
    [int]$TimeoutSec = 60,
    [switch]$ForceRebuild
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\MaterialTemplateMcp"
}

$mcpToolPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58McpTool.ps1"
if (-not (Test-Path -LiteralPath $mcpToolPath)) {
    throw "Missing MCP wrapper: $mcpToolPath"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Invoke-ToolViaWrapper {
    param(
        [string]$ToolsetName,
        [string]$ToolName,
        [hashtable]$Arguments
    )

    $argsPath = Join-Path $env:TEMP ("ue58_mcp_args_{0}.json" -f ([guid]::NewGuid().ToString("N")))
    try {
        $Arguments | ConvertTo-Json -Depth 64 -Compress | Set-Content -LiteralPath $argsPath -Encoding UTF8
        $responseText = & $mcpToolPath `
            -ServerUrl $ServerUrl `
            -Mode CallTool `
            -ToolsetName $ToolsetName `
            -ToolName $ToolName `
            -ArgumentsJsonPath $argsPath `
            -TimeoutSec $TimeoutSec

        $response = ($responseText | Out-String) | ConvertFrom-Json
        if ($response.result.isError) {
            $message = ($response.result.content | ForEach-Object { $_.text }) -join "`n"
            throw "MCP tool failed: $ToolsetName.$ToolName - $message"
        }

        $toolText = ($response.result.content | Select-Object -First 1).text
        if ([string]::IsNullOrWhiteSpace($toolText)) {
            return $null
        }

        return ($toolText | ConvertFrom-Json)
    }
    finally {
        Remove-Item -LiteralPath $argsPath -Force -ErrorAction SilentlyContinue
    }
}

# ProgrammaticToolset requires the execution environment to be read once before
# execute_tool_script is used.
$environment = Invoke-ToolViaWrapper `
    -ToolsetName "editor_toolset.toolsets.programmatic.ProgrammaticToolset" `
    -ToolName "get_execution_environment" `
    -Arguments @{}

$programmaticScript = @'
import json

MAT_TOOLS = "editor_toolset.toolsets.material.MaterialTools"
MI_TOOLS = "editor_toolset.toolsets.material_instance.MaterialInstanceTools"
ASSET_TOOLS = "editor_toolset.toolsets.asset.AssetTools"
OBJ_TOOLS = "editor_toolset.toolsets.object.ObjectTools"

MATERIAL_FOLDER = "/Game/Art/Material/EnvMaterial/Main"
INSTANCE_FOLDER = "/Game/Art/Material/EnvMaterial/Instances"
MATERIAL_NAME = "M_Env_MasterA_Source"
MATERIAL_PATH = MATERIAL_FOLDER + "/" + MATERIAL_NAME
DEFAULT_TEX = {"refPath": "/Engine/EngineResources/DefaultTexture.DefaultTexture"}
DEFAULT_NORMAL = {"refPath": "/Engine/EngineMaterials/DefaultNormal.DefaultNormal"}
FORCE_REBUILD = __FORCE_REBUILD__

PARAMETERS = [
    "TierMaterialQuality",
    "MaxRuntimeBlendLayers",
    "LayerBWeight",
    "LayerCWeight",
    "DynamicOverlayQuality",
    "MaterialLightQuality",
    "MaterialLightMaxLightInfoCount",
    "LightInfoIntensity",
    "UseBakedResult",
]

BATCH_VT_TEXTURES = [
    "T_BatchVT_BaseColor",
    "T_BatchVT_Normal",
    "T_BatchVT_ORM",
    "T_BatchVT_LightInfo",
]


def tool(name, args):
    return execute_tool(name, json.dumps(args))


def create_folder(path):
    return tool(ASSET_TOOLS + ".create_folder", {"path": path})["returnValue"]


def exists(path):
    return tool(ASSET_TOOLS + ".exists", {"path": path})["returnValue"]


def load_asset(path):
    return tool(ASSET_TOOLS + ".load_asset", {"asset_path": path})["returnValue"]


def delete_asset(path):
    return tool(ASSET_TOOLS + ".delete", {"path": path})["returnValue"]


def save(paths):
    return tool(ASSET_TOOLS + ".save_assets", {"asset_paths": paths})["returnValue"]


def create_material(folder, name):
    return tool(MAT_TOOLS + ".create_material", {"folder_path": folder, "asset_name": name})["returnValue"]


def add_expr(material, class_path, x, y):
    return tool(MAT_TOOLS + ".add_expression", {
        "material_or_function": material,
        "expression_class": {"refPath": class_path},
        "x": x,
        "y": y
    })["returnValue"]


def set_props(ref, values):
    return tool(OBJ_TOOLS + ".set_properties", {
        "instance": ref,
        "values": json.dumps(values)
    })["returnValue"]


def output_candidates(output):
    names = []
    if output not in names:
        names.append(output)
    if output == "":
        names.append("Result")
    return names


def connect_expr(src, output, dst, input_name):
    last_error = None
    for output_name in output_candidates(output):
        try:
            return tool(MAT_TOOLS + ".connect_expressions", {
                "from_expression": src,
                "from_output_name": output_name,
                "to_expression": dst,
                "to_input_name": input_name
            })
        except Exception as error:
            last_error = error
    raise last_error


def connect_output(expr, output, prop):
    last_error = None
    for output_name in output_candidates(output):
        try:
            return tool(MAT_TOOLS + ".connect_to_output", {
                "expression": expr,
                "output_name": output_name,
                "material_property": prop
            })
        except Exception as error:
            last_error = error
    raise last_error


def input_names(expr):
    return tool(MAT_TOOLS + ".get_expression_input_names", {"expression": expr})["returnValue"]


def property_input(material, prop):
    return tool(MAT_TOOLS + ".get_property_input", {
        "material": material,
        "material_property": prop
    })["returnValue"]


def recompile(material):
    return tool(MAT_TOOLS + ".recompile", {"material_or_function": material})


def list_parameters(material):
    return tool(MI_TOOLS + ".list_parameters", {"material": material})["returnValue"]


def create_mi(folder, name, parent):
    path = folder + "/" + name
    if exists(path):
        return load_asset(path)
    return tool(MI_TOOLS + ".create", {"folder_path": folder, "asset_name": name, "parent": parent})["returnValue"]


def set_scalar(instance, name, value):
    tool(MI_TOOLS + ".set_parameter_override", {"instance": instance, "name": name, "override": True})
    return tool(MI_TOOLS + ".set_scalar_parameter", {"instance": instance, "name": name, "value": value})


def get_scalar(instance, name):
    return tool(MI_TOOLS + ".get_scalar_parameter", {"instance": instance, "name": name})["returnValue"]


def tex_param(material, name, x, y, texture=DEFAULT_TEX):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionTextureSampleParameter2D", x, y)
    set_props(expr, {"ParameterName": name, "Group": "01 Source Textures", "Texture": texture})
    return expr


def scalar_param(material, name, default, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionScalarParameter", x, y)
    set_props(expr, {"ParameterName": name, "DefaultValue": default, "Group": "00 Performance Tier"})
    return expr


def const_expr(material, value, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionConstant", x, y)
    set_props(expr, {"R": value})
    return expr


def multiply(material, a, b, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionMultiply", x, y)
    connect_expr(a, "", expr, "A")
    connect_expr(b, "", expr, "B")
    return expr


def subtract(material, a, b, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionSubtract", x, y)
    connect_expr(a, "", expr, "A")
    connect_expr(b, "", expr, "B")
    return expr


def saturate(material, value, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionClamp", x, y)
    set_props(expr, {"MinDefault": 0.0, "MaxDefault": 1.0})
    connect_expr(value, "", expr, input_names(expr)[0])
    return expr


def lerp(material, a, b, alpha, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionLinearInterpolate", x, y)
    connect_expr(a, "", expr, "A")
    connect_expr(b, "", expr, "B")
    connect_expr(alpha, "", expr, "Alpha")
    return expr


def quality_switch(material, default_value, low, medium, high, epic, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionQualitySwitch", x, y)
    pins = input_names(expr)
    by_name = {name: name for name in pins}
    for pin_name, source in [
        ["Default", default_value],
        ["Low", low],
        ["Medium", medium],
        ["High", high],
        ["Epic", epic],
    ]:
        if source and pin_name in by_name:
            connect_expr(source, "", expr, by_name[pin_name])
    return expr


def mask(material, name, r, g, b, a, x, y):
    expr = add_expr(material, "/Script/Engine.MaterialExpressionComponentMask", x, y)
    set_props(expr, {"Desc": name, "R": r, "G": g, "B": b, "A": a})
    return expr


def run():
    create_folder(MATERIAL_FOLDER)
    create_folder(INSTANCE_FOLDER)

    instance_asset_paths = [
        INSTANCE_FOLDER + "/MI_Env_MasterA_Source_Epic",
        INSTANCE_FOLDER + "/MI_Env_MasterA_Source_High",
        INSTANCE_FOLDER + "/MI_Env_MasterA_Source_Mid",
        INSTANCE_FOLDER + "/MI_Env_MasterA_Source_Low",
    ]

    if FORCE_REBUILD:
        for path in instance_asset_paths:
            if exists(path):
                delete_asset(path)
        if exists(MATERIAL_PATH):
            delete_asset(MATERIAL_PATH)

    if exists(MATERIAL_PATH):
        material = load_asset(MATERIAL_PATH)
        created_material = False
    else:
        material = create_material(MATERIAL_FOLDER, MATERIAL_NAME)
        created_material = True

    current_params = list_parameters(material)
    current_param_names = set([p["name"] for p in current_params])
    required_names = set(PARAMETERS + BATCH_VT_TEXTURES + [
        "T_BaseColor_A",
        "T_BaseColor_B",
        "T_BaseColor_C",
        "T_Normal_A",
        "T_ORM_A",
        "T_Height_B",
        "T_Height_C",
        "T_LightInfo",
        "LightInfoIntensity",
    ])
    missing_params = sorted(list(required_names - current_param_names))

    existing_wiring = {}
    missing_outputs = []
    if not created_material:
        for prop in ["MP_BaseColor", "MP_Normal", "MP_AmbientOcclusion", "MP_Roughness", "MP_Metallic"]:
            try:
                info = property_input(material, prop)
                existing_wiring[prop] = info
                if not info.get("expression"):
                    missing_outputs.append(prop)
            except Exception:
                missing_outputs.append(prop)

    graph_updated = created_material or len(missing_params) > 0 or len(missing_outputs) > 0

    if graph_updated:
        base_a = tex_param(material, "T_BaseColor_A", -900, -360)
        base_b = tex_param(material, "T_BaseColor_B", -900, -200)
        base_c = tex_param(material, "T_BaseColor_C", -900, -40)
        normal_a = tex_param(material, "T_Normal_A", -900, 140, DEFAULT_NORMAL)
        orm_a = tex_param(material, "T_ORM_A", -900, 320)
        height_b = tex_param(material, "T_Height_B", -900, 500)
        height_c = tex_param(material, "T_Height_C", -900, 660)
        light_info = tex_param(material, "T_LightInfo", -900, 840)
        batch_base = tex_param(material, "T_BatchVT_BaseColor", -1180, 1040)
        batch_normal = tex_param(material, "T_BatchVT_Normal", -1180, 1200, DEFAULT_NORMAL)
        batch_orm = tex_param(material, "T_BatchVT_ORM", -1180, 1360)
        batch_light_info = tex_param(material, "T_BatchVT_LightInfo", -1180, 1520)

        scalar_defaults = {
            "TierMaterialQuality": 3.0,
            "MaxRuntimeBlendLayers": 3.0,
            "LayerBWeight": 1.0,
            "LayerCWeight": 1.0,
            "DynamicOverlayQuality": 3.0,
            "MaterialLightQuality": 3.0,
            "MaterialLightMaxLightInfoCount": 4.0,
            "LightInfoIntensity": 0.0,
            "UseBakedResult": 0.0,
        }
        scalar_refs = {}
        y = -360
        for name, default in scalar_defaults.items():
            scalar_refs[name] = scalar_param(material, name, default, -520, y)
            y += 120

        height_b_mask = mask(material, "Height_B_R", True, False, False, False, -560, 620)
        height_c_mask = mask(material, "Height_C_R", True, False, False, False, -560, 740)
        orm_r = mask(material, "ORM_R_AO", True, False, False, False, -560, 300)
        orm_g = mask(material, "ORM_G_Roughness", False, True, False, False, -560, 420)
        orm_b = mask(material, "ORM_B_Metallic", False, False, True, False, -560, 540)
        light_rgb = mask(material, "LightInfo_RGB", True, True, True, False, -560, 880)
        batch_ao = mask(material, "BatchVT_ORM_R_AO", True, False, False, False, -800, 1360)
        batch_roughness = mask(material, "BatchVT_ORM_G_Roughness", False, True, False, False, -800, 1480)
        batch_metallic = mask(material, "BatchVT_ORM_B_Metallic", False, False, True, False, -800, 1600)
        batch_light_rgb = mask(material, "BatchVT_LightInfo_RGB", True, True, True, False, -800, 1720)
        mask_input = input_names(orm_r)[0]
        for src, dst in [
            [height_b, height_b_mask],
            [height_c, height_c_mask],
            [orm_a, orm_r],
            [orm_a, orm_g],
            [orm_a, orm_b],
            [light_info, light_rgb],
            [batch_orm, batch_ao],
            [batch_orm, batch_roughness],
            [batch_orm, batch_metallic],
            [batch_light_info, batch_light_rgb],
        ]:
            connect_expr(src, "", dst, mask_input)

        one_layer = const_expr(material, 1.0, -260, -240)
        two_layers = const_expr(material, 2.0, -260, -100)
        layer_b_enabled = saturate(
            material,
            subtract(material, scalar_refs["MaxRuntimeBlendLayers"], one_layer, -80, -240),
            120,
            -240)
        layer_c_enabled = saturate(
            material,
            subtract(material, scalar_refs["MaxRuntimeBlendLayers"], two_layers, -80, -100),
            120,
            -100)
        layer_b_height_alpha = multiply(material, scalar_refs["LayerBWeight"], height_b_mask, -260, 540)
        layer_c_height_alpha = multiply(material, scalar_refs["LayerCWeight"], height_c_mask, -260, 680)
        layer_b_alpha = multiply(material, layer_b_height_alpha, layer_b_enabled, -40, 540)
        layer_c_alpha = multiply(material, layer_c_height_alpha, layer_c_enabled, -40, 680)
        base_ab = lerp(material, base_a, base_b, layer_b_alpha, 200, -260)
        base_abc = lerp(material, base_ab, base_c, layer_c_alpha, 440, -220)
        light_quality = multiply(material, light_rgb, scalar_refs["MaterialLightQuality"], -260, 900)
        light_emissive = multiply(material, light_quality, scalar_refs["LightInfoIntensity"], -40, 900)

        base_quality = quality_switch(material, base_abc, batch_base, base_ab, base_abc, base_abc, 700, -260)
        normal_quality = quality_switch(material, normal_a, batch_normal, normal_a, normal_a, normal_a, 700, 120)
        ao_quality = quality_switch(material, orm_r, batch_ao, orm_r, orm_r, orm_r, 700, 300)
        roughness_quality = quality_switch(material, orm_g, batch_roughness, orm_g, orm_g, orm_g, 700, 440)
        metallic_quality = quality_switch(material, orm_b, batch_metallic, orm_b, orm_b, orm_b, 700, 580)
        emissive_quality = quality_switch(material, light_emissive, batch_light_rgb, light_emissive, light_emissive, light_emissive, 700, 900)

        connect_output(base_quality, "", "MP_BaseColor")
        connect_output(normal_quality, "", "MP_Normal")
        connect_output(ao_quality, "", "MP_AmbientOcclusion")
        connect_output(roughness_quality, "", "MP_Roughness")
        connect_output(metallic_quality, "", "MP_Metallic")
        connect_output(emissive_quality, "", "MP_EmissiveColor")

        recompile(material)

    tiers = {
        "Epic": [3, 3, 1, 1, 3, 3, 4, 1, 0],
        "High": [2, 3, 1, 1, 2, 2, 2, 1, 0],
        "Mid": [1, 2, 1, 0, 1, 1, 1, 1, 0],
        "Low": [0, 1, 0, 0, 0, 0, 0, 0, 1],
    }

    instance_paths = []
    scalar_values = {}
    for tier, values in tiers.items():
        name = "MI_Env_MasterA_Source_" + tier
        inst = create_mi(INSTANCE_FOLDER, name, material)
        path = INSTANCE_FOLDER + "/" + name
        instance_paths.append(path)
        for param_name, param_value in zip(PARAMETERS, values):
            set_scalar(inst, param_name, param_value)
        scalar_values[tier] = {}
        for param_name in PARAMETERS:
            scalar_values[tier][param_name] = get_scalar(inst, param_name)

    saved = save([MATERIAL_PATH] + instance_paths)
    parameter_list = list_parameters(material)
    output_properties = ["MP_BaseColor", "MP_Normal", "MP_AmbientOcclusion", "MP_Roughness", "MP_Metallic", "MP_EmissiveColor"]
    output_wiring = {}
    for prop in output_properties:
        output_wiring[prop] = property_input(material, prop)

    return {
        "material": MATERIAL_PATH,
        "created_material": created_material,
        "force_rebuild": FORCE_REBUILD,
        "graph_updated": graph_updated,
        "missing_parameters_before_update": missing_params,
        "missing_outputs_before_update": missing_outputs,
        "instances": instance_paths,
        "saved": saved,
        "parameters": parameter_list,
        "scalar_values": scalar_values,
        "output_wiring": output_wiring
    }
'@

$programmaticScript = $programmaticScript.Replace(
    "__FORCE_REBUILD__",
    $(if ($ForceRebuild.IsPresent) { "True" } else { "False" }))

$executionResult = Invoke-ToolViaWrapper `
    -ToolsetName "editor_toolset.toolsets.programmatic.ProgrammaticToolset" `
    -ToolName "execute_tool_script" `
    -Arguments @{ script = $programmaticScript }

$result = $executionResult.returnValue | ConvertFrom-Json

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58MaterialTemplateMcp_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$textureParameters = @($result.parameters | Where-Object { $_.type -eq "Texture" } | ForEach-Object { $_.name })
$scalarParameters = @($result.parameters | Where-Object { $_.type -eq "Scalar" } | ForEach-Object { $_.name })

$tierRows = @(
    "| Tier | TierMaterialQuality | MaxRuntimeBlendLayers | LayerBWeight | LayerCWeight | DynamicOverlayQuality | MaterialLightQuality | MaterialLightMaxLightInfoCount | LightInfoIntensity | UseBakedResult |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"
)
foreach ($tier in @("Epic", "High", "Mid", "Low")) {
    $values = $result.scalar_values.$tier
    $tierRows += "| $tier | $($values.TierMaterialQuality) | $($values.MaxRuntimeBlendLayers) | $($values.LayerBWeight) | $($values.LayerCWeight) | $($values.DynamicOverlayQuality) | $($values.MaterialLightQuality) | $($values.MaterialLightMaxLightInfoCount) | $($values.LightInfoIntensity) | $($values.UseBakedResult) |"
}

$outputRows = @("| Material output | Connected expression | Output pin |", "| --- | --- | --- |")
foreach ($prop in @("MP_BaseColor", "MP_Normal", "MP_AmbientOcclusion", "MP_Roughness", "MP_Metallic", "MP_EmissiveColor")) {
    $input = $result.output_wiring.$prop
    $expressionPath = if ($input.expression) { [string]$input.expression.refPath } else { "" }
    $outputRows += "| $prop | $expressionPath | $($input.output_name) |"
}

$reportTime = Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz'
$reportLines = @(
    "# UE5.8 Material Template MCP Setup",
    "",
    "- Time: $reportTime",
    "- Repo: $RepoRoot",
    "- MCP server: $ServerUrl",
    "- Environment language: $($environment.returnValue.language)",
    "- Force rebuild: $($result.force_rebuild)",
    "- Created source material this run: $($result.created_material)",
    "- Updated source graph this run: $($result.graph_updated)",
    "- Saved assets: $($result.saved)",
    "",
    "## Created Assets",
    "",
    "| Asset | Result |",
    "| --- | --- |",
    "| $($result.material) | Created or updated through MCP |"
)

foreach ($instancePath in $result.instances) {
    $reportLines += "| $instancePath | Created or updated through MCP |"
}

$reportLines += @(
    "",
    "## Parameter Evidence",
    "",
    "| Type | Parameters |",
    "| --- | --- |",
    "| Scalar | $($scalarParameters -join ', ') |",
    "| Texture | $($textureParameters -join ', ') |",
    "",
    "## Output Wiring Evidence",
    ""
) + $outputRows + @(
    "",
    "## Batch VT Contract",
    "",
    "- Low material quality path is wired through UE `Quality Switch` nodes to the `T_BatchVT_*` texture parameters.",
    "- Generated batch/packaging tools must replace `T_BatchVT_BaseColor`, `T_BatchVT_Normal`, `T_BatchVT_ORM`, and `T_BatchVT_LightInfo` with the UDIM/SVT/VT atlas outputs for the merged proxy.",
    "- Epic and High keep the full Source A/B/C height-blend path; Medium uses the reduced Source A/B path; Low uses the baked BatchVT path.",
    "",
    "## Tier Instance Values",
    ""
) + $tierRows + @(
    "",
    "## Limits",
    "",
    "- This creates the Source material interface template, not the final ground/wall production material network.",
    "- The Source shader graph uses UE material nodes: BaseColor A/B/C texture samples, Height B/C alpha, ORM channel masks, LightInfo emissive scaling, and UE native `Quality Switch` nodes.",
    "- This proves the Source material has a baked BatchVT contract slot, but final mesh merge, UDIM/SVT tile generation, and `M_Env_Baked_VTAtlas` visual parity still need a production batch validation pass.",
    "- Full project asset audit, lighting budget, and final validation automation remain deferred."
)

$reportText = $reportLines -join [Environment]::NewLine
$reportText | Set-Content -LiteralPath $reportPath -Encoding UTF8
$reportText | Set-Content -LiteralPath $latestPath -Encoding UTF8

Write-Output "Wrote material template MCP setup report: $reportPath"
Write-Output "Updated latest material template MCP setup report: $latestPath"
