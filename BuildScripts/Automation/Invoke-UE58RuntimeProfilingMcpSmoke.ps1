param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [string]$ServerUrl = "http://127.0.0.1:8765/mcp",
    [int]$TimeoutSec = 60,
    [switch]$CapturePie,
    [switch]$SkipPie
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingSmoke"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58RuntimeProfilingMcpSmoke_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$viewportPngPath = Join-Path $OutputRoot "runtime_profiling_viewport.png"

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
                name = "devkit-ue58-runtime-profiling-smoke"
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
    }

    return $sessionHeaders
}

$mcpHeaders = New-McpSession

function Invoke-EditorTool {
    param(
        [string]$ToolName,
        [hashtable]$Arguments
    )

    $callArguments = @{
        toolset_name = "EditorToolset.EditorAppToolset"
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

    $responseText = Invoke-McpRequest -Headers $mcpHeaders -Payload $payload
    $outer = $responseText | ConvertFrom-Json
    if ($outer.result.isError) {
        $message = ($outer.result.content | ForEach-Object { $_.text }) -join "`n"
        throw $message
    }

    $text = ($outer.result.content | Select-Object -First 1).text
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $null
    }
    return ($text | ConvertFrom-Json)
}

function Save-ToolsetImage {
    param(
        [object]$Image,
        [string]$Path
    )

    if (-not $Image -or [string]::IsNullOrWhiteSpace($Image.data)) {
        throw "MCP returned an empty image."
    }

    $bytes = [Convert]::FromBase64String($Image.data)
    [IO.File]::WriteAllBytes($Path, $bytes)
    return $bytes.Length
}

$cvarNames = @(
    "sg.GlobalIlluminationQuality",
    "r.ScreenPercentage",
    "r.Lumen.DiffuseIndirect.Allow",
    "r.MeshDrawCommands.LogDynamicInstancingStats",
    "t.MaxFPS"
)

$cvarRows = @()
foreach ($name in $cvarNames) {
    try {
        $result = Invoke-EditorTool -ToolName "SearchCVars" -Arguments @{ name = $name }
        $returnValue = [string]$result.returnValue
        $found = -not [string]::IsNullOrWhiteSpace($returnValue) -and $returnValue -match [regex]::Escape($name)
        $cvarRows += [pscustomobject]@{
            Name = $name
            Found = $found
            DetailLength = $returnValue.Length
            Error = ""
        }
    }
    catch {
        $cvarRows += [pscustomobject]@{
            Name = $name
            Found = $false
            DetailLength = 0
            Error = $_.Exception.Message
        }
    }
}

$pieStatus = "Skipped"
$viewportBytes = 0
$shouldCapturePie = $CapturePie -and (-not $SkipPie)
if ($shouldCapturePie) {
    try {
        $isRunning = Invoke-EditorTool -ToolName "IsPIERunning" -Arguments @{}
        if ($isRunning.returnValue -eq $true) {
            Invoke-EditorTool -ToolName "StopPIE" -Arguments @{} | Out-Null
        }

        Invoke-EditorTool -ToolName "StartPIE" -Arguments @{
            options = @{
                bSimulate = $true
                playMode = "PlayMode_Simulate"
                warmupSeconds = 2
            }
        } | Out-Null

        $camera = Invoke-EditorTool -ToolName "GetCameraTransform" -Arguments @{}
        $capture = Invoke-EditorTool -ToolName "CaptureViewport" -Arguments @{
            captureTransform = $camera.returnValue
            bShowUI = $false
        }
        $viewportBytes = Save-ToolsetImage -Image $capture.returnValue.image -Path $viewportPngPath

        Invoke-EditorTool -ToolName "StopPIE" -Arguments @{} | Out-Null
        $pieStatus = "Captured"
    }
    catch {
        $pieStatus = "Failed: $($_.Exception.Message)"
        try {
            Invoke-EditorTool -ToolName "StopPIE" -Arguments @{} | Out-Null
        }
        catch {
        }
    }
}

$missingCvars = $cvarRows | Where-Object { -not $_.Found }
$status = if ($missingCvars.Count -eq 0 -and ((-not $shouldCapturePie) -or $pieStatus -eq "Captured")) { "Ready" } else { "Incomplete" }

$lines = @(
    "# UE5.8 Runtime Profiling MCP Smoke",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- MCP server: $ServerUrl",
    "- Status: $status",
    "- PIE viewport capture: $pieStatus",
    "- Viewport PNG: $(if ($viewportBytes -gt 0) { "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingSmoke\runtime_profiling_viewport.png" } else { "(not captured)" })",
    "- Viewport bytes: $viewportBytes",
    "- Limitation: this is a readiness smoke, not a GPU measurement. Final acceptance still requires stat/profilegpu captures from the runtime profiling plan.",
    "",
    "## CVar Search Results",
    "",
    "| CVar | Found | DetailLength | Error |",
    "| --- | --- | ---: | --- |"
)

foreach ($row in $cvarRows) {
    $name = $row.Name
    $found = $row.Found
    $detailLength = $row.DetailLength
    $errorText = $row.Error.Replace("|", "\|")
    $lines += "| ``$name`` | $found | $detailLength | $errorText |"
}

$lines += @(
    "",
    "## Next Measurement Gate",
    "",
    "- Run each scenario from ``UE58RuntimeProfilingPlanReport.md`` in the same loaded map/camera state.",
    "- Collect ``stat unit``, ``stat rhi``, ``stat scenerendering``, ``stat gpu``, ``r.MeshDrawCommands.LogDynamicInstancingStats 1``, and ``profilegpu`` output.",
    "- Compare ``Baseline_LumenOff_NoBatch``, ``LumenLite_NoBatch``, ``BatchProxy_LumenOff``, and ``BatchProxy_LumenLite`` before enabling handheld Lumen by default."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote runtime profiling MCP smoke: $reportPath"
Write-Output "Updated latest runtime profiling MCP smoke: $latestPath"
if ($status -ne "Ready") {
    exit 1
}
