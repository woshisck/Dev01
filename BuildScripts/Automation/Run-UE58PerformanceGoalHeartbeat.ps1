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
$envBatchTagToolsScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Test-UE58EnvBatchTagTools.ps1"
$envBatchTagToolsLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\EnvBatchTagTools\LATEST.md"
$pilotClusterScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Select-UE58PilotCluster.ps1"
$pilotClusterLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\PilotCluster\LATEST.md"
$runtimeProfilingPlanFallbackScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Write-UE58RuntimeProfilingPlanFallback.ps1"
$runtimeProfilingPlanFallbackLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingPlanFallback\LATEST.md"
$commandletAvailabilityScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Test-UE58CommandletAvailability.ps1"
$commandletAvailabilityLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\CommandletAvailability\LATEST.md"
$postCleanLinkValidationScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58PostCleanLinkValidation.ps1"
$postCleanLinkValidationLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\PostCleanLinkValidation\LATEST.md"
$buildValidationScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58BuildValidation.ps1"
$buildValidationLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\BuildValidation\LATEST.md"
$runtimeProfilingCaptureScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58RuntimeProfilingCapture.ps1"
$runtimeProfilingCaptureLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture\LATEST.md"
$runtimeProfilingCapturePreparedRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture\Prepared"
$runtimeProfilingCapturePreparedLatest = Join-Path $runtimeProfilingCapturePreparedRoot "LATEST.md"
$materialBatchDryRunLatest = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\MaterialBatchDryRun\LATEST.md"
$continuationAutomationId = "ue58-epic-high-mid-low"
$continuationAutomationCadence = "5 hours"
$continuationAutomationRule = "RRULE:FREQ=HOURLY;INTERVAL=5"
$masterPlanMatch = Get-ChildItem -LiteralPath (Join-Path $RepoRoot "Docs") -Recurse -File -Filter "UE58_EpicHighMidLow_*.md" -ErrorAction SilentlyContinue | Select-Object -First 1
$masterPlanPath = if ($masterPlanMatch) { $masterPlanMatch.FullName } else { "(not found)" }
$guidePath = Join-Path $RepoRoot "guide.md"

$masterPlanText = ""
if ($masterPlanMatch) {
    $masterPlanText = Get-Content -LiteralPath $masterPlanPath -Raw
}

$governanceChecks = @(
    [pscustomobject]@{
        Check = "guide.md startup context"
        Passed = (Test-Path -LiteralPath $guidePath)
        Evidence = "guide.md exists at repo root."
    },
    [pscustomobject]@{
        Check = "Current master plan located"
        Passed = [bool]$masterPlanMatch
        Evidence = $masterPlanPath
    },
    [pscustomobject]@{
        Check = "Module ownership boundaries recorded"
        Passed = ($masterPlanText -match "Source/DevKit" -and $masterPlanText -match "Source/DevKitEditor" -and $masterPlanText -match "Config/DefaultDeviceProfiles.ini" -and $masterPlanText -match "Docs/GeneratedReports")
        Evidence = "Master plan records runtime, editor, config, and report ownership."
    },
    [pscustomobject]@{
        Check = "Generated assets must use commandlets/tools"
        Passed = ($masterPlanText -match "uasset" -and $masterPlanText -match "Commandlet")
        Evidence = "Master plan forbids manual binary asset edits and requires generated asset tooling."
    },
    [pscustomobject]@{
        Check = "Source/Proxy/Baked and residency gates tracked"
        Passed = ($masterPlanText -match "ResidencyRiskPlan" -and $masterPlanText -match "Source/Proxy/Baked layer readiness")
        Evidence = "Master plan requires layer readiness plus VT/non-VT residency gates."
    }
)

$failedGovernanceChecks = @($governanceChecks | Where-Object { -not $_.Passed })
$governanceStatus = if ($failedGovernanceChecks.Count -eq 0) { "Passed" } else { "NeedsAttention" }

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

