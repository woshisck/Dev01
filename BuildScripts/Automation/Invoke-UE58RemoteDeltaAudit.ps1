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
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RemoteDeltaAudit"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Invoke-GitLines {
    param([string[]]$Arguments)

    $output = (& git -C $RepoRoot @Arguments) 2>&1
    return @($output)
}

function Test-RemoteAddedLinesPresent {
    param([string]$Path)

    $remoteDiff = Invoke-GitLines -Arguments @("-c", "core.autocrlf=false", "diff", "--unified=0", "main..origin/main", "--", $Path)
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
$reportPath = Join-Path $OutputRoot "UE58RemoteDeltaAudit_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$gitStatus = Invoke-GitLines -Arguments @("status", "--short", "--branch")
$remoteCommits = Invoke-GitLines -Arguments @("log", "--oneline", "--left-right", "--cherry-pick", "main...origin/main")
$remoteFiles = Invoke-GitLines -Arguments @("diff", "--name-only", "main..origin/main") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
$remoteNameStatus = Invoke-GitLines -Arguments @("diff", "--name-status", "main..origin/main") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
$remoteStatusByPath = @{}
foreach ($line in $remoteNameStatus) {
    $parts = $line -split "\s+", 2
    if ($parts.Count -eq 2) {
        $remoteStatusByPath[$parts[1]] = $parts[0]
    }
}
$behind = ($gitStatus -join "`n") -match "behind"

$mergeBase = ""
$mergeTreeStatus = "NotRun"
$mergeTreeOutput = @()
$mergeConflictDetected = $false
try {
    $mergeBaseLines = Invoke-GitLines -Arguments @("merge-base", "main", "origin/main")
    $mergeBase = ($mergeBaseLines | Select-Object -First 1)
    if (-not [string]::IsNullOrWhiteSpace($mergeBase)) {
        $mergeTreeOutput = Invoke-GitLines -Arguments @("merge-tree", $mergeBase, "main", "origin/main")
        $mergeTreeStatus = "Ran"
        $mergeTreeText = $mergeTreeOutput -join "`n"
        $mergeConflictDetected = $mergeTreeText -match "(?m)^(<<<<<<<|=======|>>>>>>>|CONFLICT\b)" -or
            $mergeTreeText -match "(?i)\bchanged in both\b|\badded in both\b|\bremoved in both\b"
    }
}
catch {
    $mergeTreeStatus = "Unavailable: $($_.Exception.Message)"
}

$fileRows = @()
foreach ($path in $remoteFiles) {
    $diffToOrigin = Invoke-GitLines -Arguments @("-c", "core.autocrlf=false", "diff", "--name-only", "--ignore-space-at-eol", "origin/main", "--", $path)
    $worktreePath = Join-Path $RepoRoot $path
    $remoteStatus = "?"
    if ($remoteStatusByPath.ContainsKey($path)) {
        $remoteStatus = $remoteStatusByPath[$path]
    }
    $fileRows += [pscustomobject]@{
        Path = $path
        RemoteStatus = $remoteStatus
        ExistsInWorktree = Test-Path -LiteralPath $worktreePath
        IdenticalToOrigin = (($diffToOrigin | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count -eq 0)
        RemoteAddedLinesPresent = Test-RemoteAddedLinesPresent -Path $path
    }
}

$allRemoteAddedLinesPresent = $true
foreach ($row in $fileRows) {
    if (-not $row.RemoteAddedLinesPresent) {
        $allRemoteAddedLinesPresent = $false
    }
}

if ($remoteFiles.Count -eq 0) {
    $status = "NoRemoteDelta"
}
elseif ($allRemoteAddedLinesPresent -and -not $mergeConflictDetected -and $mergeTreeStatus -ne "NotRun" -and -not $mergeTreeStatus.StartsWith("Unavailable")) {
    $status = "ReadyForReviewedMerge"
}
else {
    $status = "NeedsManualResolution"
}

$lines = @(
    "# UE5.8 Remote Delta Audit",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Local branch behind origin/main: $behind",
    "- Merge base: $mergeBase",
    "- Merge-tree status: $mergeTreeStatus",
    "- Merge-tree conflict detected: $mergeConflictDetected",
    "- Remote added lines present in worktree: $allRemoteAddedLinesPresent",
    "",
    "## Remote Commits",
    "",
    '```text'
)
if ($remoteCommits.Count -gt 0) {
    $lines += $remoteCommits
}
else {
    $lines += "(no remote delta)"
}
$lines += '```'

$lines += @(
    "",
    "## Remote Files",
    "",
    "| Path | RemoteStatus | ExistsInWorktree | IdenticalToOrigin | RemoteAddedLinesPresent |",
    "| --- | --- | --- | --- | --- |"
)
if ($fileRows.Count -gt 0) {
    foreach ($row in ($fileRows | Sort-Object Path)) {
        $lines += "| ``$($row.Path)`` | $($row.RemoteStatus) | $($row.ExistsInWorktree) | $($row.IdenticalToOrigin) | $($row.RemoteAddedLinesPresent) |"
    }
}
else {
    $lines += "| (none) |  |  |  |  |"
}

$lines += @(
    "",
    "## Merge-Tree Preview",
    "",
    '```text'
)
if ($mergeTreeOutput.Count -gt 0) {
    $lines += $mergeTreeOutput
}
else {
    $lines += "(merge-tree preview unavailable or empty)"
}
$lines += '```'

$lines += @(
    "",
    "## Interpretation",
    "",
    "- This script performs no merge, staging, commit, push, or file checkout.",
    "- `ReadyForReviewedMerge` means the remote delta appears text-mergeable and the remote-added lines are already present in the current worktree where applicable.",
    "- `RemoteStatus=D` means the remote branch deleted the file; the current worktree will still show the file until a reviewed merge/rebase or explicit deletion applies that choice.",
    "- Final upload still requires an explicit reviewed merge/rebase step, fresh required tests, compile immediately before upload, user-approved commit scope, and push approval."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote remote delta audit: $reportPath"
Write-Output "Updated latest remote delta audit: $latestPath"
Write-Output "Status: $status"
