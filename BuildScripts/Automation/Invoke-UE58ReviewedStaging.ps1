param(
    [string]$RepoRoot = "",
    [string]$SubmissionGateRoot = "",
    [switch]$IncludeEvidence,
    [switch]$IncludeManualReview,
    [switch]$Apply,
    [switch]$AllowBehind
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($SubmissionGateRoot)) {
    $SubmissionGateRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate"
}

$SubmissionGateRoot = (Resolve-Path -LiteralPath $SubmissionGateRoot).Path

$phase1PathspecPath = Join-Path $SubmissionGateRoot "Phase1Candidate.paths.txt"
$evidencePathspecPath = Join-Path $SubmissionGateRoot "EvidenceOnly.paths.txt"
$manualReviewPathspecPath = Join-Path $SubmissionGateRoot "NeedsManualReview.paths.txt"
$outputRoot = Join-Path $SubmissionGateRoot "StagingReview"

New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

function Read-PathspecList {
    param(
        [string]$Path,
        [string]$Category
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing $Category pathspec file: $Path"
    }

    $items = @()
    foreach ($line in (Get-Content -LiteralPath $Path)) {
        $trimmed = $line.Trim()
        if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
            $items += [pscustomobject]@{
                Category = $Category
                Path = $trimmed.Replace("\", "/")
            }
        }
    }

    return $items
}

function Convert-GitStatusLineToPath {
    param([string]$Line)

    if ([string]::IsNullOrWhiteSpace($Line) -or $Line -match "^## ") {
        return $null
    }

    $path = $Line.Substring(3).Trim()
    if ($path -match " -> ") {
        $path = ($path -split " -> ")[-1]
    }

    return $path.Replace("\", "/")
}

$selectedEntries = @()
$selectedEntries += Read-PathspecList -Path $phase1PathspecPath -Category "Phase1Candidate"

if ($IncludeEvidence) {
    $selectedEntries += Read-PathspecList -Path $evidencePathspecPath -Category "EvidenceOnly"
}

if ($IncludeManualReview) {
    $selectedEntries += Read-PathspecList -Path $manualReviewPathspecPath -Category "NeedsManualReview"
}

$selectedEntries = $selectedEntries | Sort-Object Category, Path -Unique
$selectedPaths = @($selectedEntries | ForEach-Object { $_.Path })

Push-Location $RepoRoot
try {
    $gitStatus = (git status --short --branch) 2>$null
    $gitStatusPorcelain = (git status --porcelain -uall) 2>$null
}
finally {
    Pop-Location
}

$gitBehind = ($gitStatus -join "`n") -match "behind"
if ($Apply -and $gitBehind -and -not $AllowBehind) {
    throw "Refusing to stage because local branch is behind origin/main. Re-run with -AllowBehind only after intentionally accepting that risk."
}

$changedPathSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($line in $gitStatusPorcelain) {
    $statusPath = Convert-GitStatusLineToPath -Line $line
    if ($statusPath) {
        [void]$changedPathSet.Add($statusPath)
    }
}

$missingFromWorktree = @()
$notCurrentlyChanged = @()
foreach ($path in $selectedPaths) {
    $absolutePath = Join-Path $RepoRoot $path
    if (-not (Test-Path -LiteralPath $absolutePath) -and -not $changedPathSet.Contains($path)) {
        $missingFromWorktree += $path
    }
    if (-not $changedPathSet.Contains($path)) {
        $notCurrentlyChanged += $path
    }
}

if ($Apply -and $missingFromWorktree.Count -gt 0) {
    throw "Refusing to stage because selected pathspecs contain missing files. Refresh the submission gate first."
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $outputRoot "UE58ReviewedStaging_$timestamp.md"
$latestPath = Join-Path $outputRoot "LATEST.md"

$phase1Count = @($selectedEntries | Where-Object { $_.Category -eq "Phase1Candidate" }).Count
$evidenceCount = @($selectedEntries | Where-Object { $_.Category -eq "EvidenceOnly" }).Count
$manualReviewCount = @($selectedEntries | Where-Object { $_.Category -eq "NeedsManualReview" }).Count

$lines = @(
    "# UE5.8 Reviewed Staging",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Mode: $(if ($Apply) { "Apply" } else { "DryRun" })",
    "- Include evidence files: $IncludeEvidence",
    "- Include manual-review assets: $IncludeManualReview",
    "- Local branch behind origin/main: $gitBehind",
    "- Selected files: $($selectedPaths.Count)",
    "- Phase1Candidate files: $phase1Count",
    "- EvidenceOnly files: $evidenceCount",
    "- NeedsManualReview files: $manualReviewCount",
    "",
    "## Safety Result",
    ""
)

if ($Apply) {
    $lines += "- Staging was requested with -Apply."
}
else {
    $lines += "- Dry run only; no git index changes were made."
}

if ($gitBehind -and -not $AllowBehind) {
    $lines += "- Apply mode is blocked until the branch is no longer behind origin/main, or -AllowBehind is used intentionally."
}

if ($missingFromWorktree.Count -gt 0) {
    $lines += "- Missing selected paths: $($missingFromWorktree.Count)"
}
else {
    $lines += "- Missing selected paths: 0"
}

if ($notCurrentlyChanged.Count -gt 0) {
    $lines += "- Selected paths not currently changed: $($notCurrentlyChanged.Count)"
}
else {
    $lines += "- Selected paths not currently changed: 0"
}

$lines += @(
    "",
    "## Selected Files",
    "",
    "| Category | Path |",
    "| --- | --- |"
)

foreach ($entry in $selectedEntries) {
    $lines += "| $($entry.Category) | ``$($entry.Path)`` |"
}

if ($missingFromWorktree.Count -gt 0) {
    $lines += @(
        "",
        "## Missing Selected Paths",
        ""
    )
    foreach ($path in $missingFromWorktree) {
        $lines += "- ``$path``"
    }
}

if ($notCurrentlyChanged.Count -gt 0) {
    $lines += @(
        "",
        "## Selected Paths Not Currently Changed",
        ""
    )
    foreach ($path in $notCurrentlyChanged) {
        $lines += "- ``$path``"
    }
}

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

if ($Apply) {
    Push-Location $RepoRoot
    try {
        if ($selectedPaths.Count -gt 0) {
            $pathspecFile = [System.IO.Path]::GetTempFileName()
            try {
                $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
                [System.IO.File]::WriteAllLines($pathspecFile, $selectedPaths, $utf8NoBom)
                git add -f --pathspec-from-file="$pathspecFile"
                if ($LASTEXITCODE -ne 0) {
                    throw "git add failed with exit code $LASTEXITCODE."
                }
            }
            finally {
                Remove-Item -LiteralPath $pathspecFile -Force -ErrorAction SilentlyContinue
            }
        }
    }
    finally {
        Pop-Location
    }
}

Write-Output "Wrote reviewed staging report: $reportPath"
Write-Output "Updated reviewed staging latest: $latestPath"
if ($Apply) {
    Write-Output "Staged selected UE5.8 files."
}
else {
    Write-Output "Dry run complete; no git index changes were made."
}