$runtimeProfilingPlanFallbackStatus = "Skipped: runtime profiling plan fallback script not found"
if (Test-Path -LiteralPath $runtimeProfilingPlanFallbackScriptPath) {
    try {
        $runtimeProfilingPlanFallbackOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $runtimeProfilingPlanFallbackScriptPath -RepoRoot $RepoRoot 2>&1
        $runtimeProfilingPlanFallbackStatus = ($runtimeProfilingPlanFallbackOutput -join " ; ")
    }
    catch {
        $runtimeProfilingPlanFallbackStatus = "Runtime profiling plan fallback failed: $($_.Exception.Message)"
    }
}

$commandletAvailabilityStatus = "Skipped: commandlet availability script not found"
if (Test-Path -LiteralPath $commandletAvailabilityScriptPath) {
    try {
        $commandletAvailabilityOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $commandletAvailabilityScriptPath -RepoRoot $RepoRoot 2>&1
        $commandletAvailabilityStatus = ($commandletAvailabilityOutput -join " ; ")
    }
    catch {
        $commandletAvailabilityStatus = "Commandlet availability failed: $($_.Exception.Message)"
    }
}

$postCleanLinkValidationStatus = "Skipped: post-clean-link validation script not found"
if (Test-Path -LiteralPath $postCleanLinkValidationScriptPath) {
    try {
        $postCleanLinkValidationOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $postCleanLinkValidationScriptPath -RepoRoot $RepoRoot 2>&1
        $postCleanLinkValidationStatus = ($postCleanLinkValidationOutput -join " ; ")
    }
    catch {
        $postCleanLinkValidationStatus = "Post-clean-link validation failed: $($_.Exception.Message)"
    }
}

$buildValidationStatus = "Skipped: build validation script not found"
if (Test-Path -LiteralPath $buildValidationScriptPath) {
    try {
        $buildValidationOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $buildValidationScriptPath -RepoRoot $RepoRoot 2>&1
        $buildValidationStatus = ($buildValidationOutput -join " ; ")
    }
    catch {
        $buildValidationStatus = "Build validation failed: $($_.Exception.Message)"
    }
}

