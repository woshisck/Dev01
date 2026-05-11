param(
    [string]$EngineDir = $env:UE_ENGINE_DIR,
    [string]$ProjectFile = "",
    [string]$ArchiveRoot = "",
    [switch]$Clean,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Resolve-EngineRoot {
    param([string]$Candidate)

    if ([string]::IsNullOrWhiteSpace($Candidate)) {
        return $null
    }

    $Resolved = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Candidate)
    if (Test-Path (Join-Path $Resolved "Engine\Build\BatchFiles\RunUAT.bat")) {
        return $Resolved
    }

    if (Split-Path $Resolved -Leaf | Where-Object { $_ -ieq "Engine" }) {
        $Parent = Split-Path $Resolved -Parent
        if (Test-Path (Join-Path $Parent "Engine\Build\BatchFiles\RunUAT.bat")) {
            return $Parent
        }
    }

    return $null
}

function Find-EngineRoot {
    param([string]$Preferred)

    $Candidates = @(
        $Preferred,
        "D:\Code\UE_Engine\UE_5.4",
        "D:\UE_src\5.4\UnrealEngine-5.4",
        "C:\Program Files\Epic Games\UE_5.4",
        "D:\Program Files\Epic Games\UE_5.4"
    )

    foreach ($Candidate in $Candidates) {
        $Root = Resolve-EngineRoot $Candidate
        if ($Root) {
            return $Root
        }
    }

    throw "Could not find Unreal Engine 5.4. Set UE_ENGINE_DIR to your Unreal Engine root, for example: D:\Code\UE_Engine\UE_5.4"
}

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    Write-Host ""
    Write-Host "Running:" -ForegroundColor Cyan
    Write-Host "  `"$FilePath`" $($Arguments -join ' ')" -ForegroundColor Cyan

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE."
    }
}

function Get-RelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $BaseFullPath = [System.IO.Path]::GetFullPath($BasePath)
    $TargetFullPath = [System.IO.Path]::GetFullPath($TargetPath)

    if (!$BaseFullPath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $BaseFullPath += [System.IO.Path]::DirectorySeparatorChar
    }

    $BaseUri = New-Object System.Uri($BaseFullPath)
    $TargetUri = New-Object System.Uri($TargetFullPath)
    $RelativeUri = $BaseUri.MakeRelativeUri($TargetUri)
    return [System.Uri]::UnescapeDataString($RelativeUri.ToString()).Replace('/', [System.IO.Path]::DirectorySeparatorChar)
}

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
if ([string]::IsNullOrWhiteSpace($ProjectFile)) {
    $ProjectFile = Join-Path $ProjectRoot "DevKit.uproject"
}
$ProjectFile = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ProjectFile)
if (!(Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}

$EngineRoot = Find-EngineRoot $EngineDir
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$BuildConfig = "Development"
$Platform = "Win64"
$Target = "DevKit"

if ([string]::IsNullOrWhiteSpace($ArchiveRoot)) {
    $ArchiveRoot = Join-Path $ProjectRoot "Build\Packages"
}
$ArchiveRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ArchiveRoot)
$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$ArchiveDir = Join-Path $ArchiveRoot "$Target-$Platform-$BuildConfig-$Timestamp"
$LogDir = Join-Path $ArchiveDir "_Logs"

New-Item -ItemType Directory -Force -Path $ArchiveDir | Out-Null
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$UatArgs = @(
    "BuildCookRun",
    "-project=`"$ProjectFile`"",
    "-noP4",
    "-platform=$Platform",
    "-target=$Target",
    "-clientconfig=$BuildConfig",
    "-build",
    "-cook",
    "-allmaps",
    "-stage",
    "-pak",
    "-iostore",
    "-archive",
    "-archivedirectory=`"$ArchiveDir`"",
    "-prereqs",
    "-CrashReporter",
    "-utf8output"
)

if ($Clean) {
    $UatArgs += "-clean"
}
if ($SkipBuild) {
    $UatArgs = $UatArgs | Where-Object { $_ -ne "-build" }
    $UatArgs += "-skipbuild"
}

Write-Host "Packaging $Target $Platform $BuildConfig..." -ForegroundColor Green
Write-Host "Engine:  $EngineRoot"
Write-Host "Project: $ProjectFile"
Write-Host "Output:  $ArchiveDir"

Invoke-Checked -FilePath $RunUAT -Arguments $UatArgs

$WindowsPackageDir = Join-Path $ArchiveDir "Windows"
if (!(Test-Path $WindowsPackageDir)) {
    $WindowsPackageDir = $ArchiveDir
}

$GameExe = Get-ChildItem -Path $WindowsPackageDir -Recurse -Filter "$Target.exe" -File |
    Sort-Object FullName |
    Select-Object -First 1

if (!$GameExe) {
    $GameExe = Get-ChildItem -Path $WindowsPackageDir -Recurse -Filter "$Target*.exe" -File |
    Sort-Object FullName |
    Select-Object -First 1
}

if ($GameExe) {
    $LauncherPath = Join-Path $WindowsPackageDir "Play $Target.bat"
    $RelativeExe = Get-RelativePath -BasePath $WindowsPackageDir -TargetPath $GameExe.FullName
    @"
@echo off
setlocal
cd /d "%~dp0"
start "" "$RelativeExe"
"@ | Set-Content -Path $LauncherPath -Encoding ASCII
}

$ReadmePath = Join-Path $WindowsPackageDir "README_PLAY.txt"
@"
$Target $Platform $BuildConfig package

How to play:
1. Double-click "Play $Target.bat" if present, or run "$Target.exe".
2. If Windows asks for prerequisites, run Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe from this package.

Build time: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Project: $ProjectFile
"@ | Set-Content -Path $ReadmePath -Encoding ASCII

Write-Host ""
Write-Host "Package complete:" -ForegroundColor Green
Write-Host "  $WindowsPackageDir"
if ($GameExe) {
    Write-Host "Launcher:"
    Write-Host "  $(Join-Path $WindowsPackageDir "Play $Target.bat")"
}
