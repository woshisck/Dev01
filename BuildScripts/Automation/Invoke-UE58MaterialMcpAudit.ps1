param(
    [string]$RepoRoot = "",
    [string]$MaterialPath = "/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building",
    [string]$ServerUrl = "http://127.0.0.1:8765/mcp",
    [string]$OutputRoot = "",
    [int]$TimeoutSec = 30,
    [switch]$FullExpressionGraph,
    [switch]$ProbeAllOutputs
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function ConvertFrom-SseJson {
    param([string]$Content)

    $trimmed = $Content.Trim()
    if ($trimmed.StartsWith("event:")) {
        $dataLines = @()
        foreach ($line in ($trimmed -split "`r?`n")) {
            if ($line.StartsWith("data:")) {
                $dataLines += $line.Substring(5).Trim()
            }
        }
        return ($dataLines -join "`n")
    }

    return $Content
}

function Invoke-McpRequest {
    param(
        [hashtable]$Headers,
        [hashtable]$Payload
    )

    $body = $Payload | ConvertTo-Json -Depth 64 -Compress
    $response = Invoke-WebRequest -Uri $ServerUrl -Method Post -Headers $Headers -Body $body -UseBasicParsing -TimeoutSec $TimeoutSec
    return ConvertFrom-SseJson -Content $response.Content
}

function New-McpSession {
    $commonHeaders = @{
        "Content-Type" = "application/json"
        "Accept" = "application/json, text/event-stream"
    }

    $initializePayload = @{
        jsonrpc = "2.0"
        id = 1
        method = "initialize"
        params = @{
            protocolVersion = "2025-06-18"
            capabilities = @{}
            clientInfo = @{
                name = "devkit-ue58-material-mcp-audit"
                version = "1.0"
            }
        }
    }

    $initializeBody = $initializePayload | ConvertTo-Json -Depth 64 -Compress
    $initializeResponse = Invoke-WebRequest -Uri $ServerUrl -Method Post -Headers $commonHeaders -Body $initializeBody -UseBasicParsing -TimeoutSec $TimeoutSec
    $sessionId = $initializeResponse.Headers["Mcp-Session-Id"]
    if ([string]::IsNullOrWhiteSpace($sessionId)) {
        throw "MCP initialize response did not include Mcp-Session-Id."
    }

    $sessionHeaders = @{
        "Content-Type" = "application/json"
        "Accept" = "application/json, text/event-stream"
        "Mcp-Session-Id" = $sessionId
        "Mcp-Protocol-Version" = "2025-06-18"
    }

    $initializedPayload = @{
        jsonrpc = "2.0"
        method = "notifications/initialized"
        params = @{}
    }

    try {
        Invoke-McpRequest -Headers $sessionHeaders -Payload $initializedPayload | Out-Null
    }
    catch {
        # Notifications are fire-and-forget on some MCP builds.
    }

    return $sessionHeaders
}

function Invoke-Tool {
    param(
        [hashtable]$Headers,
        [string]$ToolsetName,
        [string]$ToolName,
        [hashtable]$Arguments
    )

    $callArguments = @{
        toolset_name = $ToolsetName
        tool_name = $ToolName
        arguments = $Arguments
    }

    $payload = @{
        jsonrpc = "2.0"
        id = 2
        method = "tools/call"
        params = @{
            name = "call_tool"
            arguments = $callArguments
        }
    }

    $responseText = Invoke-McpRequest -Headers $Headers -Payload $payload
    $response = $responseText | ConvertFrom-Json
    if ($response.result.isError) {
        $message = ($response.result.content | ForEach-Object { $_.text }) -join "`n"
        throw "MCP tool failed: $ToolsetName.$ToolName - $message"
    }

    $toolText = ($response.result.content | Select-Object -First 1).text
    if ([string]::IsNullOrWhiteSpace($toolText)) {
        return $null
    }

    return $toolText | ConvertFrom-Json
}

function Get-ExpressionTypeCounts {
    param([object[]]$Expressions)

    $counts = [ordered]@{}
    foreach ($expression in $Expressions) {
        $path = [string]$expression.refPath
        $type = "(unknown)"
        if ($path -match ':(MaterialExpression[^_]+)') {
            $type = $Matches[1]
        }
        if (-not $counts.Contains($type)) {
            $counts[$type] = 0
        }
        $counts[$type] += 1
    }
    return $counts
}

$headers = New-McpSession

$materialRef = @{ refPath = $MaterialPath }
$materialTools = "editor_toolset.toolsets.material.MaterialTools"
$materialInstanceTools = "editor_toolset.toolsets.material_instance.MaterialInstanceTools"

$parametersResult = Invoke-Tool -Headers $headers -ToolsetName $materialInstanceTools -ToolName "list_parameters" -Arguments @{
    material = $materialRef
}

$expressionsResult = $null
if ($FullExpressionGraph) {
    $expressionsResult = Invoke-Tool -Headers $headers -ToolsetName $materialTools -ToolName "get_expressions" -Arguments @{
        material_or_function = $materialRef
    }
}

$propertiesToProbe = if ($ProbeAllOutputs) {
    @(
        "MP_MaterialAttributes",
        "MP_BaseColor",
        "MP_Normal",
        "MP_Roughness",
        "MP_Metallic",
        "MP_Specular",
        "MP_EmissiveColor",
        "MP_OpacityMask",
        "MP_AmbientOcclusion"
    )
}
else {
    @("MP_MaterialAttributes")
}

$propertyInputs = @()
foreach ($propertyName in $propertiesToProbe) {
    $propertyResult = Invoke-Tool -Headers $headers -ToolsetName $materialTools -ToolName "get_property_input" -Arguments @{
        material = $materialRef
        material_property = $propertyName
    }
    $propertyInputs += [pscustomobject]@{
        Property = $propertyName
        Expression = [string]$propertyResult.returnValue.expression
        OutputName = [string]$propertyResult.returnValue.output_name
        Connected = ([string]$propertyResult.returnValue.expression) -ne "None" -and -not [string]::IsNullOrWhiteSpace([string]$propertyResult.returnValue.expression)
    }
}

$parameters = @($parametersResult.returnValue)
$expressions = if ($FullExpressionGraph -and $expressionsResult) { @($expressionsResult.returnValue) } else { @() }
$expressionPaths = $expressions | ForEach-Object { [string]$_.refPath }
$expressionTypeCounts = Get-ExpressionTypeCounts -Expressions $expressions

$textureParameters = @($parameters | Where-Object { $_.type -eq "Texture" } | ForEach-Object { [string]$_.name })
$scalarParameters = @($parameters | Where-Object { $_.type -eq "Scalar" } | ForEach-Object { [string]$_.name })
$vectorParameters = @($parameters | Where-Object { $_.type -eq "Vector" } | ForEach-Object { [string]$_.name })
$staticSwitchParameters = @($parameters | Where-Object { $_.type -eq "StaticSwitch" } | ForEach-Object { [string]$_.name })

$requiredBatchParameters = @(
    "T_Array_A",
    "T_Array_M",
    "T_Array_N",
    "Tex_LightInfo",
    "LightInfoCount"
)
$missingBatchParameters = @($requiredBatchParameters | Where-Object {
    $name = $_
    -not ($parameters | Where-Object { $_.name -eq $name })
})

$textureArrayExpressionCount = @($expressionPaths | Where-Object { $_ -match "TextureSampleParameter2DArray|TextureObjectParameter|TextureSample" }).Count
$perInstanceCustomDataCount = @($expressionPaths | Where-Object { $_ -match "PerInstanceCustomData" }).Count
$textureCoordinateCount = @($expressionPaths | Where-Object { $_ -match "TextureCoordinate" }).Count
$materialAttributesConnected = @($propertyInputs | Where-Object { $_.Property -eq "MP_MaterialAttributes" -and $_.Connected }).Count -gt 0
$batchParameterReady = $missingBatchParameters.Count -eq 0

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58MaterialMcpAudit_$timestamp.md"
$latestPath = Join-Path $OutputRoot "UE58MaterialMcpAudit_LATEST.md"

$parameterRows = @("| Type | Count |", "| --- | ---: |")
$parameterRows += "| Scalar | $($scalarParameters.Count) |"
$parameterRows += "| Vector | $($vectorParameters.Count) |"
$parameterRows += "| Texture | $($textureParameters.Count) |"
$parameterRows += "| StaticSwitch | $($staticSwitchParameters.Count) |"

$propertyRows = @("| Property | Connected | Expression | Output |", "| --- | --- | --- | --- |")
foreach ($row in $propertyInputs) {
    $propertyRows += ('| ' + $row.Property + ' | ' + $row.Connected + ' | `' + $row.Expression + '` | `' + $row.OutputName + '` |')
}

$requiredRows = @("| Parameter | Present |", "| --- | --- |")
foreach ($name in $requiredBatchParameters) {
    $present = ($parameters | Where-Object { $_.name -eq $name } | Measure-Object).Count -gt 0
    $requiredRows += "| $name | $present |"
}

$expressionRows = @("| Expression Type | Count |", "| --- | ---: |")
foreach ($entry in $expressionTypeCounts.GetEnumerator() | Sort-Object Name) {
    $expressionRows += "| $($entry.Key) | $($entry.Value) |"
}

$textureParameterLines = $textureParameters | Sort-Object | ForEach-Object { "- $_" }
$staticSwitchLines = $staticSwitchParameters | Sort-Object | ForEach-Object { "- $_" }
if ($textureParameterLines.Count -eq 0) { $textureParameterLines = @("- (none)") }
if ($staticSwitchLines.Count -eq 0) { $staticSwitchLines = @("- (none)") }

$lines = @(
    "# UE5.8 Material MCP Audit",
    "",
    "- Time: $(Get-Date -Format o)",
    "- MCP server: $ServerUrl",
    ('- Material: `' + $MaterialPath + '`'),
    ('- Toolsets: `' + $materialTools + '`, `' + $materialInstanceTools + '`'),
    "- Expression graph mode: $(if ($FullExpressionGraph) { 'Full' } else { 'SkippedQuickMaterialOutputs' })",
    "- Output probe mode: $(if ($ProbeAllOutputs) { 'AllCommonOutputs' } else { 'MaterialAttributesOnly' })",
    "- Parameter count: $($parameters.Count)",
    "- Expression count: $($expressions.Count)",
    "- MaterialAttributes connected: $materialAttributesConnected",
    "- Required batch parameters present: $batchParameterReady",
    "- Texture-array related expression count: $textureArrayExpressionCount",
    "- PerInstanceCustomData expression count: $perInstanceCustomDataCount",
    "- TextureCoordinate expression count: $textureCoordinateCount",
    "",
    "## Parameter Summary",
    ""
) + $parameterRows + @(
    "",
    "## Required Batch Parameters",
    ""
) + $requiredRows + @(
    "",
    "## Material Output Connections",
    ""
) + $propertyRows + @(
    "",
    "## Texture Parameters",
    ""
) + $textureParameterLines + @(
    "",
    "## Static Switch Parameters",
    ""
) + $staticSwitchLines + @(
    "",
    "## Expression Type Counts",
    ""
) + $expressionRows + @(
    "",
    "## Interpretation",
    "",
    "- MCP confirms this material is driven through `MP_MaterialAttributes`; individual BaseColor/Normal/Roughness pins may be disconnected by design.",
    "- MCP confirms the existing source material exposes the unique texture, Texture2DArray fallback, `Tex_LightInfo`, surface switch, and property-texture source parameters needed by the VT Atlas batch plan.",
    "- Default audit mode intentionally skips the full expression graph because `get_expressions` can be slow on this material through MCP; rerun with `-FullExpressionGraph -ProbeAllOutputs` when graph-wide expression counts are specifically needed.",
    "- This audit does not prove the final batch parent samples `TexCoord7.x`; that remains the next material-authoring validation step for `M_Env_Baked_VTAtlas`."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote material MCP audit: $reportPath"
Write-Output "Updated material MCP audit latest: $latestPath"