$runtimeProfilingCaptureStatus = "Skipped: runtime profiling capture script not found"
if (Test-Path -LiteralPath $runtimeProfilingCaptureScriptPath) {
    try {
        $runtimeProfilingCaptureOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $runtimeProfilingCaptureScriptPath -RepoRoot $RepoRoot -OutputRoot $runtimeProfilingCapturePreparedRoot 2>&1
        $runtimeProfilingCaptureStatus = ($runtimeProfilingCaptureOutput -join " ; ")
    }
    catch {
        $runtimeProfilingCaptureStatus = "Runtime profiling capture refresh failed: $($_.Exception.Message)"
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
$submissionGateMaterialBatchReady = "Unknown"
$submissionGateMaterialBatchSourceProxyEvidence = "Unknown"
if (Test-Path -LiteralPath $submissionGateScriptPath) {
    try {
        $submissionGateOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $submissionGateScriptPath -RepoRoot $RepoRoot 2>&1
        $submissionGateStatus = ($submissionGateOutput -join " ; ")
    }
    catch {
        $submissionGateStatus = "Submission gate failed: $($_.Exception.Message)"
    }
}
if (Test-Path -LiteralPath $submissionGateLatest) {
    try {
        $submissionGateText = Get-Content -LiteralPath $submissionGateLatest -Raw
        if ($submissionGateText -match "(?m)^- MaterialBatch dry-run ready: (.+)$") {
            $submissionGateMaterialBatchReady = $Matches[1].Trim()
        }
        if ($submissionGateText -match "(?m)^- MaterialBatch dry-run Source/Proxy asset evidence: (.+)$") {
            $submissionGateMaterialBatchSourceProxyEvidence = $Matches[1].Trim()
        }
    }
    catch {
        $submissionGateMaterialBatchReady = "Unreadable"
        $submissionGateMaterialBatchSourceProxyEvidence = "Unreadable"
    }
}

$envBatchTagToolsStatus = "Skipped: EnvBatch tag tools script not found"
if (Test-Path -LiteralPath $envBatchTagToolsScriptPath) {
    try {
        $envBatchTagToolsOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $envBatchTagToolsScriptPath -RepoRoot $RepoRoot 2>&1
        $envBatchTagToolsStatus = ($envBatchTagToolsOutput -join " ; ")
    }
    catch {
        $envBatchTagToolsStatus = "EnvBatch tag tools check failed: $($_.Exception.Message)"
    }
}

$pilotClusterStatus = "Skipped: pilot cluster script not found"
if (Test-Path -LiteralPath $pilotClusterScriptPath) {
    try {
        $pilotClusterOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $pilotClusterScriptPath -RepoRoot $RepoRoot 2>&1
        $pilotClusterStatus = ($pilotClusterOutput -join " ; ")
    }
    catch {
        $pilotClusterStatus = "Pilot cluster selection failed: $($_.Exception.Message)"
    }
}

$pilotClusterLatestStatus = "No pilot cluster report found"
if (Test-Path -LiteralPath $pilotClusterLatest) {
    try {
        $pilotClusterText = Get-Content -LiteralPath $pilotClusterLatest -Raw
        if ($pilotClusterText -match "(?m)^- Status: (.+)$") {
            $pilotClusterLatestStatus = $Matches[1].Trim()
        }
        else {
            $pilotClusterLatestStatus = "Report exists but no status line was found"
        }
    }
    catch {
        $pilotClusterLatestStatus = "Could not read pilot cluster report: $($_.Exception.Message)"
    }
}

$materialBatchDryRunStatus = "No MaterialBatch dry-run report found"
if (Test-Path -LiteralPath $materialBatchDryRunLatest) {
    try {
        $materialBatchDryRunText = Get-Content -LiteralPath $materialBatchDryRunLatest -Raw
        if ($materialBatchDryRunText -match "(?m)^- Status: (.+)$") {
            $materialBatchDryRunStatus = $Matches[1].Trim()
        }
        else {
            $materialBatchDryRunStatus = "Report exists but no status line was found"
        }
    }
    catch {
        $materialBatchDryRunStatus = "Could not read MaterialBatch dry-run report: $($_.Exception.Message)"
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
    "- Submission MaterialBatch dry-run ready: $submissionGateMaterialBatchReady",
    "- Submission Source/Proxy asset evidence: $submissionGateMaterialBatchSourceProxyEvidence",
    "- EnvBatch tag tools: $envBatchTagToolsStatus",
    "- EnvBatch tag tools latest: $envBatchTagToolsLatest",
    "- Pilot cluster: $pilotClusterLatestStatus",
    "- Pilot cluster refresh: $pilotClusterStatus",
    "- Pilot cluster latest: $pilotClusterLatest",
    "- Runtime profiling plan fallback: $runtimeProfilingPlanFallbackStatus",
    "- Runtime profiling plan fallback latest: $runtimeProfilingPlanFallbackLatest",
    "- Commandlet availability: $commandletAvailabilityStatus",
    "- Commandlet availability latest: $commandletAvailabilityLatest",
    "- Post-clean-link validation: $postCleanLinkValidationStatus",
    "- Post-clean-link validation latest: $postCleanLinkValidationLatest",
    "- Build validation: $buildValidationStatus",
    "- Build validation latest: $buildValidationLatest",
    "- Runtime profiling capture: $runtimeProfilingCaptureStatus",
    "- Runtime profiling capture latest: $runtimeProfilingCaptureLatest",
    "- Runtime profiling capture prepared latest: $runtimeProfilingCapturePreparedLatest",
    "- MaterialBatch dry-run: $materialBatchDryRunStatus",
    "- MaterialBatch dry-run latest: $materialBatchDryRunLatest",
    "- Continuation automation: $continuationAutomationId",
    "- Continuation cadence: $continuationAutomationCadence",
    "- Continuation rule: $continuationAutomationRule",
    "- Governance gate: $governanceStatus ($($failedGovernanceChecks.Count) failed)",
    "- Master plan: $masterPlanPath",
    "",
    "## Active Objective",
    "",
    "Implement the UE5.8 Epic/High/Mid/Low model, material, VT Atlas, bake, Source/Proxy, and performance tier plan. Keep automation context recoverable if the conversation is interrupted. Compilation is allowed for this goal.",
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
    "## Governance Gate",
    "",
    "| Check | Passed | Evidence |",
    "| --- | --- | --- |"
)

foreach ($check in $governanceChecks) {
    $lines += "| $($check.Check) | $($check.Passed) | $($check.Evidence) |"
}

$lines += @(
    "",
    "## Resume Checklist",
    "",
    "1. Read guide.md before changing files.",
    "2. Check git status --short --branch and do not revert unrelated changes.",
    "3. Treat the file named UE58_EpicHighMidLow_*.md under Docs performance folder as the current master plan.",
    "4. Keep module ownership explicit: Source/DevKit for runtime settings/data, Source/DevKitEditor for commandlets/tools, Config for four-tier CVars/tags, Docs/GeneratedReports for evidence.",
    "5. Do not manually edit binary .uasset files; generated materials, textures, mapping data, and batch assets must come from commandlets or editor tools.",
    "6. Continue from the first unfinished phase in the master plan: audit, tags/assets, model proxy, material bake, VT Atlas, Source/Proxy/Baked validation, performance tier integration, or runtime profiling.",
    "7. For any VT/non-VT or Source/Proxy/Baked change, inspect ResidencyRiskPlan, Source/Proxy/Baked layer readiness, and Source/Proxy asset readiness before treating it as validated.",
    "8. Keep the native EnvBatch Tagger and Python fallback tag contracts aligned; check EnvBatchTagTools/LATEST.md before asking art to use either tool.",
    "9. Check PilotCluster/LATEST.md before changing the real-cluster dry-run target; WP1 is ready only when the pilot report is ReadyForCommandlet, and WP12 is ready only when actor tag evidence is captured by dry-run.",
    "10. The Codex continuation heartbeat should resume this thread every 5 hours through automation id ue58-epic-high-mid-low.",
    "11. MaterialBatch dry-run is not ready until actual StreamingLevel layer evidence, Source/Proxy asset evidence, and residency evidence are all true.",
    "12. If compile or clean-link validation is blocked by UnrealEditor, Invoke-UE58BuildValidation.ps1 -RunBuild may auto-close the editor for this goal; heartbeat itself still does not compile.",
    "13. If MCP evidence is needed and no editor is listening, launch UE5.8 editor for MCP, collect the evidence, and record the editor state in the relevant report.",
    "14. After a clean link succeeds, run Invoke-UE58PostCleanLinkValidation.ps1 -RunReportCommandlets -RunMaterialBatchDryRun to continue validation.",
    "15. Use Invoke-UE58BuildValidation.ps1 -RunBuild for an explicit compile record; heartbeat only refreshes the non-compiling build-status report.",
    "16. Before upload/push, compile first and record the build result.",
    "",
    "## Suggested Resume Prompt",
    "",
    "Continue the UE5.8 Epic/High/Mid/Low model/material performance-tier goal. Read guide.md, this heartbeat report, git status, the MaterialBatch dry-run latest report, the submission gate latest report, and the current master plan file named UE58_EpicHighMidLow_*.md under Docs performance folder. Do not repeat completed edits or revert unrelated work. Keep Source/DevKit, Source/DevKitEditor, Config, Docs, generated asset, ResidencyRiskPlan, Source/Proxy/Baked layer readiness, Source/Proxy asset readiness, and MaterialBatch dry-run ready constraints visible while continuing from the first unfinished phase and reporting verification evidence. Compilation is allowed for this goal; if UnrealEditor blocks compile, auto-close it through the build validation flow, and if MCP evidence is needed while the editor is closed, auto-launch UE5.8 editor for MCP."
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
