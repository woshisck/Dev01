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

    if ((Split-Path $Resolved -Leaf) -ieq "Engine") {
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
        "D:\UE\UE_5.8",
        "D:\Code\UE_Engine\UE_5.8",
        "D:\Program Files\Epic Games\UE_5.8",
        "C:\Program Files\Epic Games\UE_5.8"
    )

    foreach ($Candidate in $Candidates) {
        $Root = Resolve-EngineRoot $Candidate
        if ($Root) {
            return $Root
        }
    }

    throw "Could not find Unreal Engine 5.8. Set UE_ENGINE_DIR to your UE 5.8 root, for example: D:\UE\UE_5.8"
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
    $ArchiveRoot = Join-Path $ProjectRoot "Build\Packages\PlayablePerfTest"
}
$ArchiveRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ArchiveRoot)
$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$ArchiveDir = Join-Path $ArchiveRoot "$Target-$Platform-$BuildConfig-PlayablePerfTest-$Timestamp"

New-Item -ItemType Directory -Force -Path $ArchiveDir | Out-Null

$Maps = @(
    "/Game/Maps/L_EntryMenu",
    "/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom"
)
$MapArg = "-map=" + ($Maps -join "+")

$UatArgs = @(
    "BuildCookRun",
    "-project=`"$ProjectFile`"",
    "-noP4",
    "-platform=$Platform",
    "-target=$Target",
    "-clientconfig=$BuildConfig",
    "-build",
    "-cook",
    $MapArg,
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

Write-Host "Packaging playable performance test: $Target $Platform $BuildConfig" -ForegroundColor Green
Write-Host "Engine:  $EngineRoot"
Write-Host "Project: $ProjectFile"
Write-Host "Maps:    $($Maps -join ', ')"
Write-Host "Output:  $ArchiveDir"

Invoke-Checked -FilePath $RunUAT -Arguments $UatArgs

$WindowsPackageDir = Join-Path $ArchiveDir "Windows"
if (!(Test-Path $WindowsPackageDir)) {
    $WindowsPackageDir = $ArchiveDir
}

$GameExe = Get-ChildItem -Path $WindowsPackageDir -Recurse -Filter "$Target.exe" -File -ErrorAction SilentlyContinue |
    Sort-Object FullName |
    Select-Object -First 1

if (!$GameExe) {
    $GameExe = Get-ChildItem -Path $WindowsPackageDir -Recurse -Filter "$Target*.exe" -File -ErrorAction SilentlyContinue |
        Sort-Object FullName |
        Select-Object -First 1
}

if ($GameExe) {
    $LauncherPath = Join-Path $WindowsPackageDir "Play $Target Playable Perf Test.bat"
    $SmokeLauncherPath = Join-Path $WindowsPackageDir "Run Runtime GM Smoke Test.bat"
    $RelativeExe = Get-RelativePath -BasePath $WindowsPackageDir -TargetPath $GameExe.FullName
    @"
@echo off
setlocal
cd /d "%~dp0"
start "" "$RelativeExe" -log
"@ | Set-Content -Path $LauncherPath -Encoding ASCII

    @"
@echo off
setlocal
cd /d "%~dp0"
"$RelativeExe" /Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom -log -nosound -RuntimeGMSmokeTest -RuntimeGMSmokeExit -RuntimeGMSmokeSpawnCount=1 -RuntimeGMSmokeSpawnRadius=1200
"@ | Set-Content -Path $SmokeLauncherPath -Encoding ASCII
}

$ReadmePath = Join-Path $WindowsPackageDir "README_PLAYABLE_PERF_TEST.txt"
@"
$Target $Platform $BuildConfig playable performance test package

Purpose:
- Start from the normal entry menu.
- Enter the playable InitialRoom flow.
- Use real player movement, combat, pause UI, enemy spawning, and performance tools.
- Press F12 in a Development build to open the Runtime GM panel.

Runtime GM default assets:
- Configure them in UE: Tools -> DevKit Tools -> Performance Tools -> Runtime GM Settings
- The settings are stored under [/Script/DevKit.YogRuntimeGMSettings] in Config/DefaultGame.ini.

Automated smoke test:
- Run `Run Runtime GM Smoke Test.bat`.
- It opens InitialRoom directly and calls Runtime GM: give configured weapon, spawn one configured enemy near the player, then reset player/enemy state.
- Check the log for `[RuntimeGM][Smoke] PASSED`.

Packaged maps:
- /Game/Maps/L_EntryMenu
- /Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom

Build time: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Project: $ProjectFile
"@ | Set-Content -Path $ReadmePath -Encoding UTF8

Write-Host ""
Write-Host "Playable performance test package complete:" -ForegroundColor Green
Write-Host "  $WindowsPackageDir"
if ($GameExe) {
    Write-Host "Launcher:"
    Write-Host "  $(Join-Path $WindowsPackageDir "Play $Target Playable Perf Test.bat")"
}
