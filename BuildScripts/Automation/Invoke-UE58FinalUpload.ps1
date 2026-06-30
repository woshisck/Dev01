param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [string]$EngineRoot = "",
    [string]$CommitMessage = "",
    [switch]$IncludeEvidence,
    [switch]$IncludeManualReview,
    [switch]$ApprovedScope,
    [switch]$Apply,
    [switch]$StopEditorForBuild
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$uprojectPath = Join-Path $RepoRoot "DevKit.uproject"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\FinalUpload"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Resolve-UE58EngineRoot {
    param([string]$RequestedRoot)

    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $candidates += $RequestedRoot
    }
    if (-not [string]::IsNullOrWhiteSpace($env:UE58_ENGINE_ROOT)) {
        $candidates += $env:UE58_ENGINE_ROOT
    }
    $candidates += "D:\UE\UE_5.8"
    $candidates += "D:\Epic Library\UE_5.8"

    foreach ($candidate in $candidates) {
        $buildPath = Join-Path $candidate "Engine\Build\BatchFiles\Build.bat"
        if (Test-Path -LiteralPath $buildPath) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "UE5.8 engine root was not found. Set -EngineRoot or UE58_ENGINE_ROOT."
}

function Test-FileText {
    param(
        [string]$Path,
        [string]$Pattern
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return $false
    }

    return (Get-Content -LiteralPath $Path -Raw) -match $Pattern
}

function Get-ReportBool {
    param(
        [string]$Text,
        [string]$Label
    )

    $pattern = "(?m)^- $([regex]::Escape($Label)): (True|False)\s*$"
    if ($Text -match $pattern) {
        return [bool]::Parse($Matches[1])
    }

    return $false
}

function Get-ReportValue {
    param(
        [string]$Text,
        [string]$Label
    )

    $pattern = "(?m)^- $([regex]::Escape($Label)): (.+?)\s*$"
    if ($Text -match $pattern) {
        return $Matches[1]
    }

    return ""
}

