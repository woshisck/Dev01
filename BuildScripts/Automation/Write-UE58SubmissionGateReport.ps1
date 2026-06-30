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
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Test-FileText {
    param(
        [string]$Path,
        [string]$Pattern
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return $false
    }

    $text = Get-Content -LiteralPath $Path -Raw
    return $text -match $Pattern
}

function Test-RuntimeCaptureReady {
    param([string]$Path)

    return (
        ((Test-FileText -Path $Path -Pattern "(?m)^- Status: (Captured|Partial)\s*$") -and
            (Test-FileText -Path $Path -Pattern "\|\s*[1-9][0-9]*\s*\|")) -or
        ((Test-FileText -Path $Path -Pattern "(?m)^- Status: ParsedLogCaptured\s*$") -and
            (Test-FileText -Path $Path -Pattern "Frame Time ms") -and
            (Test-FileText -Path $Path -Pattern "\|\s*[0-9]+(?:\.[0-9]+)?\s*\|\s*[0-9]+(?:\.[0-9]+)?\s*\|"))
    )
}

function Get-PreferredRuntimeCapturePath {
    param([string]$LatestPath)

    $paths = @()
    if (Test-Path -LiteralPath $LatestPath) {
        $paths += (Resolve-Path -LiteralPath $LatestPath).Path
    }

    $root = Split-Path -Parent $LatestPath
    if (Test-Path -LiteralPath $root) {
        $paths += @(
            Get-ChildItem -LiteralPath $root -File -Filter "UE58RuntimeProfilingCapture_*.md" -ErrorAction SilentlyContinue |
                Sort-Object LastWriteTime -Descending |
                ForEach-Object { $_.FullName }
        )
    }

    foreach ($path in ($paths | Select-Object -Unique)) {
        if (Test-RuntimeCaptureReady -Path $path) {
            return $path
        }
    }

    return $LatestPath
}

function Get-UbtBuildStatus {
    $ubtLogPath = Join-Path $env:LOCALAPPDATA "UnrealBuildTool\Log.txt"
    if (-not (Test-Path -LiteralPath $ubtLogPath)) {
        return "Missing"
    }

    $tail = Get-Content -LiteralPath $ubtLogPath -Tail 100
    $resultLine = $tail | Where-Object { $_ -match "^Result:" } | Select-Object -Last 1
    if ($resultLine) {
        return $resultLine.Trim()
    }

    return "Unknown"
}

function Get-ScopeCategory {
    param([string]$Path)

    if ($Path -like "Source/DevKit/*" -or $Path -like "Source\DevKit\*") { return "Runtime code" }
    if ($Path -like "Source/DevKitEditor/*" -or $Path -like "Source\DevKitEditor\*") { return "Editor automation code" }
    if ($Path -like "BuildScripts/Automation/*" -or $Path -like "BuildScripts\Automation\*") { return "Codex automation scripts" }
    if ($Path -like "Config/*" -or $Path -like "Config\*") { return "Project config" }
    if ($Path -like "Content/UI/*" -or $Path -like "Content\UI\*") { return "UI assets" }
    if ($Path -like "Content/Generated/*" -or $Path -like "Content\Generated\*") { return "Generated material-batch assets" }
    if ($Path -like "Content/Art/Map/*" -or $Path -like "Content\Art\Map\*") { return "Map assets" }
    if ($Path -like "Docs/GeneratedReports/*" -or $Path -like "Docs\GeneratedReports\*") { return "Generated reports" }
    if ($Path -like "Docs/*" -or $Path -like "Docs\*") { return "Design and production docs" }
    if ($Path -eq ".gitignore" -or $Path -eq "AGENTS.md" -or $Path -eq "guide.md" -or $Path -eq "Generate_And_Build.bat") { return "Repo/build rules" }

    return "Other"
}

function Convert-GitStatusLineToEntry {
    param([string]$Line)

    if ([string]::IsNullOrWhiteSpace($Line) -or $Line -match "^## ") {
        return $null
    }

    $status = $Line.Substring(0, 2).Trim()
    if ([string]::IsNullOrWhiteSpace($status)) {
        $status = "M"
    }
    $path = $Line.Substring(3).Trim()
    if ($path -match " -> ") {
        $path = ($path -split " -> ")[-1]
    }

    return [pscustomobject]@{
        Status = $status
        Path = $path
        Category = Get-ScopeCategory -Path $path
    }
}

