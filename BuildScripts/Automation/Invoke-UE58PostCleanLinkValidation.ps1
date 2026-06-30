param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = "",
    [string]$OutputRoot = "",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string]$Cluster = "Prison_S_01_SourceProxy",
    [string]$Material = "/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building",
    [string]$Camera = "RepresentativeCamera",
    [int]$TimeoutSec = 300,
    [switch]$RunReportCommandlets,
    [switch]$RunMaterialBatchDryRun
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$uprojectPath = Join-Path $RepoRoot "DevKit.uproject"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\PostCleanLinkValidation"
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
        if (Test-Path -LiteralPath $editorCmdPath) {
            return [pscustomobject]@{
                Root = (Resolve-Path -LiteralPath $candidate).Path
                EditorCmd = (Resolve-Path -LiteralPath $editorCmdPath).Path
            }
        }
    }

    throw "UE5.8 UnrealEditor-Cmd.exe was not found. Set -EngineRoot or UE58_ENGINE_ROOT."
}

function Read-TextIfExists {
    param([string]$Path)
    if (Test-Path -LiteralPath $Path) {
        return Get-Content -LiteralPath $Path -Raw
    }
    return ""
}

function Get-LatestUbtBuildStatus {
    $ubtLogPath = Join-Path $env:LOCALAPPDATA "UnrealBuildTool\Log.txt"
    $text = Read-TextIfExists -Path $ubtLogPath
    $tail = if ([string]::IsNullOrWhiteSpace($text)) { @() } else { $text -split "`r?`n" | Select-Object -Last 160 }
    $resultLine = $tail | Where-Object { $_ -match "^Result:" } | Select-Object -Last 1
    $linkBlocked = (($tail | Select-String -Pattern "LNK1104|cannot open file.*UnrealEditor-.*\.dll|being used by another process").Count -gt 0)

    return [pscustomobject]@{
        LogPath = $ubtLogPath
        Result = if ($resultLine) { $resultLine.Trim() } else { "Unknown" }
        LinkBlocked = $linkBlocked
        Tail = (($tail | Select-Object -Last 16) -join " ; ")
    }
}

