param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = "",
    [string]$OutputRoot = "",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string]$Cluster = "Prison_S_01_SourceProxy",
    [string]$Tier = "Mid",
    [string]$TextureBackend = "VTAtlas",
    [string]$SurfaceKind = "MixedStatic",
    [string]$BakePolicy = "StaticBake",
    [string]$RequireTag = "EnvBatch.",
    [int]$MaxActors = 2000,
    [int]$TimeoutSec = 600,
    [switch]$Run,
    [switch]$AllowStaleBinaries
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$uprojectPath = Join-Path $RepoRoot "DevKit.uproject"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\MaterialBatchDryRun"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Resolve-UE58EditorCmd {
    param([string]$RequestedRoot)

    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $candidates += $RequestedRoot
    }
    if (-not [string]::IsNullOrWhiteSpace($env:UE58_ENGINE_ROOT)) {
        $candidates += $env:UE58_ENGINE_ROOT
    }
    $candidates += "Z:\GZA_Software\RealityCapture\UE_5.8"
    $candidates += "D:\UE\UE_5.8"
    $candidates += "D:\Epic Library\UE_5.8"

    foreach ($candidate in $candidates) {
        $editorCmdPath = Join-Path $candidate "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
        $editorPath = Join-Path $candidate "Engine\Binaries\Win64\UnrealEditor.exe"
        if (Test-Path -LiteralPath $editorCmdPath) {
            return [pscustomobject]@{
                Root = (Resolve-Path -LiteralPath $candidate).Path
                Editor = (Resolve-Path -LiteralPath $editorCmdPath).Path
            }
        }
        if (Test-Path -LiteralPath $editorPath) {
            return [pscustomobject]@{
                Root = (Resolve-Path -LiteralPath $candidate).Path
                Editor = (Resolve-Path -LiteralPath $editorPath).Path
            }
        }
    }

    throw "UE5.8 editor executable was not found. Set -EngineRoot or UE58_ENGINE_ROOT."
}

function Get-LatestUbtBuildStatus {
    $ubtLogPath = Join-Path $env:LOCALAPPDATA "UnrealBuildTool\Log.txt"
    if (-not (Test-Path -LiteralPath $ubtLogPath)) {
        return [pscustomobject]@{
            LogPath = $ubtLogPath
            Result = "Missing"
            Detail = "No UnrealBuildTool log was found."
            LinkBlocked = $false
        }
    }

    $tail = Get-Content -LiteralPath $ubtLogPath -Tail 160
    $resultLine = $tail | Where-Object { $_ -match "^Result:" } | Select-Object -Last 1
    $linkBlocked = (($tail | Select-String -Pattern "LNK1104|cannot open file.*UnrealEditor-.*\.dll|being used by another process").Count -gt 0)
    return [pscustomobject]@{
        LogPath = $ubtLogPath
        Result = if ($resultLine) { $resultLine.Trim() } else { "Unknown" }
        Detail = (($tail | Select-Object -Last 12) -join " ; ")
        LinkBlocked = $linkBlocked
    }
}

function Invoke-Commandlet {
    param(
        [string]$EditorExe,
        [string]$Name,
        [string[]]$CommandletArgs,
        [string]$LogPath
    )

    $arguments = @(
        "`"$uprojectPath`"",
        "-run=$Name",
        "-unattended",
        "-nop4",
        "-nosplash",
        "-NoSound",
        "-NullRHI",
        "-AbsLog=`"$LogPath`""
    ) + $CommandletArgs

    $process = Start-Process -FilePath $EditorExe -ArgumentList $arguments -PassThru -WindowStyle Hidden
    $completed = $process.WaitForExit($TimeoutSec * 1000)
    if (-not $completed) {
        try {
            $process.Kill()
            $process.WaitForExit(30000) | Out-Null
        }
        catch {
        }
    }

    $logText = if (Test-Path -LiteralPath $LogPath) { Get-Content -LiteralPath $LogPath -Raw } else { "" }
    $commandletResultSucceeded = $logText -match "Commandlet $Name.*finished execution \(result 0\)"
    $mcpPortBindError = $logText -match "HttpListener unable to bind to 127\.0\.0\.1:8765"
    $effectiveSucceeded = $completed -and (($process.ExitCode -eq 0) -or ($commandletResultSucceeded -and $mcpPortBindError))

    return [pscustomobject]@{
        Name = $Name
        Completed = $completed
        ExitCode = if ($completed) { $process.ExitCode } else { $null }
        EffectiveSucceeded = $effectiveSucceeded
        CommandletResultSucceeded = $commandletResultSucceeded
        McpPortBindError = $mcpPortBindError
        TimedOut = (-not $completed)
        LogPath = $LogPath
        Command = "$EditorExe $($arguments -join ' ')"
    }
}