function Get-StagingRecommendation {
    param([string]$Path, [string]$Category)

    if ($Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/FinalUpload/*") {
        return "GeneratedHistoryIgnored"
    }

    if ($Path -like "Docs/GeneratedReports/UE58PerformanceAudit/LATEST.md" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/LATEST.md" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/LATEST.md" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UE58_Progress_Questions_Response.html" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/ApprovalPacket/UserResponses_*.md" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/SubmissionGate/*.paths.txt" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/*/LATEST.md" -or
        $Path -like "Docs/GeneratedReports/CommandletReports/*" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/BatchVisualAudit/*.png" -or
        $Path -like "Docs/GeneratedReports/UE58PerformanceAutomation/SceneParityAudit/*.png") {
        return "EvidenceOnly"
    }

    if ($Category -eq "Generated reports") {
        return "GeneratedHistoryIgnored"
    }

    if ($Category -eq "Map assets" -or
        $Path -like "Content/UI/BP_YogHUD.uasset" -or
        $Path -like "Content/UI/Playtest_UI/*") {
        return "NeedsManualReview"
    }

    if ($Category -in @(
        "Codex automation scripts",
        "Design and production docs",
        "Editor automation code",
        "Generated material-batch assets",
        "Project config",
        "Repo/build rules",
        "Runtime code"
    )) {
        return "Phase1Candidate"
    }

    if ($Path -like "Content/UI/Frontend/*") {
        return "Phase1Candidate"
    }

    return "NeedsManualReview"
}

function Test-RemoteAddedLinesPresent {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $remoteDiff = (git -c core.autocrlf=false -C $RepoRoot diff --unified=0 main..origin/main -- $Path) 2>$null
    $addedLines = @()
    foreach ($line in $remoteDiff) {
        if ($line.StartsWith("+++") -or -not $line.StartsWith("+")) {
            continue
        }
        $addedLines += $line.Substring(1)
    }

    if ($addedLines.Count -eq 0) {
        return $true
    }

    $worktreePath = Join-Path $RepoRoot $Path
    if (-not (Test-Path -LiteralPath $worktreePath)) {
        return $false
    }

    $worktreeText = Get-Content -LiteralPath $worktreePath -Raw
    foreach ($addedLine in $addedLines) {
        if (-not $worktreeText.Contains($addedLine)) {
            return $false
        }
    }

    return $true
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58SubmissionGate_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$auditLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAudit\LATEST.md"
$heartbeatLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\LATEST.md"
$batchVisualLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\LATEST.md"
$sceneParityLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SceneParityAudit\LATEST.md"
$runtimeSmokeLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingSmoke\LATEST.md"
$runtimeCaptureLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture\LATEST.md"
$runtimeCaptureEvidencePath = Get-PreferredRuntimeCapturePath -LatestPath $runtimeCaptureLatestPath
$materialBatchDryRunLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\MaterialBatchDryRun\LATEST.md"
$commandletAvailabilityLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\CommandletAvailability\LATEST.md"
$remoteDeltaAuditLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RemoteDeltaAudit\LATEST.md"
$artifactContractLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\ArtifactContract\LATEST.md"
$performanceSettingsLog = Join-Path $RepoRoot "Saved\Logs\Codex_PerformanceSettings_Focus_Tests.log"
$uiTestsLog = Join-Path $RepoRoot "Saved\Logs\Codex_UI_Focus_Tests.log"
$materialBatchTestsLog = Join-Path $RepoRoot "Saved\Logs\Codex_MaterialBatchTests.log"
$ue58PerformanceTestsLog = Join-Path $RepoRoot "Saved\Logs\Codex_UE58Performance_Tests.log"

Push-Location $RepoRoot
try {
    $gitStatus = (git status --short --branch) 2>$null
    $gitStatusPorcelain = (git status --porcelain -uall) 2>$null
    $remoteDelta = (git log --oneline --left-right --cherry-pick main...origin/main) 2>$null
    $remoteDeltaFiles = (git diff --name-only main..origin/main) 2>$null
}
finally {
    Pop-Location
}

$gitBehind = ($gitStatus -join "`n") -match "behind"
$workingTreeDirty = (($gitStatus | Where-Object { $_ -notmatch "^## " }) | Measure-Object).Count -gt 0
$worktreeEntries = @()
foreach ($line in $gitStatusPorcelain) {
    $entry = Convert-GitStatusLineToEntry -Line $line
    if ($entry) {
        $entry | Add-Member -MemberType NoteProperty -Name StagingRecommendation -Value (Get-StagingRecommendation -Path $entry.Path -Category $entry.Category)
        $worktreeEntries += $entry
    }
}
$scopeGroups = $worktreeEntries | Group-Object Category | Sort-Object Name
$stagingGroups = $worktreeEntries | Group-Object StagingRecommendation | Sort-Object Name
$remoteOverlap = @()
foreach ($entry in $worktreeEntries) {
    $normalizedPath = $entry.Path.Replace("\", "/")
    foreach ($remoteFile in $remoteDeltaFiles) {
        if ($normalizedPath -eq $remoteFile.Replace("\", "/")) {
            $diffToOrigin = (git -c core.autocrlf=false -C $RepoRoot diff --name-only --ignore-space-at-eol origin/main -- $entry.Path) 2>$null
            $remoteOverlap += [pscustomobject]@{
                Path = $entry.Path
                Status = $entry.Status
                Category = $entry.Category
                IdenticalToOrigin = ($diffToOrigin.Count -eq 0)
                RemoteAddedLinesPresent = Test-RemoteAddedLinesPresent -RepoRoot $RepoRoot -Path $entry.Path
            }
        }
    }
}

$staticAuditReady = (Test-FileText -Path $auditLatestPath -Pattern "UE58RuntimeProfiling MCP smoke ready \| True") -and
    (Test-FileText -Path $auditLatestPath -Pattern "UE58 Batch Visual MCP captures \| True") -and
    (Test-FileText -Path $auditLatestPath -Pattern "UE58 Scene Parity MCP ready \| True") -and
    (Test-FileText -Path $auditLatestPath -Pattern "Graphics settings focus contract \| True")
$batchVisualReady = Test-FileText -Path $batchVisualLatestPath -Pattern "Status: Captured"
$sceneParityReady = (Test-FileText -Path $sceneParityLatestPath -Pattern "(?m)^- Status: Captured\s*$") -and
    (Test-FileText -Path $sceneParityLatestPath -Pattern "(?m)^- Cleanup status: RemovedScratchActors\s*$") -and
    (Test-FileText -Path $sceneParityLatestPath -Pattern "(?m)^- Viewport bytes: [1-9][0-9]{3,}\s*$")
$runtimeSmokeReady = Test-FileText -Path $runtimeSmokeLatestPath -Pattern "Status: Ready"
$artifactContractReady = Test-FileText -Path $artifactContractLatestPath -Pattern "(?m)^- Status: Passed\s*$"
$commandletAvailabilityReady = Test-FileText -Path $commandletAvailabilityLatestPath -Pattern "(?m)^- Status: Available\s*$"
$commandletAvailabilityBlockedByCleanLink = Test-FileText -Path $commandletAvailabilityLatestPath -Pattern "(?m)^- Status: BlockedByCleanLink\s*$"
$remoteDeltaAuditReady = Test-FileText -Path $remoteDeltaAuditLatestPath -Pattern "(?m)^- Status: (ReadyForReviewedMerge|NoRemoteDelta)\s*$"
$materialBatchDryRunCaptured = Test-FileText -Path $materialBatchDryRunLatestPath -Pattern "(?m)^- Status: DryRunCaptured\s*$"
$materialBatchDryRunLayerEvidence = Test-FileText -Path $materialBatchDryRunLatestPath -Pattern "(?m)^- Actual layer evidence: True\s*$"
$materialBatchDryRunSourceProxyAssetEvidence = Test-FileText -Path $materialBatchDryRunLatestPath -Pattern "(?m)^- Source/Proxy asset evidence: True\s*$"
$materialBatchDryRunResidencyEvidence = Test-FileText -Path $materialBatchDryRunLatestPath -Pattern "(?m)^- Residency risk evidence: True\s*$"
$materialBatchDryRunReady = $materialBatchDryRunCaptured -and
    $materialBatchDryRunLayerEvidence -and
    $materialBatchDryRunSourceProxyAssetEvidence -and
    $materialBatchDryRunResidencyEvidence
$runtimeCaptureReady = Test-RuntimeCaptureReady -Path $runtimeCaptureEvidencePath
$mcpOnline = Test-FileText -Path $heartbeatLatestPath -Pattern "MCP: Listening on 127\.0\.0\.1:8765"
$latestBuildStatus = Get-UbtBuildStatus
$latestBuildSucceeded = $latestBuildStatus -match "Result: Succeeded"

$tests = @(
    [pscustomobject]@{ Name = "DevKit.Performance.Settings"; Path = $performanceSettingsLog; Passed = Test-FileText -Path $performanceSettingsLog -Pattern "TEST COMPLETE\. EXIT CODE: 0" },
    [pscustomobject]@{ Name = "DevKitEditor.UI"; Path = $uiTestsLog; Passed = Test-FileText -Path $uiTestsLog -Pattern "TEST COMPLETE\. EXIT CODE: 0" },
    [pscustomobject]@{ Name = "DevKitEditor.MaterialBatch"; Path = $materialBatchTestsLog; Passed = Test-FileText -Path $materialBatchTestsLog -Pattern "TEST COMPLETE\. EXIT CODE: 0" },
    [pscustomobject]@{ Name = "DevKitEditor.UE58Performance"; Path = $ue58PerformanceTestsLog; Passed = Test-FileText -Path $ue58PerformanceTestsLog -Pattern "TEST COMPLETE\. EXIT CODE: 0" }
)

$allTestsPassed = (($tests | Where-Object { -not $_.Passed }) | Measure-Object).Count -eq 0

$hardBlocks = @()
if ($gitBehind) {
    $hardBlocks += "Local main is behind origin/main; merge/rebase the remote commit before final upload."
}
if ($workingTreeDirty) {
    $hardBlocks += "Working tree is dirty; review and intentionally stage the UE5.8 scope before commit."
}
if (-not $allTestsPassed) {
    $hardBlocks += "One or more required automation logs are missing or not passing."
}
if (-not $latestBuildSucceeded) {
    $hardBlocks += "Latest Unreal build result is not recorded as succeeded."
}
if (-not $commandletAvailabilityReady) {
    $hardBlocks += "UE5.8 report-only commandlets are not all available in the current editor binaries."
}

$evidenceGaps = @()
if (-not $staticAuditReady) {
    $evidenceGaps += "Static audit does not yet show all UE5.8 automation evidence as ready."
}
if (-not $batchVisualReady) {
    $evidenceGaps += "Batch visual MCP audit is missing or incomplete."
}
if (-not $sceneParityReady) {
    $evidenceGaps += "Scene-level source/proxy visual parity is not captured yet; run Invoke-UE58SceneParityMcpAudit.ps1 to generate the side-by-side target-level viewport PNG."
}
if (-not $runtimeSmokeReady) {
    $evidenceGaps += "Runtime profiling MCP smoke is missing or incomplete."
}
if (-not $artifactContractReady) {
    $evidenceGaps += "UE5.8 performance artifact contract is missing or failing; run Test-UE58PerformanceArtifactContract.ps1."
}
if (-not $commandletAvailabilityReady) {
    if ($commandletAvailabilityBlockedByCleanLink) {
        $evidenceGaps += "Report-only commandlets are blocked by a clean-link requirement; close UnrealEditor after saving work, link DevKitEditor, then rerun GraphicsSettingsWidgetSetup, MaterialBatchMaterialAudit, and UE58RuntimeProfilingPlan reports."
    }
    else {
        $evidenceGaps += "Commandlet availability report is missing or not Available; run Test-UE58CommandletAvailability.ps1 before real dry-run or final upload."
    }
}
if (-not $materialBatchDryRunReady) {
    $evidenceGaps += "MaterialBatch real-cluster dry-run is not captured with actual StreamingLevel layer, Source/Proxy asset readiness, and residency evidence."
}
if (-not $runtimeCaptureReady) {
    if (Test-FileText -Path $runtimeCaptureEvidencePath -Pattern "(?m)^- Status: LogCaptured\s*$") {
        $evidenceGaps += "Runtime profiling automation ran baseline and Lumen Lite scenarios and invoked stat/profilegpu commands, but no ProfileGPU artifact was emitted; add a follow-up capture path for parseable GPU metrics before final handheld decisions."
    }
    else {
        $evidenceGaps += "Representative runtime GPU profiling capture has not been run yet; use Invoke-UE58RuntimeProfilingCapture.ps1 -Run to collect baseline and Lumen Lite logs/artifacts."
    }
}
if (-not $mcpOnline) {
    $evidenceGaps += "Latest heartbeat does not show the UE5.8 MCP server listening on 127.0.0.1:8765."
}
if ($gitBehind -and -not $remoteDeltaAuditReady) {
    $evidenceGaps += "Remote delta audit is missing or not ready; run Invoke-UE58RemoteDeltaAudit.ps1 before choosing a merge/rebase strategy."
}

$canCommitPhase1 = (-not $gitBehind) -and $allTestsPassed -and $latestBuildSucceeded -and $staticAuditReady -and $batchVisualReady -and $sceneParityReady -and $runtimeSmokeReady -and $artifactContractReady -and $commandletAvailabilityReady -and $materialBatchDryRunReady
$canUploadFinal = $canCommitPhase1 -and ($evidenceGaps.Count -eq 0)

$phase1PathspecPath = Join-Path $OutputRoot "Phase1Candidate.paths.txt"
$evidencePathspecPath = Join-Path $OutputRoot "EvidenceOnly.paths.txt"
$manualReviewPathspecPath = Join-Path $OutputRoot "NeedsManualReview.paths.txt"

$phase1Paths = $worktreeEntries |
    Where-Object { $_.StagingRecommendation -eq "Phase1Candidate" } |
    Sort-Object Path |
    ForEach-Object { $_.Path }
$evidencePaths = $worktreeEntries |
    Where-Object { $_.StagingRecommendation -eq "EvidenceOnly" } |
    Sort-Object Path |
    ForEach-Object { $_.Path }
$manualReviewPaths = $worktreeEntries |
    Where-Object { $_.StagingRecommendation -eq "NeedsManualReview" } |
    Sort-Object Path |
    ForEach-Object { $_.Path }

Set-Content -LiteralPath $phase1PathspecPath -Value $phase1Paths -Encoding UTF8
Set-Content -LiteralPath $evidencePathspecPath -Value $evidencePaths -Encoding UTF8
Set-Content -LiteralPath $manualReviewPathspecPath -Value $manualReviewPaths -Encoding UTF8

$lines = @(
    "# UE5.8 Submission Gate",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Can commit Phase 1 scope: $canCommitPhase1",
    "- Can upload final main: $canUploadFinal",
    "- Latest UE build: $latestBuildStatus",
    "- Working tree dirty: $workingTreeDirty",
    "- Local branch behind origin/main: $gitBehind",
    "- MCP online in latest heartbeat: $mcpOnline",
    "- Static audit ready: $staticAuditReady",
    "- Batch visual MCP ready: $batchVisualReady",
    "- Scene parity MCP ready: $sceneParityReady",
    "- Runtime profiling MCP smoke ready: $runtimeSmokeReady",
    "- Artifact contract ready: $artifactContractReady",
    "- Commandlet availability ready: $commandletAvailabilityReady",
    "- Commandlet availability blocked by clean link: $commandletAvailabilityBlockedByCleanLink",
    "- MaterialBatch dry-run captured: $materialBatchDryRunCaptured",
    "- MaterialBatch dry-run layer evidence: $materialBatchDryRunLayerEvidence",
    "- MaterialBatch dry-run Source/Proxy asset evidence: $materialBatchDryRunSourceProxyAssetEvidence",
    "- MaterialBatch dry-run residency evidence: $materialBatchDryRunResidencyEvidence",
    "- MaterialBatch dry-run ready: $materialBatchDryRunReady",
    "- Runtime profiling capture ready: $runtimeCaptureReady",
    "- Runtime profiling capture evidence: $runtimeCaptureEvidencePath",
    "- Remote delta audit ready: $remoteDeltaAuditReady",
    "- Phase 1 pathspec: Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\Phase1Candidate.paths.txt",
    "- Evidence pathspec: Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\EvidenceOnly.paths.txt",
    "- Manual-review pathspec: Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\NeedsManualReview.paths.txt",
    "",
    "## Test Logs",
    "",
    "| Suite | Passed | Log |",
    "| --- | --- | --- |"
)

foreach ($test in $tests) {
    $relativePath = $test.Path
    if ($relativePath.StartsWith($RepoRoot)) {
        $relativePath = $relativePath.Substring($RepoRoot.Length).TrimStart("\", "/")
    }
    $lines += "| $($test.Name) | $($test.Passed) | ``$relativePath`` |"
}

$lines += @(
    "",
    "## Remote Delta",
    "",
    '```text'
)
if ($remoteDelta) {
    $lines += $remoteDelta
}
else {
    $lines += "(no remote delta)"
}
$lines += '```'

$lines += @(
    "",
    "## Git Status",
    "",
    '```text'
)
if ($gitStatus) {
    $lines += $gitStatus
}
else {
    $lines += "(git status unavailable or empty)"
}
$lines += '```'

$lines += @(
    "",
    "## Worktree Scope Summary",
    "",
    "| Category | Count |",
    "| --- | ---: |"
)
foreach ($group in $scopeGroups) {
    $lines += "| $($group.Name) | $($group.Count) |"
}

$lines += @(
    "",
    "## Staging Recommendation Summary",
    "",
    "| Recommendation | Count | Meaning |",
    "| --- | ---: | --- |"
)
foreach ($group in $stagingGroups) {
    $meaning = switch ($group.Name) {
        "Phase1Candidate" { "Likely belongs to the UE5.8 Phase 1 implementation scope." }
        "EvidenceOnly" { "Evidence/report artifact; keep only if the review wants generated proof in the commit." }
        "GeneratedHistoryIgnored" { "Historical generated report/log artifact; not staged by the reviewed upload path." }
        "NeedsManualReview" { "Binary or broad asset change; review explicitly before staging." }
        default { "Unclassified; inspect before staging." }
    }
    $lines += "| $($group.Name) | $($group.Count) | $meaning |"
}

$lines += @(
    "",
    "## Worktree Scope Files",
    "",
    "| Recommendation | Category | Status | Path |",
    "| --- | --- | --- | --- |"
)
foreach ($entry in ($worktreeEntries | Sort-Object Category, Path)) {
    $lines += "| $($entry.StagingRecommendation) | $($entry.Category) | $($entry.Status) | ``$($entry.Path)`` |"
}

$lines += @(
    "",
    "## Remote Overlap Files",
    ""
)
if ($remoteOverlap.Count -gt 0) {
    $lines += @(
        "| Path | IdenticalToOrigin | RemoteAddedLinesPresent |",
        "| --- | --- | --- |"
    )
    foreach ($entry in ($remoteOverlap | Sort-Object Path -Unique)) {
        $lines += "| ``$($entry.Path)`` | $($entry.IdenticalToOrigin) | $($entry.RemoteAddedLinesPresent) |"
    }
}
else {
    $lines += "- None."
}

$lines += @(
    "",
    "## Hard Blocks",
    ""
)
if ($hardBlocks.Count -gt 0) {
    foreach ($block in $hardBlocks) {
        $lines += "- $block"
    }
}
else {
    $lines += "- None detected for a Phase 1 commit gate."
}

$lines += @(
    "",
    "## Evidence Gaps",
    ""
)
foreach ($gap in $evidenceGaps) {
    $lines += "- $gap"
}

$lines += @(
    "",
    "## Required Final Sequence",
    "",
    "1. Resolve remote/main delta without reverting local UE5.8 work.",
    "2. Run the required automation tests again after merge/rebase.",
    "3. Compile immediately before upload.",
    "4. Commit only the reviewed UE5.8 scope.",
    "5. Push main only after the user approves the final scope."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote submission gate report: $reportPath"
Write-Output "Updated latest submission gate: $latestPath"