function Invoke-NativeProcessLines {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$WorkingDirectory
    )

    $stdoutPath = [System.IO.Path]::GetTempFileName()
    $stderrPath = [System.IO.Path]::GetTempFileName()
    try {
        $process = Start-Process -FilePath $FilePath `
            -ArgumentList $Arguments `
            -WorkingDirectory $WorkingDirectory `
            -NoNewWindow `
            -Wait `
            -PassThru `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath

        $lines = @()
        if (Test-Path -LiteralPath $stdoutPath) {
            $lines += Get-Content -LiteralPath $stdoutPath
        }
        if (Test-Path -LiteralPath $stderrPath) {
            $lines += Get-Content -LiteralPath $stderrPath
        }

        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Lines = @($lines)
        }
    }
    finally {
        Remove-Item -LiteralPath $stdoutPath -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $stderrPath -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-UEBuild {
    param(
        [string]$ResolvedEngineRoot,
        [string]$ProjectPath
    )

    $buildBat = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\Build.bat"
    if (-not (Test-Path -LiteralPath $buildBat)) {
        throw "Build.bat was not found: $buildBat"
    }

    & $buildBat DevKitEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -FromMsBuild
    if ($LASTEXITCODE -ne 0) {
        throw "UE build failed with exit code $LASTEXITCODE."
    }
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58FinalUpload_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$submissionGateScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Write-UE58SubmissionGateReport.ps1"
$reviewedStagingScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58ReviewedStaging.ps1"
$requiredTestsScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58RequiredTests.ps1"
$remoteDeltaAuditScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58RemoteDeltaAudit.ps1"
$artifactContractScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Test-UE58PerformanceArtifactContract.ps1"
$submissionGateLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SubmissionGate\LATEST.md"
$remoteDeltaAuditLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RemoteDeltaAudit\LATEST.md"
$artifactContractLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\ArtifactContract\LATEST.md"

if (-not (Test-Path -LiteralPath $submissionGateScriptPath)) {
    throw "Submission gate script was not found: $submissionGateScriptPath"
}
if (-not (Test-Path -LiteralPath $reviewedStagingScriptPath)) {
    throw "Reviewed staging script was not found: $reviewedStagingScriptPath"
}
if (-not (Test-Path -LiteralPath $requiredTestsScriptPath)) {
    throw "Required tests script was not found: $requiredTestsScriptPath"
}
if (-not (Test-Path -LiteralPath $remoteDeltaAuditScriptPath)) {
    throw "Remote delta audit script was not found: $remoteDeltaAuditScriptPath"
}
if (-not (Test-Path -LiteralPath $artifactContractScriptPath)) {
    throw "Artifact contract script was not found: $artifactContractScriptPath"
}

$engineRootResolved = Resolve-UE58EngineRoot -RequestedRoot $EngineRoot
$buildCommand = "$(Join-Path $engineRootResolved "Engine\Build\BatchFiles\Build.bat") DevKitEditor Win64 Development -Project=$uprojectPath -WaitMutex -FromMsBuild"

$fetchLatestRemoteBeforeUpload = $true
$remoteAuditRequiredBeforeUpload = $true
$fetchOutput = @()
$fetchStatus = "NotRun"
$remoteAuditOutput = @()
$remoteAuditRunStatus = "NotRun"
$remoteAuditStatusAfterFetch = "Unknown"
$artifactContractOutput = @()
$artifactContractRunStatus = "NotRun"
$artifactContractStatus = "Unknown"

Push-Location $RepoRoot
try {
    $fetchResult = Invoke-NativeProcessLines -FilePath "git" -Arguments @("fetch", "--prune", "origin") -WorkingDirectory $RepoRoot
    $fetchOutput = $fetchResult.Lines
    if ($fetchResult.ExitCode -ne 0) {
        throw "git fetch --prune origin failed with exit code $($fetchResult.ExitCode)."
    }
    $fetchStatus = "Succeeded"
}
catch {
    $fetchStatus = "Failed: $($_.Exception.Message)"
}
finally {
    Pop-Location
}

if ($fetchStatus -eq "Succeeded") {
    try {
        $remoteAuditOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $remoteDeltaAuditScriptPath -RepoRoot $RepoRoot 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "Remote delta audit failed with exit code $LASTEXITCODE."
        }
        $remoteAuditRunStatus = "Ran"
    }
    catch {
        $remoteAuditRunStatus = "Failed: $($_.Exception.Message)"
    }
}
else {
    $remoteAuditRunStatus = "SkippedBecauseFetchFailed"
}

if (Test-Path -LiteralPath $remoteDeltaAuditLatestPath) {
    $remoteAuditTextAfterFetch = Get-Content -LiteralPath $remoteDeltaAuditLatestPath -Raw
    $remoteAuditStatusAfterFetch = Get-ReportValue -Text $remoteAuditTextAfterFetch -Label "Status"
    if ([string]::IsNullOrWhiteSpace($remoteAuditStatusAfterFetch)) {
        $remoteAuditStatusAfterFetch = "Unknown"
    }
}

try {
    $artifactContractOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $artifactContractScriptPath -RepoRoot $RepoRoot 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Artifact contract failed with exit code $LASTEXITCODE."
    }
    $artifactContractRunStatus = "Ran"
}
catch {
    $artifactContractRunStatus = "Failed: $($_.Exception.Message)"
}

if (Test-Path -LiteralPath $artifactContractLatestPath) {
    $artifactContractText = Get-Content -LiteralPath $artifactContractLatestPath -Raw
    $artifactContractStatus = Get-ReportValue -Text $artifactContractText -Label "Status"
    if ([string]::IsNullOrWhiteSpace($artifactContractStatus)) {
        $artifactContractStatus = "Unknown"
    }
}

Push-Location $RepoRoot
try {
    $gitStatusBefore = (git status --short --branch) 2>$null
    $remoteDeltaBefore = (git log --oneline --left-right --cherry-pick main...origin/main) 2>$null
    $cachedBefore = (git diff --cached --name-only) 2>$null
}
finally {
    Pop-Location
}

$gateOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $submissionGateScriptPath -RepoRoot $RepoRoot 2>&1
$gateText = if (Test-Path -LiteralPath $submissionGateLatestPath) { Get-Content -LiteralPath $submissionGateLatestPath -Raw } else { "" }

$gitBehind = ($gitStatusBefore -join "`n") -match "behind"
$workingTreeDirty = (($gitStatusBefore | Where-Object { $_ -notmatch "^## " }) | Measure-Object).Count -gt 0
$cachedDirtyBefore = @($cachedBefore).Count -gt 0
$allReady = (Get-ReportBool -Text $gateText -Label "Static audit ready") -and
    (Get-ReportBool -Text $gateText -Label "Batch visual MCP ready") -and
    (Get-ReportBool -Text $gateText -Label "Scene parity MCP ready") -and
    (Get-ReportBool -Text $gateText -Label "Runtime profiling MCP smoke ready") -and
    (Get-ReportBool -Text $gateText -Label "Artifact contract ready") -and
    (Get-ReportBool -Text $gateText -Label "Commandlet availability ready") -and
    (Get-ReportBool -Text $gateText -Label "MaterialBatch dry-run ready") -and
    (Get-ReportBool -Text $gateText -Label "Runtime profiling capture ready")
$testsReady = $gateText -match "\| DevKit\.Performance\.Settings \| True \|" -and
    $gateText -match "\| DevKitEditor\.UI \| True \|" -and
    $gateText -match "\| DevKitEditor\.MaterialBatch \| True \|" -and
    $gateText -match "\| DevKitEditor\.UE58Performance \| True \|"
$buildAlreadySucceeded = $gateText -match "(?m)^- Latest UE build: Result: Succeeded\s*$"
$editorProcesses = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)

$preflightBlocks = @()
if ($fetchStatus -ne "Succeeded") {
    $preflightBlocks += "Could not fetch latest origin/main before upload; do not commit or push until the remote state is known."
}
if ($remoteAuditRunStatus -ne "Ran") {
    $preflightBlocks += "Remote delta audit did not run after fetch; upload requires a fresh remote audit."
}
if ($remoteAuditStatusAfterFetch -notin @("ReadyForReviewedMerge", "NoRemoteDelta")) {
    $preflightBlocks += "Fresh remote delta audit status is $remoteAuditStatusAfterFetch; review and resolve remote additions before final upload."
}
if ($artifactContractRunStatus -ne "Ran") {
    $preflightBlocks += "Artifact contract did not run; final upload requires a fresh static artifact contract check."
}
if ($artifactContractStatus -ne "Passed") {
    $preflightBlocks += "Artifact contract status is $artifactContractStatus; fix or rerun Test-UE58PerformanceArtifactContract.ps1 before final upload."
}
if ($gitBehind) {
    $preflightBlocks += "Local main is behind origin/main; merge/rebase remote changes before final upload."
}
if ($cachedDirtyBefore) {
    $preflightBlocks += "Git index already has staged files; clear or review the index before final upload automation."
}
if (-not $allReady) {
    $preflightBlocks += "One or more UE5.8 evidence gates are not ready."
}
if (-not $testsReady) {
    $preflightBlocks += "One or more required automation test logs are missing or not passing."
}
if ($Apply -and -not $ApprovedScope) {
    $preflightBlocks += "-Apply requires -ApprovedScope so the commit only happens after explicit user approval."
}
if ($Apply -and [string]::IsNullOrWhiteSpace($CommitMessage)) {
    $preflightBlocks += "-Apply requires a non-empty -CommitMessage."
}
if ($Apply -and $editorProcesses.Count -gt 0 -and -not $StopEditorForBuild) {
    $preflightBlocks += "UnrealEditor is running. Close it or pass -StopEditorForBuild before compiling."
}

$stagingStatus = "NotRun"
$preTestBuildStatus = "NotRun"
$testsRunStatus = "NotRun"
$finalBuildStatus = "NotRun"
$commitStatus = "NotRun"
$pushStatus = "NotRun"
$commitHash = ""
$pushOutput = @()
$applyError = ""

