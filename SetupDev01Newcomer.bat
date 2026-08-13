@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "DEV01_P4PORT=ssl:124.223.187.156:1666"
set "DEV01_STREAM=//Dev01/main"
set "DEV01_UGS_DEPOT=//Dev01Binaries/Tools/UnrealGameSync/Release.zip#head"
set "DEV01_P4USER="
set "DEV01_WORKSPACE_ROOT="
set "DEV01_UGS_ROOT="
set "DEV01_NO_LAUNCH=0"
set "DEV01_NO_MINIMAL_SYNC=0"

:parse_args
if "%~1"=="" goto choose_user
if /I "%~1"=="--help" goto help
if /I "%~1"=="-h" goto help
if /I "%~1"=="--p4user" (
    if "%~2"=="" goto bad_args
    set "DEV01_P4USER=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--workspace" (
    if "%~2"=="" goto bad_args
    set "DEV01_WORKSPACE_ROOT=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--ugs-path" (
    if "%~2"=="" goto bad_args
    set "DEV01_UGS_ROOT=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--no-launch" (
    set "DEV01_NO_LAUNCH=1"
    shift
    goto parse_args
)
if /I "%~1"=="--no-minimal-sync" (
    set "DEV01_NO_MINIMAL_SYNC=1"
    shift
    goto parse_args
)
goto bad_args

:help
echo Dev01 newcomer setup
echo.
echo Usage:
echo   SetupDev01Newcomer.bat
echo   SetupDev01Newcomer.bat --p4user NAME --workspace "D:\Dev01" --ugs-path "D:\Tools\Dev01UGS"
echo.
echo Options:
echo   --p4user NAME        Personal Perforce account. Never use a shared Admin account.
echo   --workspace PATH     Local Dev01 project root. Do not choose a drive root.
echo   --ugs-path PATH      UnrealGameSync installation directory.
echo   --no-launch          Install and configure without starting UGS.
echo   --no-minimal-sync    Do not sync the small project bootstrap files.
echo.
echo This script never stores a password. Perforce asks for it interactively.
exit /b 0

:bad_args
echo [Dev01] ERROR: Invalid or incomplete arguments.
echo [Dev01] Run SetupDev01Newcomer.bat --help for usage.
exit /b 1

:choose_user
if defined DEV01_P4USER goto choose_ugs
set "DEV01_P4USER=%P4USER%"
if not defined DEV01_P4USER set "DEV01_P4USER=%USERNAME%"
set "DEV01_USER_INPUT="
echo.
echo [Dev01] Enter your PERSONAL Perforce account.
echo [Dev01] Do not use the shared Admin account.
set /p "DEV01_USER_INPUT=Perforce user [%DEV01_P4USER%]: "
if defined DEV01_USER_INPUT set "DEV01_P4USER=%DEV01_USER_INPUT%"

:choose_ugs
if defined DEV01_UGS_ROOT goto choose_workspace
set "DEV01_DEFAULT_UGS=%LOCALAPPDATA%\Dev01\UnrealGameSync"
for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -Command ^
  "Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.FolderBrowserDialog; $d.Description='Choose the Dev01 UnrealGameSync installation folder'; $d.ShowNewFolderButton=$true; $candidate=$env:DEV01_DEFAULT_UGS; while($candidate -and -not (Test-Path -LiteralPath $candidate)){ $candidate=Split-Path -Parent $candidate }; if($candidate){$d.SelectedPath=$candidate}; if($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){[Console]::Write($d.SelectedPath)}"`) do set "DEV01_UGS_ROOT=%%I"
if not defined DEV01_UGS_ROOT goto cancelled

:choose_workspace
if defined DEV01_WORKSPACE_ROOT goto validate_paths
if exist "D:\" (
    set "DEV01_DEFAULT_WORKSPACE=D:\Dev01"
) else (
    set "DEV01_DEFAULT_WORKSPACE=%USERPROFILE%\Dev01"
)
for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -Command ^
  "Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.FolderBrowserDialog; $d.Description='Choose the Dev01 project workspace folder'; $d.ShowNewFolderButton=$true; $candidate=$env:DEV01_DEFAULT_WORKSPACE; while($candidate -and -not (Test-Path -LiteralPath $candidate)){ $candidate=Split-Path -Parent $candidate }; if($candidate){$d.SelectedPath=$candidate}; if($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){[Console]::Write($d.SelectedPath)}"`) do set "DEV01_WORKSPACE_ROOT=%%I"
if not defined DEV01_WORKSPACE_ROOT goto cancelled

:validate_paths
for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; $ugs=[IO.Path]::GetFullPath($env:DEV01_UGS_ROOT).TrimEnd('\'); $ws=[IO.Path]::GetFullPath($env:DEV01_WORKSPACE_ROOT).TrimEnd('\'); if($ugs -match '^[A-Za-z]:$' -or $ws -match '^[A-Za-z]:$'){throw 'Do not use a drive root. Choose a subfolder such as D:\Dev01.'}; if($ugs -ieq $ws -or $ugs.StartsWith($ws+'\',[StringComparison]::OrdinalIgnoreCase) -or $ws.StartsWith($ugs+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'UGS and the project workspace must be separate folders.'}; [Console]::Write($ugs+'|'+$ws)"`) do set "DEV01_VALIDATED_PATHS=%%I"
if errorlevel 1 (
    echo [Dev01] ERROR: The selected folders are not safe.
    echo [Dev01] Use separate subfolders, for example D:\Tools\Dev01UGS and D:\Dev01.
    exit /b 2
)
for /f "tokens=1,2 delims=|" %%I in ("%DEV01_VALIDATED_PATHS%") do (
    set "DEV01_UGS_ROOT=%%I"
    set "DEV01_WORKSPACE_ROOT=%%J"
)

where p4.exe >nul 2>nul
if errorlevel 1 (
    echo [Dev01] ERROR: p4.exe was not found in PATH.
    echo [Dev01] Install Helix Visual Client with command-line tools, then rerun this script.
    exit /b 3
)

if not exist "%DEV01_UGS_ROOT%" mkdir "%DEV01_UGS_ROOT%"
if errorlevel 1 goto create_failed
if not exist "%DEV01_WORKSPACE_ROOT%" mkdir "%DEV01_WORKSPACE_ROOT%"
if errorlevel 1 goto create_failed

echo.
echo [Dev01] Server:        %DEV01_P4PORT%
echo [Dev01] User:          %DEV01_P4USER%
echo [Dev01] Project root:  %DEV01_WORKSPACE_ROOT%
echo [Dev01] UGS folder:    %DEV01_UGS_ROOT%
echo.

p4.exe -p "%DEV01_P4PORT%" trust -l >nul 2>nul
if errorlevel 1 (
    echo [Dev01] Trusting the verified Dev01 SSL server fingerprint...
    p4.exe -p "%DEV01_P4PORT%" trust -y
    if errorlevel 1 (
        echo [Dev01] ERROR: Unable to trust %DEV01_P4PORT%.
        exit /b 4
    )
)

p4.exe -p "%DEV01_P4PORT%" -u "%DEV01_P4USER%" login -s >nul 2>nul
if errorlevel 1 (
    echo [Dev01] Perforce login is required. Enter the password for %DEV01_P4USER%.
    echo [Dev01] Password characters are hidden; the blank input line is normal.
    p4.exe -p "%DEV01_P4PORT%" -u "%DEV01_P4USER%" login -a
    if errorlevel 1 (
        echo [Dev01] ERROR: Perforce login failed.
        exit /b 5
    )
)

for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$u=$env:DEV01_P4USER -replace '[^A-Za-z0-9_.-]','_'; $h=$env:COMPUTERNAME -replace '[^A-Za-z0-9_.-]','_'; [Console]::Write(('dev01_{0}_{1}_main' -f $u,$h).ToLowerInvariant())"`) do set "DEV01_CLIENT=%%I"
if not defined DEV01_CLIENT (
    echo [Dev01] ERROR: Unable to create the workspace name.
    exit /b 6
)

set "DEV01_CLIENT_SPEC=%TEMP%\Dev01-client-%RANDOM%-%RANDOM%.txt"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$lines=@('Client: '+$env:DEV01_CLIENT,'Owner: '+$env:DEV01_P4USER,'Host: '+$env:COMPUTERNAME,'Description:','`tPersonal Dev01 UGS workspace created by SetupDev01Newcomer.bat.','Root: '+$env:DEV01_WORKSPACE_ROOT,'Options: noallwrite noclobber nocompress unlocked nomodtime normdir','SubmitOptions: submitunchanged','LineEnd: local','Stream: '+$env:DEV01_STREAM); Set-Content -LiteralPath $env:DEV01_CLIENT_SPEC -Value $lines -Encoding ascii"
if errorlevel 1 (
    echo [Dev01] ERROR: Unable to prepare the workspace specification.
    exit /b 6
)
p4.exe -p "%DEV01_P4PORT%" -u "%DEV01_P4USER%" client -i < "%DEV01_CLIENT_SPEC%"
set "DEV01_CLIENT_EXIT=%ERRORLEVEL%"
del /q "%DEV01_CLIENT_SPEC%" >nul 2>nul
if not "%DEV01_CLIENT_EXIT%"=="0" (
    echo [Dev01] ERROR: Unable to create or update workspace %DEV01_CLIENT%.
    exit /b 6
)

>"%DEV01_WORKSPACE_ROOT%\.p4config" (
    echo P4PORT=%DEV01_P4PORT%
    echo P4USER=%DEV01_P4USER%
    echo P4CLIENT=%DEV01_CLIENT%
    echo P4IGNORE=.p4ignore
)

set "DEV01_UGS_ZIP=%TEMP%\Dev01-UnrealGameSync-%RANDOM%-%RANDOM%.zip"
echo [Dev01] Downloading the verified UnrealGameSync release...
p4.exe -p "%DEV01_P4PORT%" -u "%DEV01_P4USER%" print -q -o "%DEV01_UGS_ZIP%" "%DEV01_UGS_DEPOT%"
if errorlevel 1 (
    echo [Dev01] ERROR: Unable to download %DEV01_UGS_DEPOT%.
    del /q "%DEV01_UGS_ZIP%" >nul 2>nul
    exit /b 7
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; $stage=Join-Path $env:TEMP ('Dev01UGS-'+[guid]::NewGuid().ToString('N')); try { [void](New-Item -ItemType Directory -Force -Path $stage); Expand-Archive -LiteralPath $env:DEV01_UGS_ZIP -DestinationPath $stage -Force; $exe=Get-ChildItem -LiteralPath $stage -Filter UnrealGameSync.exe -Recurse | Select-Object -First 1; if(-not $exe){throw 'UnrealGameSync.exe is missing from the release archive'}; $source=$exe.Directory.FullName; [void](New-Item -ItemType Directory -Force -Path $env:DEV01_UGS_ROOT); Get-ChildItem -LiteralPath $source -Force | Copy-Item -Destination $env:DEV01_UGS_ROOT -Recurse -Force } finally { Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue }"
set "DEV01_INSTALL_EXIT=%ERRORLEVEL%"
del /q "%DEV01_UGS_ZIP%" >nul 2>nul
if not "%DEV01_INSTALL_EXIT%"=="0" (
    echo [Dev01] ERROR: Unable to install UnrealGameSync.
    exit /b 8
)

>"%DEV01_UGS_ROOT%\StartDev01UGS.cmd" (
    echo @echo off
    echo setlocal
    echo set "UNREALGAMESYNC_SYNC_FOLDER=%%~dp0Latest"
    echo start "Dev01 UnrealGameSync" /D "%%~dp0" "%%~dp0UnrealGameSync.exe"
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$root=[IO.Path]::GetFullPath($env:DEV01_UGS_ROOT).TrimEnd('\'); [Environment]::SetEnvironmentVariable('UNREALGAMESYNC_SYNC_FOLDER',(Join-Path $root 'Latest'),'User'); $state=Join-Path $env:LOCALAPPDATA 'Dev01'; [void](New-Item -ItemType Directory -Force -Path $state); Set-Content -LiteralPath (Join-Path $state 'UGSInstallPath.txt') -Value $root -Encoding ascii"

if "%DEV01_NO_MINIMAL_SYNC%"=="1" goto finish
echo [Dev01] Syncing the small project bootstrap files...
p4.exe -p "%DEV01_P4PORT%" -u "%DEV01_P4USER%" -c "%DEV01_CLIENT%" sync -q ^
  "%DEV01_STREAM%/DevKit.uproject#head" ^
  "%DEV01_STREAM%/Build/...#head" ^
  "%DEV01_STREAM%/StartDevKit.bat#head"
if errorlevel 1 (
    echo [Dev01] WARNING: UGS was installed, but the minimal project sync did not finish.
    echo [Dev01] Open UGS, select workspace %DEV01_CLIENT%, then use Sync Now.
)

:finish
echo.
echo [Dev01] Setup complete.
echo [Dev01] Workspace: %DEV01_CLIENT%
echo [Dev01] Project:   %DEV01_WORKSPACE_ROOT%
echo [Dev01] UGS:       %DEV01_UGS_ROOT%
echo [Dev01] First UGS project selection:
echo [Dev01]   Workspace: %DEV01_CLIENT%
echo [Dev01]   Path:      /DevKit.uproject
echo [Dev01] In UGS keep Build unchecked, then use Sync Now.

if "%DEV01_NO_LAUNCH%"=="1" exit /b 0
if not exist "%DEV01_UGS_ROOT%\StartDev01UGS.cmd" (
    echo [Dev01] ERROR: UGS launcher is missing.
    exit /b 9
)
start "Dev01 UnrealGameSync" "%DEV01_UGS_ROOT%\StartDev01UGS.cmd"
exit /b 0

:create_failed
echo [Dev01] ERROR: Unable to create one of the selected folders.
exit /b 2

:cancelled
echo [Dev01] Setup cancelled. No workspace was created.
exit /b 1
