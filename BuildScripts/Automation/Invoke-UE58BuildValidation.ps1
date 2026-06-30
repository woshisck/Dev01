param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = "",
    [string]$OutputRoot = "",
    [switch]$RunBuild,
    [switch]$AllowOpenEditor,
    [bool]$AutoCloseEditor = $true
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$uprojectPath = Join-Path $RepoRoot "DevKit.uproject"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\BuildValidation"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Resolve-UE58BuildBat {
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
        $buildPath = Join-Path $candidate "Engine\Build\BatchFiles\Build.bat"
        if (Test-Path -LiteralPath $buildPath) {
            return [pscustomobject]@{
                Root = (Resolve-Path -LiteralPath $candidate).Path
                BuildBat = (Resolve-Path -LiteralPath $buildPath).Path
            }
        }
    }

    throw "UE5.8 Build.bat was not found. Set -EngineRoot or UE58_ENGINE_ROOT."
}

function Read-TextIfExists {
    param([string]$Path)
    if (Test-Path -LiteralPath $Path) {
        return Get-Content -LiteralPath $Path -Raw
    }
    return ""
}

function Get-UbtStatus {
    $ubtLogPath = Join-Path $env:LOCALAPPDATA "UnrealBuildTool\Log.txt"
    $text = Read-TextIfExists -Path $ubtLogPath
    $tail = if ([string]::IsNullOrWhiteSpace($text)) { @() } else { $text -split "`r?`n" | Select-Object -Last 180 }
    $resultLine = $tail | Where-Object { $_ -match "^Result:" } | Select-Object -Last 1
    $linkBlocked = (($tail | Select-String -Pattern "LNK1104|cannot open file.*UnrealEditor-.*\.dll|being used by another process").Count -gt 0)
    $lockedDlls = @()
    foreach ($line in $tail) {
        if ($line -match "cannot open file '([^']+UnrealEditor-[^']+\.dll)'") {
            $lockedDlls += $Matches[1]
        }
        elseif ($line -match "ERROR opening file (.+UnrealEditor-.+\.dll) for write") {
            $lockedDlls += $Matches[1]
        }
    }

    return [pscustomobject]@{
        LogPath = $ubtLogPath
        Result = if ($resultLine) { $resultLine.Trim() } else { "Unknown" }
        LinkBlocked = $linkBlocked
        LockedDlls = @($lockedDlls | Sort-Object -Unique)
        Tail = (($tail | Select-Object -Last 30) -join "`n")
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

$engine = Resolve-UE58BuildBat -RequestedRoot $EngineRoot
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58BuildValidation_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$openEditors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)
$editorCloseAttempted = $false
$editorForceClosed = $false
$buildCommandArgs = @(
    "DevKitEditor",
    "Win64",
    "Development",
    "-Project=$uprojectPath",
    "-WaitMutex",
    "-FromMsBuild"
)
$plannedCommand = "$($engine.BuildBat) $($buildCommandArgs -join ' ')"
$buildOutput = @()
$buildExitCode = ""
$blockedBeforeBuild = $false

if ($RunBuild -and $openEditors.Count -gt 0 -and -not $AllowOpenEditor) {
    if ($AutoCloseEditor) {
        $editorCloseAttempted = $true
        foreach ($process in $openEditors) {
            try {
                $null = $process.CloseMainWindow()
            }
            catch {
            }
        }

        Start-Sleep -Seconds 20
        $remainingEditors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)
        if ($remainingEditors.Count -gt 0) {
            $editorForceClosed = $true
            foreach ($process in $remainingEditors) {
                try {
                    Stop-Process -Id $process.Id -Force
                }
                catch {
                }
            }
            Start-Sleep -Seconds 3
        }

        $openEditors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue)
    }

    $blockedBeforeBuild = $openEditors.Count -gt 0
}

if ($RunBuild -and -not $blockedBeforeBuild) {
    $buildOutput = & $engine.BuildBat @buildCommandArgs 2>&1
    $buildExitCode = $LASTEXITCODE
}

$ubtStatus = Get-UbtStatus
$buildSucceeded = $ubtStatus.Result -eq "Result: Succeeded"
$status = if ($blockedBeforeBuild) {
    "BlockedByOpenEditor"
}
elseif ($RunBuild -and $buildSucceeded) {
    "BuildSucceeded"
}
elseif ($RunBuild) {
    "BuildFailed"
}
elseif ($ubtStatus.LinkBlocked) {
    "LatestBuildBlockedByOpenEditor"
}
elseif ($buildSucceeded) {
    "LatestBuildSucceeded"
}
else {
    "Prepared"
}

$lines = @(
    "# UE5.8 Build Validation",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Run build requested: $RunBuild",
    "- Allow open editor: $AllowOpenEditor",
    "- Auto close editor: $AutoCloseEditor",
    "- Editor close attempted: $editorCloseAttempted",
    "- Editor force closed: $editorForceClosed",
    "- Engine root: $($engine.Root)",
    "- Build.bat: $($engine.BuildBat)",
    "- Planned command: $plannedCommand",
    "- Open UnrealEditor process count: $($openEditors.Count)",
    "- Blocked before build: $blockedBeforeBuild",
    "- Build exit code: $buildExitCode",
    "- Latest UBT result: $($ubtStatus.Result)",
    "- Latest UBT link blocked: $($ubtStatus.LinkBlocked)",
    "- UBT log: $($ubtStatus.LogPath)",
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
    "## Locked DLL Evidence",
    "",
    "| DLL |",
    "| --- |"
)

if ($ubtStatus.LockedDlls.Count -gt 0) {
    foreach ($dll in $ubtStatus.LockedDlls) {
        $lines += "| ``$(ConvertTo-RelativePath -Path $dll)`` |"
    }
}
else {
    $lines += "| (none detected in latest UBT tail) |"
}

$lines += @(
    "",
    "## Build Output",
    "",
    '```text'
)

if ($buildOutput.Count -gt 0) {
    $lines += $buildOutput
}
else {
    $lines += "(build was not run by this invocation)"
}

$lines += @(
    '```',
    "",
    "## UBT Tail",
    "",
    '```text',
    $ubtStatus.Tail,
    '```',
    "",
    "## Acceptance Notes",
    "",
    "- Default mode only records the latest UBT state and open-editor locks; it does not compile.",
    "- `-RunBuild` compiles `DevKitEditor Win64 Development` only when explicitly requested.",
    "- If UnrealEditor is open, `-RunBuild` closes it by default before compiling; pass `-AutoCloseEditor:`$false to record a block instead.",
    "- `-AllowOpenEditor` bypasses the close/block guard for explicit stale-binary or lock investigations."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote build validation report: $reportPath"
Write-Output "Updated latest build validation: $latestPath"
Write-Output "Status: $status"

if ($RunBuild -and -not $buildSucceeded) {
    exit 1
}