function Get-MarkdownBulletValue {
    param(
        [string]$Text,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    $pattern = "(?m)^- $([regex]::Escape($Label)): (.+)$"
    if ($Text -match $pattern) {
        return $Matches[1].Trim()
    }

    return ""
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

function Invoke-ReportCommandlet {
    param(
        [string]$EditorExe,
        [string]$Name,
        [string[]]$Args,
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
    ) + $Args

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

    return [pscustomobject]@{
        Commandlet = $Name
        Completed = $completed
        ExitCode = if ($completed) { $process.ExitCode } else { $null }
        TimedOut = (-not $completed)
        LogPath = $LogPath
        Command = "$EditorExe $($arguments -join ' ')"
    }
}

$engine = Resolve-UE58EditorCmd -RequestedRoot $EngineRoot
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$runRoot = Join-Path $OutputRoot $timestamp
$reportPath = Join-Path $OutputRoot "UE58PostCleanLinkValidation_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$savedLogsRoot = Join-Path $RepoRoot "Saved\Logs"
$buildStatus = Get-LatestUbtBuildStatus
$openEditors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)
$shouldRun = $RunReportCommandlets -or $RunMaterialBatchDryRun
$blockedByOpenEditor = $shouldRun -and $openEditors.Count -gt 0
$commandletResults = @()
$dryRunOutput = @()
$dryRunStatus = "NotRun"

$commandletAvailabilityScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Test-UE58CommandletAvailability.ps1"
$commandletAvailabilityLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\CommandletAvailability\LATEST.md"
$materialBatchDryRunScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58MaterialBatchDryRun.ps1"
$materialBatchDryRunLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\MaterialBatchDryRun\LATEST.md"

if ($RunReportCommandlets -and -not $blockedByOpenEditor) {
    New-Item -ItemType Directory -Force -Path $runRoot | Out-Null
    $commandletResults += Invoke-ReportCommandlet `
        -EditorExe $engine.EditorCmd `
        -Name "GraphicsSettingsWidgetSetup" `
        -Args @() `
        -LogPath (Join-Path $savedLogsRoot "Codex_GraphicsSettingsWidgetSetup_DryRun.log")
    $commandletResults += Invoke-ReportCommandlet `
        -EditorExe $engine.EditorCmd `
        -Name "MaterialBatchMaterialAudit" `
        -Args @("-Material=$Material") `
        -LogPath (Join-Path $savedLogsRoot "Codex_MaterialBatchMaterialAudit.log")
    $commandletResults += Invoke-ReportCommandlet `
        -EditorExe $engine.EditorCmd `
        -Name "UE58RuntimeProfilingPlan" `
        -Args @("-Map=$Map", "-Cluster=$Cluster", "-Camera=$Camera") `
        -LogPath (Join-Path $savedLogsRoot "Codex_UE58RuntimeProfilingPlan.log")

    if (Test-Path -LiteralPath $commandletAvailabilityScriptPath) {
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $commandletAvailabilityScriptPath -RepoRoot $RepoRoot | Out-Null
    }
}

$commandletAvailabilityText = Read-TextIfExists -Path $commandletAvailabilityLatestPath
$commandletAvailabilityStatus = Get-MarkdownBulletValue -Text $commandletAvailabilityText -Label "Status"
$commandletsAvailable = $commandletAvailabilityStatus -eq "Available"

if ($RunMaterialBatchDryRun -and -not $blockedByOpenEditor) {
    if (-not $commandletsAvailable) {
        $dryRunStatus = "SkippedUntilCommandletsAvailable"
    }
    elseif (Test-Path -LiteralPath $materialBatchDryRunScriptPath) {
        try {
            $dryRunOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $materialBatchDryRunScriptPath -RepoRoot $RepoRoot -Run 2>&1
            $dryRunStatus = "Ran"
        }
        catch {
            $dryRunStatus = "Failed: $($_.Exception.Message)"
        }
    }
    else {
        $dryRunStatus = "SkippedMissingScript"
    }
}

$materialBatchDryRunText = Read-TextIfExists -Path $materialBatchDryRunLatestPath
$materialBatchDryRunLatestStatus = Get-MarkdownBulletValue -Text $materialBatchDryRunText -Label "Status"
$materialBatchDryRunReady = $materialBatchDryRunLatestStatus -eq "DryRunCaptured" -and
    (Get-MarkdownBulletValue -Text $materialBatchDryRunText -Label "Actual layer evidence") -eq "True" -and
    (Get-MarkdownBulletValue -Text $materialBatchDryRunText -Label "Source/Proxy asset evidence") -eq "True" -and
    (Get-MarkdownBulletValue -Text $materialBatchDryRunText -Label "Residency risk evidence") -eq "True"

$reportCommandletsReady = $commandletsAvailable
$status = if ($blockedByOpenEditor) {
    "BlockedByOpenEditor"
}
elseif ($RunMaterialBatchDryRun -and $materialBatchDryRunReady) {
    "DryRunCaptured"
}
elseif ($RunReportCommandlets -and $reportCommandletsReady) {
    "ReportCommandletsAvailable"
}
elseif ($shouldRun) {
    "RanButNeedsFollowup"
}
elseif ($reportCommandletsReady) {
    "ReportCommandletsAvailable"
}
elseif ($openEditors.Count -gt 0) {
    "WaitingForEditorClose"
}
else {
    "PreparedForCleanLink"
}

$lines = @(
    "# UE5.8 Post Clean Link Validation",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Engine root: $($engine.Root)",
    "- Editor commandlet executable: $($engine.EditorCmd)",
    "- Run report commandlets requested: $RunReportCommandlets",
    "- Run MaterialBatch dry-run requested: $RunMaterialBatchDryRun",
    "- Open UnrealEditor process count: $($openEditors.Count)",
    "- Blocked by open editor: $blockedByOpenEditor",
    "- Latest UBT result: $($buildStatus.Result)",
    "- Latest UBT link blocked: $($buildStatus.LinkBlocked)",
    "- Commandlet availability status: $commandletAvailabilityStatus",
    "- Report commandlets available: $reportCommandletsReady",
    "- MaterialBatch dry-run run status: $dryRunStatus",
    "- MaterialBatch dry-run latest status: $materialBatchDryRunLatestStatus",
    "- MaterialBatch dry-run ready: $materialBatchDryRunReady",
    "- Map: $Map",
    "- Cluster: $Cluster",
    "- Material: $Material",
    "- Camera: $Camera",
    "",
    "## Open Editor Processes",
    "",
    "| Id | ProcessName | Path |",
    "| ---: | --- | --- |"
)

if ($openEditors.Count -gt 0) {
    foreach ($process in $openEditors) {
        $lines += "| $($process.Id) | $($process.ProcessName) | ``$($process.Path)`` |"
    }
}
else {
    $lines += "|  |  | (none) |"
}

$lines += @(
    "",
    "## Report Commandlet Results",
    "",
    "| Commandlet | Completed | ExitCode | TimedOut | Log |",
    "| --- | --- | ---: | --- | --- |"
)

if ($commandletResults.Count -gt 0) {
    foreach ($result in $commandletResults) {
        $lines += "| $($result.Commandlet) | $($result.Completed) | $($result.ExitCode) | $($result.TimedOut) | ``$(ConvertTo-RelativePath -Path $result.LogPath)`` |"
    }
}
else {
    $lines += "| GraphicsSettingsWidgetSetup | False |  | False | (not run) |"
    $lines += "| MaterialBatchMaterialAudit | False |  | False | (not run) |"
    $lines += "| UE58RuntimeProfilingPlan | False |  | False | (not run) |"
}

$lines += @(
    "",
    "## Planned Next Commands",
    "",
    '```powershell',
    ('BuildScripts\Automation\Invoke-UE58PostCleanLinkValidation.ps1 -RepoRoot "{0}" -RunReportCommandlets' -f $RepoRoot),
    ('BuildScripts\Automation\Invoke-UE58PostCleanLinkValidation.ps1 -RepoRoot "{0}" -RunReportCommandlets -RunMaterialBatchDryRun' -f $RepoRoot),
    '```',
    "",
    "## MaterialBatch Dry-Run Output",
    "",
    '```text'
)

if ($dryRunOutput.Count -gt 0) {
    $lines += $dryRunOutput
}
else {
    $lines += "(not run)"
}

$lines += @(
    '```',
    "",
    "## Acceptance Notes",
    "",
    "- This script never closes UnrealEditor.",
    '- If UnrealEditor is open and a run switch is passed, the script reports `BlockedByOpenEditor` and does not launch commandlets.',
    '- `-RunReportCommandlets` reruns `GraphicsSettingsWidgetSetup`, `MaterialBatchMaterialAudit`, and `UE58RuntimeProfilingPlan`, then refreshes `CommandletAvailability/LATEST.md`.',
    '- `-RunMaterialBatchDryRun` only runs the real-cluster dry-run after `CommandletAvailability/LATEST.md` is `Available`.',
    "- The MaterialBatch dry-run remains accepted only when StreamingLevel layer, Source/Proxy asset, and residency evidence are all true.",
    "",
    "## UBT Tail",
    "",
    '```text',
    $buildStatus.Tail,
    '```'
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote post-clean-link validation report: $reportPath"
Write-Output "Updated latest post-clean-link validation: $latestPath"
Write-Output "Status: $status"

if ($blockedByOpenEditor) {
    exit 1
}
