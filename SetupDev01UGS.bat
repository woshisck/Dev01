@echo off
setlocal EnableExtensions
pushd "%~dp0" >nul

set "DEV01_P4PORT=ssl:124.223.187.156:1666"
set "UGS_DEPOT=//Dev01Binaries/Tools/UnrealGameSync/Release.zip#head"
set "UGS_ZIP=%TEMP%\Dev01-UnrealGameSync-Release.zip"
set "UGS_ROOT=%LOCALAPPDATA%\Dev01\UnrealGameSync"

where p4.exe >nul 2>nul
if errorlevel 1 (
    echo [Dev01] ERROR: p4.exe was not found in PATH.
    exit /b 2
)

p4.exe -p "%DEV01_P4PORT%" login -s >nul 2>nul
if errorlevel 1 (
    echo [Dev01] P4 login is required for the user configured in this workspace.
    p4.exe -p "%DEV01_P4PORT%" login
    if errorlevel 1 exit /b 3
)

echo [Dev01] Downloading UnrealGameSync from P4...
p4.exe -p "%DEV01_P4PORT%" print -q -o "%UGS_ZIP%" "%UGS_DEPOT%"
if errorlevel 1 (
    echo [Dev01] ERROR: Unable to download %UGS_DEPOT%.
    exit /b 4
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; $dst=$env:UGS_ROOT; $stage=$dst+'.new'; $old=$dst+'.old'; Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue; [void](New-Item -ItemType Directory -Force -Path $stage); Expand-Archive -LiteralPath $env:UGS_ZIP -DestinationPath $stage -Force; if(-not (Test-Path (Join-Path $stage 'UnrealGameSync.exe'))){throw 'UnrealGameSync.exe is missing from Release.zip'}; Remove-Item -LiteralPath $old -Recurse -Force -ErrorAction SilentlyContinue; if(Test-Path -LiteralPath $dst){Move-Item -LiteralPath $dst -Destination $old}; Move-Item -LiteralPath $stage -Destination $dst"
if errorlevel 1 (
    echo [Dev01] ERROR: Unable to install UnrealGameSync.
    exit /b 5
)

echo [Dev01] UnrealGameSync installed to:
echo [Dev01]   %UGS_ROOT%
if /I "%~1"=="--no-launch" (
    popd >nul
    exit /b 0
)
start "Dev01 UnrealGameSync" "%UGS_ROOT%\UnrealGameSync.exe"
popd >nul
exit /b 0