function ConvertTo-RelativePath {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }
    if ($Path.StartsWith($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Path.Substring($RepoRoot.Length).TrimStart("\", "/")
    }
    return $Path
}

$engine = Resolve-UE58EditorCmd -RequestedRoot $EngineRoot
$buildStatus = Get-LatestUbtBuildStatus
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$runRoot = Join-Path $OutputRoot $timestamp
$reportPath = Join-Path $OutputRoot "UE58MaterialBatchDryRun_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$results = @()
$runBlockedByBuild = $false

$auditArgs = @(
    "-Map=$Map",
    "-MaxActors=$MaxActors"
)
$buildArgs = @(
    "-Map=$Map",
    "-Cluster=$Cluster",
    "-Tier=$Tier",
    "-TextureBackend=$TextureBackend",
    "-SurfaceKind=$SurfaceKind",
    "-BakePolicy=$BakePolicy",
    "-SourceProxyExclusivityGroup=$Cluster",
    "-RequireTag=$RequireTag",
    "-MaxActors=$MaxActors",
    "-ReportStaticDecals",
    "-ValidateSourceProxyExclusivity"
)

if ($Run) {
    if ($buildStatus.Result -match "Failed" -and -not $AllowStaleBinaries) {
        $runBlockedByBuild = $true
    }
    else {
        New-Item -ItemType Directory -Force -Path $runRoot | Out-Null
        $results += Invoke-Commandlet -EditorExe $engine.Editor -Name "MaterialBatchAudit" -CommandletArgs $auditArgs -LogPath (Join-Path $runRoot "MaterialBatchAudit.log")
        $results += Invoke-Commandlet -EditorExe $engine.Editor -Name "MaterialBatchBuild" -CommandletArgs $buildArgs -LogPath (Join-Path $runRoot "MaterialBatchBuild.log")
    }
}

$sharedAuditReport = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchAuditReport.md"
$sharedBuildReport = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchBuildReport.md"
$sharedBuildManifest = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchBuildManifest.json"
$buildReportText = if (Test-Path -LiteralPath $sharedBuildReport) { Get-Content -LiteralPath $sharedBuildReport -Raw } else { "" }
$manifestText = if (Test-Path -LiteralPath $sharedBuildManifest) { Get-Content -LiteralPath $sharedBuildManifest -Raw } else { "" }
$manifest = $null
if (-not [string]::IsNullOrWhiteSpace($manifestText)) {
    try {
        $manifest = $manifestText | ConvertFrom-Json
    }
    catch {
        $manifest = $null
    }
}
$sourceProxyAssetReadinessEvidence = $buildReportText -match "Source/Proxy Asset Readiness" -and $manifestText -match '"sourceProxyAssetReadiness"\s*:'
$sourceProxyAssetConfigSetEvidence = $buildReportText -match "Source/Proxy Asset Config Set" -and $manifestText -match '"sourceProxyAssetConfigSet"\s*:'
$sourceProxyAssetEvidence = $sourceProxyAssetReadinessEvidence -and $sourceProxyAssetConfigSetEvidence
$residencyRiskEvidence = $buildReportText -match "VT/Non-VT Residency Risk Plan" -and $manifestText -match '"residencyRiskPlan"\s*:'
$manifestSourceFound = if ($manifest -and $manifest.summary) { [int]$manifest.summary.sourceFound } else { -1 }
$manifestBatchCandidates = if ($manifest -and $manifest.summary) { [int]$manifest.summary.batchCandidates } else { -1 }
$manifestEnvBatchActorCount = if ($manifest -and $manifest.tagDiagnostics) { [int]$manifest.tagDiagnostics.actorCount } else { -1 }
$manifestSourceActorCount = if ($manifest -and $manifest.tagDiagnostics) { [int]$manifest.tagDiagnostics.sourceActorCount } else { -1 }
$manifestProxyActorCount = if ($manifest -and $manifest.tagDiagnostics) { [int]$manifest.tagDiagnostics.proxyActorCount } else { -1 }
$manifestBakedActorCount = if ($manifest -and $manifest.tagDiagnostics) { [int]$manifest.tagDiagnostics.bakedActorCount } else { -1 }
$manifestLayerBackend = if ($manifest -and $manifest.sourceProxyLayerReadiness -and $manifest.sourceProxyLayerReadiness.layerBackend) { [string]$manifest.sourceProxyLayerReadiness.layerBackend } elseif ($manifest -and $manifest.layerBackend) { [string]$manifest.layerBackend } else { "" }
$manifestLayerEntryCount = if ($manifest -and $manifest.sourceProxyLayerReadiness) { [int]$manifest.sourceProxyLayerReadiness.entryCount } else { -1 }
$manifestLayerReadyEntryCount = if ($manifest -and $manifest.sourceProxyLayerReadiness) { [int]$manifest.sourceProxyLayerReadiness.readyEntryCount } else { -1 }
$manifestActualLayerMatchCount = if ($manifest -and $manifest.sourceProxyLayerReadiness) { [int]$manifest.sourceProxyLayerReadiness.actualLayerMatchCount } else { -1 }
$manifestMissingActualLayerCount = if ($manifest -and $manifest.sourceProxyLayerReadiness) { [int]$manifest.sourceProxyLayerReadiness.missingActualLayerCount } else { -1 }
$manifestLayerConflictEntryCount = if ($manifest -and $manifest.sourceProxyLayerReadiness) { [int]$manifest.sourceProxyLayerReadiness.conflictEntryCount } else { -1 }
$actualLayerEvidence = (
    $buildReportText -match "Source/Proxy/Baked Layer Readiness" `
        -and $manifestLayerBackend -eq "StreamingLevel" `
        -and $manifestLayerEntryCount -gt 0 `
        -and $manifestLayerReadyEntryCount -gt 0 `
        -and $manifestActualLayerMatchCount -ge $manifestLayerReadyEntryCount `
        -and $manifestMissingActualLayerCount -eq 0 `
        -and $manifestLayerConflictEntryCount -eq 0
)
$envBatchTagEvidence = $manifestEnvBatchActorCount -gt 0
$taggedSourceCandidateEvidence = $manifestSourceFound -gt 0 -and $manifestBatchCandidates -gt 0

$successfulRuns = @($results | Where-Object { $_.EffectiveSucceeded }).Count
$status = if (-not $Run) {
    "Prepared"
}
elseif ($runBlockedByBuild) {
    "BlockedByBuild"
}
elseif ($successfulRuns -eq 2 -and $actualLayerEvidence -and $sourceProxyAssetEvidence -and $residencyRiskEvidence) {
    "DryRunCaptured"
}
elseif ($successfulRuns -gt 0) {
    "Partial"
}
else {
    "Failed"
}

$firstBlockingEvidence = if ($status -eq "DryRunCaptured") {
    "None"
}
elseif ($successfulRuns -lt 2) {
    "CommandletRunIncomplete"
}
elseif (-not $envBatchTagEvidence) {
    "MissingEnvBatchTags"
}
elseif (-not $taggedSourceCandidateEvidence) {
    "NoTaggedSourceCandidates"
}
elseif (-not $actualLayerEvidence) {
    "MissingActualLayerEvidence"
}
elseif (-not $sourceProxyAssetEvidence) {
    "MissingSourceProxyAssetEvidence"
}
elseif (-not $residencyRiskEvidence) {
    "MissingResidencyRiskEvidence"
}
else {
    "PartialUnknown"
}

$lines = @(
    "# UE5.8 MaterialBatch Real Cluster Dry Run",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Engine root: $($engine.Root)",
    "- Editor executable: $($engine.Editor)",
    "- Run requested: $Run",
    "- Allow stale binaries: $AllowStaleBinaries",
    "- Status: $status",
    "- Build status: $($buildStatus.Result)",
    "- Build link blocked: $($buildStatus.LinkBlocked)",
    "- UBT log: $($buildStatus.LogPath)",
    "- Map: $Map",
    "- Cluster: $Cluster",
    "- Tier: $Tier",
    "- Texture backend: $TextureBackend",
    "- Surface kind: $SurfaceKind",
    "- Bake policy: $BakePolicy",
    "- Require tag: $RequireTag",
    "- Max actors: $MaxActors",
    "- Shared audit report: $(ConvertTo-RelativePath -Path $sharedAuditReport)",
    "- Shared build report: $(ConvertTo-RelativePath -Path $sharedBuildReport)",
    "- Shared build manifest: $(ConvertTo-RelativePath -Path $sharedBuildManifest)",
    "- First blocking evidence: $firstBlockingEvidence",
    "- Manifest source found: $manifestSourceFound",
    "- Manifest batch candidates: $manifestBatchCandidates",
    "- EnvBatch actor evidence: $envBatchTagEvidence",
    "- EnvBatch actor count: $manifestEnvBatchActorCount",
    "- EnvBatch source actor count: $manifestSourceActorCount",
    "- EnvBatch proxy actor count: $manifestProxyActorCount",
    "- EnvBatch baked actor count: $manifestBakedActorCount",
    "- Tagged source candidate evidence: $taggedSourceCandidateEvidence",
    "- Layer evidence mode: $manifestLayerBackend",
    "- Actual layer evidence: $actualLayerEvidence",
    "- Source/Proxy layer entry count: $manifestLayerEntryCount",
    "- Source/Proxy layer ready entry count: $manifestLayerReadyEntryCount",
    "- Source/Proxy actual layer match count: $manifestActualLayerMatchCount",
    "- Source/Proxy missing actual layer count: $manifestMissingActualLayerCount",
    "- Source/Proxy layer conflict entry count: $manifestLayerConflictEntryCount",
    "- Source/Proxy asset evidence: $sourceProxyAssetEvidence",
    "- Source/Proxy asset readiness evidence: $sourceProxyAssetReadinessEvidence",
    "- Source/Proxy asset config set evidence: $sourceProxyAssetConfigSetEvidence",
    "- Residency risk evidence: $residencyRiskEvidence",
    "",
    "## Commandlet Results",
    "",
    "| Commandlet | Completed | ExitCode | EffectiveSucceeded | McpPortBindError | TimedOut | Log |",
    "| --- | --- | ---: | --- | --- | --- | --- |"
)

if ($results.Count -gt 0) {
    foreach ($result in $results) {
        $lines += "| $($result.Name) | $($result.Completed) | $($result.ExitCode) | $($result.EffectiveSucceeded) | $($result.McpPortBindError) | $($result.TimedOut) | ``$(ConvertTo-RelativePath -Path $result.LogPath)`` |"
    }
}
else {
    $lines += "| MaterialBatchAudit | False |  | False | False | False | (not run) |"
    $lines += "| MaterialBatchBuild | False |  | False | False | False | (not run) |"
}

$lines += @(
    "",
    "## Planned Commands",
    "",
    "### MaterialBatchAudit",
    "",
    '```text',
    "$($engine.Editor) `"$uprojectPath`" -run=MaterialBatchAudit -unattended -nop4 -nosplash -NoSound -NullRHI -Map=$Map -MaxActors=$MaxActors",
    '```',
    "",
    "### MaterialBatchBuild",
    "",
    '```text',
    "$($engine.Editor) `"$uprojectPath`" -run=MaterialBatchBuild -unattended -nop4 -nosplash -NoSound -NullRHI $($buildArgs -join ' ')",
    '```',
    "",
    "## Acceptance Notes",
    "",
    "- This script is the repeatable entry point for the first real-cluster MaterialBatch dry-run.",
    "- `Status: Prepared` means the command harness is ready but no Unreal commandlet was launched.",
    "- `Status: BlockedByBuild` means the latest UBT result failed; rerun after closing UnrealEditor and completing a clean link, or pass `-AllowStaleBinaries` only for an explicit stale-binary investigation.",
    "- `Status: DryRunCaptured` requires successful MaterialBatchAudit and MaterialBatchBuild commandlets plus actual StreamingLevel layer readiness, Source/Proxy asset readiness, Source/Proxy asset config set, and residency risk evidence in the shared report/manifest.",
    "- `EffectiveSucceeded: True` with `McpPortBindError: True` means the commandlet wrote its report and logged commandlet result 0, but the process exit code was polluted by a secondary MCP listener port conflict from an already-open editor.",
    "- `First blocking evidence: MissingEnvBatchTags` means the commandlets ran with current binaries, but the selected pilot cluster has no `EnvBatch.*` tagged actors in the target map yet.",
    "- `First blocking evidence: NoTaggedSourceCandidates` means some EnvBatch tags exist, but the build command did not find tagged static source candidates for the requested cluster.",
    "- The default run is dry-run/report-only. It does not pass any `-Apply*` switch and does not generate or modify `.uasset` assets.",
    "",
    "## Build Tail",
    "",
    '```text',
    $buildStatus.Detail,
    '```'
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote MaterialBatch dry-run report: $reportPath"
Write-Output "Updated latest MaterialBatch dry-run: $latestPath"
Write-Output "Status: $status"

if ($Run -and $status -in @("BlockedByBuild", "Failed")) {
    exit 1
}
