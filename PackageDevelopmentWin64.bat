@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%BuildScripts\PackageDevelopmentWin64.ps1"

if not exist "%PS_SCRIPT%" (
    echo ERROR: Could not find "%PS_SCRIPT%".
    pause
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo Packaging failed with exit code %EXIT_CODE%.
    pause
)

exit /b %EXIT_CODE%
