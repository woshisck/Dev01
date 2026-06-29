param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [int]$MaxHistoryReports = 10,
    [switch]$SkipMcpProbe
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

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58PerformanceGoalHeartbeat_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$auditScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Analyze-UE58PerformanceInputs.ps1"
$mcpToolScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58McpTool.ps1"
$submissionGateScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Write-UE58SubmissionGateReport.ps1"

Push-Location $RepoRoot
try {
    $branch = (git rev-parse --abbrev-ref HEAD) 2>$null
    $head = (git rev-parse --short HEAD) 2>$null
    $status = (git status --short --branch) 2>$null
    $stash = (git stash list -n 5) 2>$null
}
finally {
    Pop-Location
}

$mcpStatus = "Skipped"
$mcpToolsets = "Skipped"
if (-not $SkipMcpProbe) {
    try {
        $tcp = Test-NetConnection -ComputerName 127.0.0.1 -Port 8765 -InformationLevel Quiet -WarningAction SilentlyContinue
        if ($tcp) {
            $mcpStatus = "Listening on 127.0.0.1:8765"
            if (Test-Path -LiteralPath $mcpToolScriptPath) {
                try {
                    $mcpToolOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $mcpToolScriptPath -Mode ListToolsets -TimeoutSec 30 2>&1
                    $mcpToolsets = ($mcpToolOutput -join " ; ")
                    if ($mcpToolsets.Length -gt 1200) {
                        $mcpToolsets = $mcpToolsets.Substring(0, 1200) + "..."
                    }
                }
                catch {
                    $mcpToolsets = "Toolset query failed: $($_.Exception.Message)"
                }
            }
            else {
                $mcpToolsets = "Toolset query skipped: Invoke-UE58McpTool.ps1 not found"
            }
        }
        else {
            $mcpStatus = "Not listening on 127.0.0.1:8765"
            $mcpToolsets = "Unavailable: MCP port is not listening"
        }
    }
    catch {
        $mcpStatus = "Probe failed: $($_.Exception.Message)"
        $mcpToolsets = "Unavailable: MCP probe failed"
    }
}

$auditStatus = "Skipped: audit script not found"
$auditLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAudit\LATEST.md"
if (Test-Path -LiteralPath $auditScriptPath) {
    try {
        $auditOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $auditScriptPath -RepoRoot $RepoRoot 2>&1
        $auditStatus = ($auditOutput -join " ; ")
    }
    catch {
        $auditStatus = "Audit failed: $($_.Exception.Message)"
    }
}

$buildStatus = "No UnrealBuildTool log found"
$ubtLogPath = Join-Path $env:LOCALAPPDATA "UnrealBuildTool\Log.txt"
if (Test-Path -LiteralPath $ubtLogPath) {
    try {
        $ubtTail = Get-Content -LiteralPath $ubtLogPath -Tail 80
        $resultLine = $ubtTail | Where-Object { $_ -match "^Result:" } | Select-Object -Last 1
        $targetLine = $ubtTail | Where-Object { $_ -match "Running UnrealBuildTool:" } | Select-Object -Last 1
        if ($resultLine) {
            $buildStatus = $resultLine.Trim()
            if ($targetLine) {
                $buildStatus = "$buildStatus ($($targetLine.Trim()))"
            }
        }
        else {
            $buildStatus = "UBT log exists but no recent Result line was found: $ubtLogPath"
        }
    }
    catch {
        $buildStatus = "Could not read UBT log: $($_.Exception.Message)"
    }
}

$submissionGateStatus = "Skipped: submission gate script not found"
$submissionGateLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\LATEST.md"
if (Test-Path -LiteralPath $submissionGateScriptPath) {
    try {
        $submissionGateOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $submissionGateScriptPath -RepoRoot $RepoRoot 2>&1
        $submissionGateStatus = ($submissionGateOutput -join " ; ")
    }
    catch {
        $submissionGateStatus = "Submission gate failed: $($_.Exception.Message)"
    }
}

$lines = @(
    "# UE5.8 Performance Goal Heartbeat",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Branch: $branch",
    "- HEAD: $head",
    "- MCP: $mcpStatus",
    "- MCP toolsets: $mcpToolsets",
    "- Static audit: $auditStatus",
    "- Static audit latest: $auditLatest",
    "- Latest UE build: $buildStatus",
    "- Submission gate: $submissionGateStatus",
    "- Submission gate latest: $submissionGateLatest",
    "",
    "## Active Objective",
    "",
    "Implement the UE5.8 art production, material batching, geometry merge, lighting, scene, and performance tier plan. Keep automation context recoverable if the conversation is interrupted.",
    "",
    "## Git Status",
    "",
    "~~~text"
)

if ($status) {
    $lines += $status
}
else {
    $lines += "(git status unavailable or empty)"
}

$lines += @(
    "~~~",
    "",
    "## Recent Stashes",
    "",
    "~~~text"
)

if ($stash) {
    $lines += $stash
}
else {
    $lines += "(no recent stash output)"
}

$lines += @(
    "~~~",
    "",
    "## Resume Checklist",
    "",
    "1. Read guide.md before changing files.",
    "2. Check git status --short --branch and do not revert unrelated changes.",
    "3. Review both UE58_ArtPerformanceTieringAndBatching.md and the non-English UE58 comprehensive plan under Docs.",
    "4. Continue from the latest unfinished step: MCP audit, batch commandlet implementation, graphics profile implementation, profiling, or compile-before-upload.",
    "5. Before upload/push, compile first and record the build result.",
    "",
    "## Suggested Resume Prompt",
    "",
    "Continue the UE5.8 performance/art batching goal. Read guide.md, this heartbeat report, git status, and the UE58 art/performance plan. Do not repeat completed edits or revert unrelated work. Continue from the latest unfinished implementation item and report verification evidence."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

if ($MaxHistoryReports -gt 0) {
    Get-ChildItem -LiteralPath $OutputRoot -Filter "UE58PerformanceGoalHeartbeat_*.md" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -Skip $MaxHistoryReports |
        Remove-Item -Force
}

Write-Output "Wrote heartbeat report: $reportPath"
Write-Output "Updated latest report: $latestPath"
