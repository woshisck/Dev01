param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\CommandletAvailability"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

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
    }
}

function Get-CommandletProbe {
    param(
        [string]$Name,
        [string]$LogPath
    )

    $text = Read-TextIfExists -Path $LogPath
    $classMissing = $text -match "$([regex]::Escape($Name))Commandlet looked like a commandlet, but we could not find the class"
    $wroteReport = $text -match "wrote shared report|Report:"
    $attempted = -not [string]::IsNullOrWhiteSpace($text)

    [pscustomobject]@{
        Name = $Name
        LogPath = $LogPath
        Attempted = $attempted
        ClassMissing = $classMissing
        WroteReport = $wroteReport
        Status = if ($wroteReport) { "ReportWritten" } elseif ($classMissing) { "ClassMissingUntilCleanLink" } elseif ($attempted) { "AttemptedOtherResult" } else { "NotAttempted" }
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

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58CommandletAvailability_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$buildStatus = Get-LatestUbtBuildStatus

$savedLogsRoot = Join-Path $RepoRoot "Saved\Logs"
$probes = @(
    Get-CommandletProbe -Name "GraphicsSettingsWidgetSetup" -LogPath (Join-Path $savedLogsRoot "Codex_GraphicsSettingsWidgetSetup_DryRun.log")
    Get-CommandletProbe -Name "MaterialBatchMaterialAudit" -LogPath (Join-Path $savedLogsRoot "Codex_MaterialBatchMaterialAudit.log")
    Get-CommandletProbe -Name "UE58RuntimeProfilingPlan" -LogPath (Join-Path $savedLogsRoot "Codex_UE58RuntimeProfilingPlan.log")
)

$missingClassCount = @($probes | Where-Object { $_.ClassMissing }).Count
$reportWrittenCount = @($probes | Where-Object { $_.WroteReport }).Count
$status = if ($missingClassCount -gt 0 -and $buildStatus.LinkBlocked) {
    "BlockedByCleanLink"
}
elseif ($missingClassCount -gt 0) {
    "ClassMissing"
}
elseif ($reportWrittenCount -eq $probes.Count) {
    "Available"
}
else {
    "Incomplete"
}

$lines = @(
    "# UE5.8 Commandlet Availability",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Latest UBT result: $($buildStatus.Result)",
    "- Latest UBT link blocked: $($buildStatus.LinkBlocked)",
    "- UBT log: $($buildStatus.LogPath)",
    "- Missing commandlet class count: $missingClassCount",
    "- Report-written commandlet count: $reportWrittenCount",
    "",
    "## Probe Results",
    "",
    "| Commandlet | Attempted | ClassMissing | WroteReport | Status | Log |",
    "| --- | --- | --- | --- | --- | --- |"
)

foreach ($probe in $probes) {
    $lines += "| $($probe.Name) | $($probe.Attempted) | $($probe.ClassMissing) | $($probe.WroteReport) | $($probe.Status) | ``$(ConvertTo-RelativePath -Path $probe.LogPath)`` |"
}

$lines += @(
    "",
    "## Interpretation",
    "",
    "- `ClassMissingUntilCleanLink` means the source code contains the commandlet, but the currently loaded editor binaries do not include it yet.",
    "- Do not treat these commandlet reports as missing design work if the latest UBT result is link-blocked by an open UnrealEditor process.",
    "- After saving and closing UnrealEditor, complete a clean DevKitEditor link, then rerun the report-only commandlets before real dry-run or upload gates.",
    "- This report does not close or modify the running editor."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote commandlet availability report: $reportPath"
Write-Output "Updated commandlet availability latest: $latestPath"
Write-Output "Status: $status"