if ($Apply -and $preflightBlocks.Count -eq 0) {
    try {
        if ($editorProcesses.Count -gt 0 -and $StopEditorForBuild) {
            foreach ($process in $editorProcesses) {
                Stop-Process -Id $process.Id -Force
            }
            Start-Sleep -Seconds 3
        }

        $stagingArgs = @(
            "-NoProfile",
            "-ExecutionPolicy", "Bypass",
            "-File", $reviewedStagingScriptPath,
            "-RepoRoot", $RepoRoot,
            "-Apply"
        )
        if ($IncludeEvidence) {
            $stagingArgs += "-IncludeEvidence"
        }
        if ($IncludeManualReview) {
            $stagingArgs += "-IncludeManualReview"
        }
        $stagingOutput = & powershell.exe @stagingArgs 2>&1
        $stagingStatus = "Applied"

        Invoke-UEBuild -ResolvedEngineRoot $engineRootResolved -ProjectPath $uprojectPath
        $preTestBuildStatus = "Succeeded"

        $testOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $requiredTestsScriptPath -RepoRoot $RepoRoot -EngineRoot $engineRootResolved -Run 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "Required automation tests failed. $($testOutput -join ' ')"
        }
        $testsRunStatus = "Passed"

        Invoke-UEBuild -ResolvedEngineRoot $engineRootResolved -ProjectPath $uprojectPath
        $finalBuildStatus = "Succeeded"

        Push-Location $RepoRoot
        try {
            git commit -m $CommitMessage
            if ($LASTEXITCODE -ne 0) {
                throw "git commit failed with exit code $LASTEXITCODE."
            }
            $commitHash = (git rev-parse --short HEAD) 2>$null
            $commitStatus = "Committed"

            $pushOutput = git push origin main 2>&1
            if ($LASTEXITCODE -ne 0) {
                throw "git push failed with exit code $LASTEXITCODE."
            }
            $pushStatus = "Pushed"
        }
        finally {
            Pop-Location
        }
    }
    catch {
        $applyError = $_.Exception.Message
        if ($preTestBuildStatus -eq "NotRun" -and $stagingStatus -eq "Applied") {
            $preTestBuildStatus = "FailedOrSkipped"
        }
        if ($testsRunStatus -eq "NotRun" -and $preTestBuildStatus -eq "Succeeded") {
            $testsRunStatus = "FailedOrSkipped"
        }
        if ($finalBuildStatus -eq "NotRun" -and $testsRunStatus -eq "Passed") {
            $finalBuildStatus = "FailedOrSkipped"
        }
        if ($commitStatus -eq "NotRun" -and $finalBuildStatus -eq "Succeeded") {
            $commitStatus = "FailedOrSkipped"
        }
        if ($pushStatus -eq "NotRun" -and $commitStatus -eq "Committed") {
            $pushStatus = "FailedOrSkipped"
        }
    }
}

Push-Location $RepoRoot
try {
    $gitStatusAfter = (git status --short --branch) 2>$null
    $cachedAfter = (git diff --cached --name-only) 2>$null
}
finally {
    Pop-Location
}

$lines = @(
    "# UE5.8 Final Upload Automation",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Mode: $(if ($Apply) { "Apply" } else { "DryRun" })",
    "- Approved scope: $ApprovedScope",
    "- Include evidence files: $IncludeEvidence",
    "- Include manual-review assets: $IncludeManualReview",
    "- Engine root: $engineRootResolved",
    "- Build command: $buildCommand",
    "- Fetch latest remote before upload: $fetchLatestRemoteBeforeUpload",
    "- Remote audit required before upload: $remoteAuditRequiredBeforeUpload",
    "- Fetch latest remote status: $fetchStatus",
    "- Remote audit run status after fetch: $remoteAuditRunStatus",
    "- Remote audit status after fetch: $remoteAuditStatusAfterFetch",
    "- Artifact contract run status: $artifactContractRunStatus",
    "- Artifact contract status: $artifactContractStatus",
    "- Working tree dirty before: $workingTreeDirty",
    "- Local branch behind origin/main before: $gitBehind",
    "- Cached files before: $(@($cachedBefore).Count)",
    "- Evidence gates ready: $allReady",
    "- Test logs ready: $testsReady",
    "- Previous build recorded succeeded: $buildAlreadySucceeded",
    "- UnrealEditor processes before: $($editorProcesses.Count)",
    "- Staging status: $stagingStatus",
    "- Pre-test build status: $preTestBuildStatus",
    "- Required tests run status: $testsRunStatus",
    "- Final pre-upload build status: $finalBuildStatus",
    "- Commit status: $commitStatus",
    "- Push status: $pushStatus",
    "- Commit hash: $commitHash",
    "- Cached files after: $(@($cachedAfter).Count)",
    "",
    "## Preflight Blocks",
    ""
)

