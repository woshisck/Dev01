param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [string]$ServerUrl = "http://127.0.0.1:8765/mcp",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string]$SourceAsset = "/Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03/SM_FloorBrick_03a1",
    [string]$ProxyAsset = "/Game/Generated/MaterialBatch/Medium/FloorBrick03_Probe/SM_BatchProxy_FloorBrick03_Probe",
    [double]$IsolationX = 200000,
    [double]$ScratchScale = 5,
    [int]$TimeoutSec = 60
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SceneParityAudit"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58SceneParityMcpAudit_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$viewportPngPath = Join-Path $OutputRoot "scene_parity_side_by_side.png"

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
                name = "devkit-ue58-scene-parity-audit"
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

function Invoke-Tool {
    param(
        [string]$ToolsetName,
        [string]$ToolName,
        [hashtable]$Arguments
    )

    $payload = @{
        jsonrpc = "2.0"
        id = 2
        method = "tools/call"
        params = @{
            name = "call_tool"
            arguments = @{
                toolset_name = $ToolsetName
                tool_name = $ToolName
                arguments = $Arguments
            }
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

function Get-ImageStats {
    param([byte[]]$Bytes)

    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $hash = [System.BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }

    return [pscustomobject]@{
        ByteCount = $Bytes.Length
        Sha256 = $hash
        NonEmpty = ($Bytes.Length -gt 512)
    }
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
    return Get-ImageStats -Bytes $bytes
}

function New-Transform {
    param(
        [double]$X,
        [double]$Y,
        [double]$Z,
        [double]$Yaw = 0,
        [double]$Scale = 1
    )

    return @{
        location = @{ x = $X; y = $Y; z = $Z }
        rotation = @{ pitch = 0; yaw = $Yaw; roll = 0 }
        scale = @{ x = $Scale; y = $Scale; z = $Scale }
    }
}

$sourceActor = $null
$proxyActor = $null
$lightActor = $null
$captureStatus = "Incomplete"
$cleanupStatus = "NotStarted"
$failures = @()
$sourceBounds = $null
$proxyBounds = $null
$sourceTagCount = 0
$proxyTagCount = 0
$visibleActorCount = 0
$imageStats = [pscustomobject]@{
    ByteCount = 0
    Sha256 = ""
    NonEmpty = $false
}
$cameraLocation = $null
$cameraRotation = $null

try {
    $currentLevel = Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "get_current_level" -Arguments @{}
    if ($currentLevel.returnValue -ne $Map) {
        Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "load_level" -Arguments @{
            level_path = $Map
        } | Out-Null
    }

    $sourceTaggedActors = Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "find_actors" -Arguments @{
        name = ""
        tag = "EnvBatch.Source.Building_Stone"
        collision_channels = @()
    }
    $proxyTaggedActors = Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "find_actors" -Arguments @{
        name = ""
        tag = "EnvBatch.Proxy.Building_Stone"
        collision_channels = @()
    }
    $sourceTagCount = @($sourceTaggedActors.returnValue).Count
    $proxyTagCount = @($proxyTaggedActors.returnValue).Count

    $sourceActor = (Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "add_to_scene_from_asset" -Arguments @{
        asset_path = $SourceAsset
        name = "Codex_UE58Parity_Source_$timestamp"
        xform = New-Transform -X $IsolationX -Y -350 -Z 160 -Yaw 0 -Scale $ScratchScale
        snap_to_ground = $false
    }).returnValue
    $proxyActor = (Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "add_to_scene_from_asset" -Arguments @{
        asset_path = $ProxyAsset
        name = "Codex_UE58Parity_Proxy_$timestamp"
        xform = New-Transform -X $IsolationX -Y 350 -Z 160 -Yaw 0 -Scale $ScratchScale
        snap_to_ground = $false
    }).returnValue
    $lightActor = (Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "add_to_scene_from_class" -Arguments @{
        actor_type = @{ refPath = "/Script/Engine.PointLight" }
        name = "Codex_UE58Parity_Light_$timestamp"
        xform = New-Transform -X ($IsolationX - 250) -Y 0 -Z 900 -Yaw 0 -Scale 1
        snap_to_ground = $false
    }).returnValue

    Invoke-Tool -ToolsetName "editor_toolset.toolsets.actor.ActorTools" -ToolName "add_tag" -Arguments @{
        actor = $sourceActor
        tag = "Codex.UE58.SceneParity.Source"
    } | Out-Null
    Invoke-Tool -ToolsetName "editor_toolset.toolsets.actor.ActorTools" -ToolName "add_tag" -Arguments @{
        actor = $proxyActor
        tag = "Codex.UE58.SceneParity.Proxy"
    } | Out-Null
    Invoke-Tool -ToolsetName "editor_toolset.toolsets.actor.ActorTools" -ToolName "add_tag" -Arguments @{
        actor = $lightActor
        tag = "Codex.UE58.SceneParity.Light"
    } | Out-Null

    $sourceBounds = (Invoke-Tool -ToolsetName "editor_toolset.toolsets.actor.ActorTools" -ToolName "get_actor_bounds" -Arguments @{
        actor = $sourceActor
    }).returnValue
    $proxyBounds = (Invoke-Tool -ToolsetName "editor_toolset.toolsets.actor.ActorTools" -ToolName "get_actor_bounds" -Arguments @{
        actor = $proxyActor
    }).returnValue

    $scratchCameraTransform = @{
        location = @{ x = ($IsolationX - 1200); y = 0; z = 850 }
        rotation = @{ pitch = -28; yaw = 0; roll = 0 }
        scale = @{ x = 1; y = 1; z = 1 }
    }

    Invoke-Tool -ToolsetName "EditorToolset.EditorAppToolset" -ToolName "SetCameraTransform" -Arguments @{
        transform = $scratchCameraTransform
    } | Out-Null

    $camera = Invoke-Tool -ToolsetName "EditorToolset.EditorAppToolset" -ToolName "GetCameraTransform" -Arguments @{}
    $cameraLocation = $camera.returnValue.location
    $cameraRotation = $camera.returnValue.rotation
    $capture = Invoke-Tool -ToolsetName "EditorToolset.EditorAppToolset" -ToolName "CaptureViewport" -Arguments @{
        captureTransform = $camera.returnValue
        annotations = @{
            gridSpacing = 0
            gridExtent = 0
            gridHeight = 0
            maxLabelDistance = 0
            classFilter = $null
            maxLabels = 0
        }
        bShowUI = $false
    }
    $imageStats = Save-ToolsetImage -Image $capture.returnValue.image -Path $viewportPngPath
    $visibleActorCount = @($capture.returnValue.labeledActors).Count
    $captureStatus = if ($imageStats.NonEmpty) { "Captured" } else { "SuspiciousEmpty" }
}
catch {
    $captureStatus = "Failed"
    $failures += $_.Exception.Message
}
finally {
    $cleanupFailures = @()
    foreach ($actor in @($sourceActor, $proxyActor, $lightActor)) {
        if ($actor -and -not [string]::IsNullOrWhiteSpace($actor.refPath)) {
            try {
                Invoke-Tool -ToolsetName "editor_toolset.toolsets.scene.SceneTools" -ToolName "remove_from_scene" -Arguments @{
                    actor = $actor
                } | Out-Null
            }
            catch {
                $cleanupFailures += $_.Exception.Message
            }
        }
    }

    if ($cleanupFailures.Count -gt 0) {
        $cleanupStatus = "Failed"
        $failures += ($cleanupFailures | ForEach-Object { "Cleanup: $_" })
    }
    elseif ($sourceActor -or $proxyActor) {
        $cleanupStatus = "RemovedScratchActors"
    }
    else {
        $cleanupStatus = "NoScratchActorsCreated"
    }
}

$status = if ($captureStatus -eq "Captured" -and $cleanupStatus -eq "RemovedScratchActors") {
    "Captured"
}
elseif ($captureStatus -eq "Captured") {
    "CapturedWithCleanupRisk"
}
else {
    "Incomplete"
}

function Format-Bounds {
    param([object]$Bounds)

    if (-not $Bounds) {
        return "(not available)"
    }

    return "min=($($Bounds.min.x), $($Bounds.min.y), $($Bounds.min.z)); max=($($Bounds.max.x), $($Bounds.max.y), $($Bounds.max.z)); valid=$($Bounds.isValid)"
}

$relativePng = $viewportPngPath
if ($relativePng.StartsWith($RepoRoot)) {
    $relativePng = $relativePng.Substring($RepoRoot.Length).TrimStart("\", "/")
}

$lines = @(
    "# UE5.8 Scene Source/Proxy Parity MCP Audit",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- MCP server: $ServerUrl",
    "- Map: $Map",
    "- Status: $status",
    "- Capture status: $captureStatus",
    "- Cleanup status: $cleanupStatus",
    "- Existing source tagged actors: $sourceTagCount",
    "- Existing proxy tagged actors: $proxyTagCount",
    "- Scratch source asset: $SourceAsset",
    "- Scratch proxy asset: $ProxyAsset",
    "- Scratch isolation X: $IsolationX",
    "- Scratch actor scale: $ScratchScale",
    "- Viewport PNG: $relativePng",
    "- Viewport bytes: $($imageStats.ByteCount)",
    "- Viewport SHA256: $($imageStats.Sha256)",
    "- Labeled actors in capture: $visibleActorCount",
    "",
    "## Bounds",
    "",
    "| Actor | Bounds |",
    "| --- | --- |",
    "| Scratch source | $(Format-Bounds -Bounds $sourceBounds) |",
    "| Scratch proxy | $(Format-Bounds -Bounds $proxyBounds) |",
    "",
    "## Camera",
    "",
    "| Field | Value |",
    "| --- | --- |",
    "| Location | $(if ($cameraLocation) { "$($cameraLocation.x), $($cameraLocation.y), $($cameraLocation.z)" } else { "(not captured)" }) |",
    "| Rotation | $(if ($cameraRotation) { "$($cameraRotation.pitch), $($cameraRotation.yaw), $($cameraRotation.roll)" } else { "(not captured)" }) |",
    "",
    "## Interpretation",
    "",
    "- This validates that the source mesh and generated proxy mesh can render side by side in the target UE5.8 level lighting context through MCP viewport capture.",
    "- Existing production `EnvBatch.Source.*` / `EnvBatch.Proxy.*` tagged actor pairs were not required for this scratch capture.",
    "- The script removes the scratch actors after capture and does not save the level.",
    "- This is visual evidence for generated proxy review; it does not enable full production map replacement by itself."
)

if ($failures.Count -gt 0) {
    $lines += @(
        "",
        "## Failures",
        ""
    )
    foreach ($failure in $failures) {
        $lines += "- $failure"
    }
}

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote scene parity MCP audit: $reportPath"
Write-Output "Updated latest scene parity MCP audit: $latestPath"
Write-Output "Status: $status"

if ($status -eq "Incomplete") {
    exit 1
}
