param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = "",
    [string]$OutputRoot = "",
    [int]$TimeoutSec = 300,
    [switch]$Run
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$uprojectPath = Join-Path $RepoRoot "DevKit.uproject"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RequiredTests"
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

function Test-LogPassed {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return $false
    }

    return (Get-Content -LiteralPath $Path -Raw) -match "TEST COMPLETE\. EXIT CODE: 0"
}

function Get-TestCountFromLog {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return ""
    }

    $text = Get-Content -LiteralPath $Path -Raw
    if ($text -match "Found\s+([0-9]+)\s+automation tests based on") {
        return $Matches[1]
    }

    return ""
}

function Invoke-TestSuite {
    param(
        [string]$EditorExe,
        [string]$ProjectPath,
        [string]$Name,
        [string]$Filter,
        [string]$LogPath
    )

    $arguments = @(
        "`"$ProjectPath`"",
        "-Unattended",
        "-NullRHI",
        "-NoSplash",
        "-NoSound",
        "-stdout",
        "-FullStdOutLogOutput",
        "-AbsLog=`"$LogPath`"",
        "-ExecCmds=`"Automation RunTests $Filter; Quit`"",
        "-TestExit=`"Automation Test Queue Empty`""
    )

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

    $passed = Test-LogPassed -Path $LogPath
    return [pscustomobject]@{
        Name = $Name
        Filter = $Filter
        LogPath = $LogPath
        ExitCode = if ($completed) { $process.ExitCode } else { $null }
        TimedOut = (-not $completed)
        Passed = $passed
        TestCount = Get-TestCountFromLog -Path $LogPath
        Command = "$EditorExe $($arguments -join ' ')"
    }
}

$engine = Resolve-UE58EngineRoot -RequestedRoot $EngineRoot
$savedLogsRoot = Join-Path $RepoRoot "Saved\Logs"
$suites = @(
    [pscustomobject]@{ Name = "DevKit.Performance.Settings"; Filter = "DevKit.Performance.Settings"; Log = Join-Path $savedLogsRoot "Codex_PerformanceSettings_Focus_Tests.log" },
    [pscustomobject]@{ Name = "DevKitEditor.UI"; Filter = "DevKitEditor.UI"; Log = Join-Path $savedLogsRoot "Codex_UI_Focus_Tests.log" },
    [pscustomobject]@{ Name = "DevKitEditor.MaterialBatch"; Filter = "DevKitEditor.MaterialBatch"; Log = Join-Path $savedLogsRoot "Codex_MaterialBatchTests.log" },
    [pscustomobject]@{ Name = "DevKitEditor.UE58Performance"; Filter = "DevKitEditor.UE58Performance"; Log = Join-Path $savedLogsRoot "Codex_UE58Performance_Tests.log" }
)

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58RequiredTests_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$results = @()

if ($Run) {
    foreach ($suite in $suites) {
        $results += Invoke-TestSuite -EditorExe $engine.Editor -ProjectPath $uprojectPath -Name $suite.Name -Filter $suite.Filter -LogPath $suite.Log
    }
}
else {
    foreach ($suite in $suites) {
        $results += [pscustomobject]@{
            Name = $suite.Name
            Filter = $suite.Filter
            LogPath = $suite.Log
            ExitCode = ""
            TimedOut = $false
            Passed = Test-LogPassed -Path $suite.Log
            TestCount = Get-TestCountFromLog -Path $suite.Log
            Command = "$($engine.Editor) `"$uprojectPath`" -Unattended -NullRHI -NoSplash -NoSound -stdout -FullStdOutLogOutput -AbsLog=`"$($suite.Log)`" -ExecCmds=`"Automation RunTests $($suite.Filter); Quit`" -TestExit=`"Automation Test Queue Empty`""
        }
    }
}

$allPassed = (($results | Where-Object { -not $_.Passed }) | Measure-Object).Count -eq 0
$status = if ($Run) {
    if ($allPassed) { "Passed" } else { "Failed" }
}
else {
    if ($allPassed) { "PreparedExistingLogsPass" } else { "PreparedExistingLogsMissingOrFailing" }
}

function ConvertTo-RelativePath {
    param([string]$Path)

    if ($Path.StartsWith($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Path.Substring($RepoRoot.Length).TrimStart("\", "/")
    }
    return $Path
}

$lines = @(
    "# UE5.8 Required Automation Tests",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Engine root: $($engine.Root)",
    "- Editor executable: $($engine.Editor)",
    "- Run requested: $Run",
    "- Status: $status",
    "- Timeout per suite: $TimeoutSec seconds",
    "",
    "## Suites",
    "",
    "| Suite | Filter | Passed | Test Count | ExitCode | TimedOut | Log |",
    "| --- | --- | --- | ---: | ---: | --- | --- |"
)

foreach ($result in $results) {
    $relativeLog = ConvertTo-RelativePath -Path $result.LogPath
    $lines += "| $($result.Name) | ``$($result.Filter)`` | $($result.Passed) | $($result.TestCount) | $($result.ExitCode) | $($result.TimedOut) | ``$relativeLog`` |"
}

$lines += @(
    "",
    "## Commands",
    ""
)
foreach ($result in $results) {
    $lines += "### $($result.Name)"
    $lines += ""
    $lines += '```text'
    $lines += $result.Command
    $lines += '```'
    $lines += ""
}

$lines += @(
    "## Acceptance Notes",
    "",
    "- Final upload automation must run this script with `-Run` after the reviewed scope is staged and before commit/push.",
    "- Existing passing logs are useful for review, but `Status: Passed` requires a fresh run in this script."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote required tests report: $reportPath"
Write-Output "Updated required tests latest: $latestPath"
Write-Output "Status: $status"

if ($Run -and -not $allPassed) {
    exit 1
}
