@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT_FILE=%PROJECT_ROOT%DevKit.uproject"
set "EDITOR_EXE=%PROJECT_ROOT%Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%PROJECT_FILE%" (
    echo [Dev01] ERROR: Project file is missing:
    echo [Dev01]        %PROJECT_FILE%
    echo [Dev01] Run this script from the root of a complete Dev01 workspace.
    exit /b 2
)

if not exist "%EDITOR_EXE%" (
    echo [Dev01] ERROR: The source-free UE 5.8 editor is not installed in this workspace.
    echo [Dev01] Expected:
    echo [Dev01]        %EDITOR_EXE%
    echo [Dev01] Sync the Dev01 Engine package and the matching UGS precompiled binaries, then retry.
    echo [Dev01] Engine source code is intentionally not distributed through this workspace.
    exit /b 3
)

pushd "%PROJECT_ROOT%" >nul
"%EDITOR_EXE%" "%PROJECT_FILE%" %*
set "EDITOR_EXIT=%ERRORLEVEL%"
popd >nul

exit /b %EDITOR_EXIT%
