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
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\ApprovalPacket"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Invoke-GitLines {
    param([string[]]$Arguments)
    $output = (& git -C $RepoRoot @Arguments) 2>&1
    return @($output)
}

function Get-PathList {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return @()
    }
    return @(Get-Content -LiteralPath $Path | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

function Get-ReportValue {
    param(
        [string]$Path,
        [string]$Label
    )
    if (-not (Test-Path -LiteralPath $Path)) {
        return "Missing"
    }
    $match = Select-String -LiteralPath $Path -Pattern "^- $([regex]::Escape($Label)): (.+)$" | Select-Object -First 1
    if ($match) {
        return $match.Matches[0].Groups[1].Value.Trim()
    }
    return "Unknown"
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58ApprovalPacket_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$submissionGatePath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\LATEST.md"
$remoteAuditPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RemoteDeltaAudit\LATEST.md"
$requiredTestsPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RequiredTests\LATEST.md"
$phase1Path = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\Phase1Candidate.paths.txt"
$evidencePath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\EvidenceOnly.paths.txt"
$manualPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\NeedsManualReview.paths.txt"

$gitStatus = Invoke-GitLines -Arguments @("status", "--short", "--branch")
$remoteCommits = Invoke-GitLines -Arguments @("log", "--oneline", "--left-right", "--cherry-pick", "main...origin/main")
$remoteNameStatus = Invoke-GitLines -Arguments @("diff", "--name-status", "main..origin/main")
$remoteDeletedFiles = @()
foreach ($line in $remoteNameStatus) {
    if ($line -match "^D\s+(.+)$") {
        $remoteDeletedFiles += $Matches[1]
    }
}

$phase1Paths = Get-PathList -Path $phase1Path
$evidencePaths = Get-PathList -Path $evidencePath
$manualPaths = Get-PathList -Path $manualPath

$remoteAuditStatus = Get-ReportValue -Path $remoteAuditPath -Label "Status"
$remoteAuditConflict = Get-ReportValue -Path $remoteAuditPath -Label "Merge-tree conflict detected"
$remoteAuditLinesPresent = Get-ReportValue -Path $remoteAuditPath -Label "Remote added lines present in worktree"
$testsStatus = Get-ReportValue -Path $requiredTestsPath -Label "Status"
$canCommit = Get-ReportValue -Path $submissionGatePath -Label "Can commit Phase 1 scope"
$canUpload = Get-ReportValue -Path $submissionGatePath -Label "Can upload final main"

$recommendedSyncPolicy = "Accept remote deletion of generated Visual Studio solution files if the team agrees `.slnx` files are local generated IDE artifacts. Keep the local UE5.8 fallback in Generate_And_Build.bat."
if ($remoteDeletedFiles.Count -eq 0) {
    $recommendedSyncPolicy = "No remote deletions detected. Resolve remote/main delta by preserving current local UE5.8 work and fast-forwarding/merging reviewed text deltas."
}

$lines = @(
    "# UE5.8 Approval Packet",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Purpose: collect the current user approval points before merge, staging, commit, compile, and upload.",
    "- This script performs no merge, staging, commit, push, checkout, deletion, or compile.",
    "",
    "## Gate Summary",
    "",
    "| Item | Value |",
    "| --- | --- |",
    "| Can commit Phase 1 scope | $canCommit |",
    "| Can upload final main | $canUpload |",
    "| Remote delta audit status | $remoteAuditStatus |",
    "| Merge-tree conflict detected | $remoteAuditConflict |",
    "| Remote added lines present in worktree | $remoteAuditLinesPresent |",
    "| Required tests dry-run status | $testsStatus |",
    "| Phase1Candidate count | $($phase1Paths.Count) |",
    "| EvidenceOnly count | $($evidencePaths.Count) |",
    "| NeedsManualReview count | $($manualPaths.Count) |",
    "",
    "## Required User Decisions",
    "",
    "1. Remote sync policy: $recommendedSyncPolicy",
    "2. Commit scope policy: approve whether to stage Phase1Candidate only, Phase1Candidate plus EvidenceOnly reports, and whether the NeedsManualReview binary assets should be included.",
    "3. Final upload policy: after sync and staging approval, rerun required tests, compile immediately before upload, commit the approved scope, and push `main` only after explicit approval.",
    "",
    "## Remote Commits Pending",
    "",
    '```text'
)

if ($remoteCommits.Count -gt 0) {
    $lines += $remoteCommits
}
else {
    $lines += "(no remote commits pending)"
}

$lines += @(
    '```',
    "",
    "## Remote File Delta",
    "",
    '```text'
)
if ($remoteNameStatus.Count -gt 0) {
    $lines += $remoteNameStatus
}
else {
    $lines += "(no remote file delta)"
}
$lines += '```'

$lines += @(
    "",
    "## Remote Deleted Files",
    ""
)
if ($remoteDeletedFiles.Count -gt 0) {
    foreach ($path in $remoteDeletedFiles) {
        $lines += "- ``$path``"
    }
}
else {
    $lines += "- None."
}

$lines += @(
    "",
    "## NeedsManualReview Files",
    ""
)
if ($manualPaths.Count -gt 0) {
    foreach ($path in $manualPaths) {
        $lines += "- ``$path``"
    }
}
else {
    $lines += "- None."
}

$lines += @(
    "",
    "## Phase1Candidate Pathspec",
    "",
    "- Pathspec file: ``Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\Phase1Candidate.paths.txt``",
    "- Count: $($phase1Paths.Count)",
    "",
    "## EvidenceOnly Pathspec",
    "",
    "- Pathspec file: ``Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\EvidenceOnly.paths.txt``",
    "- Count: $($evidencePaths.Count)",
    "",
    "## Current Git Status",
    "",
    '```text'
)
if ($gitStatus.Count -gt 0) {
    $lines += $gitStatus
}
else {
    $lines += "(git status unavailable)"
}
$lines += '```'

$lines += @(
    "",
    "## Approval Text The User Can Give",
    "",
    "Use one of these as an explicit approval signal:",
    "",
    "- APPROVE_REMOTE_DELETE_SLNX_AND_STAGE_PHASE1_ONLY",
    "- APPROVE_REMOTE_DELETE_SLNX_AND_STAGE_PHASE1_PLUS_MANUAL_REVIEW_NO_EVIDENCE",
    "- APPROVE_REMOTE_DELETE_SLNX_AND_STAGE_PHASE1_PLUS_EVIDENCE",
    "- APPROVE_REMOTE_DELETE_SLNX_AND_STAGE_PHASE1_PLUS_EVIDENCE_PLUS_MANUAL_REVIEW",
    "",
    "If the user wants to keep the .slnx files, say so explicitly before any merge/rebase."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote approval packet: $reportPath"
Write-Output "Updated latest approval packet: $latestPath"