if ($preflightBlocks.Count -gt 0) {
    foreach ($block in $preflightBlocks) {
        $lines += "- $block"
    }
}
else {
    $lines += "- None."
}

if (-not [string]::IsNullOrWhiteSpace($applyError)) {
    $lines += @(
        "",
        "## Apply Error",
        "",
        "- $applyError"
    )
}

$lines += @(
    "",
    "## Fetch Latest Remote",
    "",
    '```text'
)
if ($fetchOutput.Count -gt 0) {
    $lines += $fetchOutput
}
else {
    $lines += "(no fetch output)"
}
$lines += '```'

$lines += @(
    "",
    "## Remote Delta Audit After Fetch",
    "",
    '```text'
)
if ($remoteAuditOutput.Count -gt 0) {
    $lines += $remoteAuditOutput
}
else {
    $lines += "(remote delta audit did not produce output)"
}
$lines += '```'

$lines += @(
    "",
    "## Artifact Contract",
    "",
    '```text'
)
if ($artifactContractOutput.Count -gt 0) {
    $lines += $artifactContractOutput
}
else {
    $lines += "(artifact contract did not produce output)"
}
$lines += '```'

$lines += @(
    "",
    "## Submission Gate Refresh",
    "",
    '```text'
)
$lines += $gateOutput
$lines += '```'

$lines += @(
    "",
    "## Remote Delta Before",
    "",
    '```text'
)
if ($remoteDeltaBefore) {
    $lines += $remoteDeltaBefore
}
else {
    $lines += "(no remote delta)"
}
$lines += '```'

if ($pushOutput.Count -gt 0) {
    $lines += @(
        "",
        "## Push Output",
        "",
        '```text'
    )
    $lines += $pushOutput
    $lines += '```'
}

$lines += @(
    "",
    "## Git Status After",
    "",
    '```text'
)
if ($gitStatusAfter) {
    $lines += $gitStatusAfter
}
else {
    $lines += "(git status unavailable or empty)"
}
$lines += '```'

$lines += @(
    "",
    "## Required Manual Sequence Before Apply",
    "",
    "1. Fetch origin/main and review any newly added remote commits.",
    "2. Review the Phase1Candidate, EvidenceOnly, and NeedsManualReview pathspecs.",
    "3. Resolve the origin/main delta without reverting UE5.8 work.",
    "4. Re-run this dry-run and confirm preflight blocks are clear.",
    "5. Run this script with `-Apply -ApprovedScope -CommitMessage ...` after explicit approval.",
    "6. In apply mode, the script stages reviewed files, compiles, runs required tests, compiles again, commits, and pushes."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote final upload report: $reportPath"
Write-Output "Updated final upload latest: $latestPath"
Write-Output "Mode: $(if ($Apply) { "Apply" } else { "DryRun" })"
Write-Output "Preflight blocks: $($preflightBlocks.Count)"
Write-Output "Staging status: $stagingStatus"
Write-Output "Pre-test build status: $preTestBuildStatus"
Write-Output "Required tests run status: $testsRunStatus"
Write-Output "Final pre-upload build status: $finalBuildStatus"
Write-Output "Commit status: $commitStatus"
Write-Output "Push status: $pushStatus"

if ($Apply -and (-not [string]::IsNullOrWhiteSpace($applyError))) {
    exit 1
}
