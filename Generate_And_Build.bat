@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%DevKit.uproject"
set "ENGINE_ROOT=Z:\GZA_Software\RealityCapture\UE_5.8"

if not exist "%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine not found: "%ENGINE_ROOT%"
    goto :fail
)

if not exist "%PROJECT_DIR%DevKit.sln" (
    echo [1/3] Generating VS solution...
    call "%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat" -ProjectFiles -Project="%UPROJECT%" -Game -Engine
    if errorlevel 1 goto :fail
)

echo [2/3] Compiling DevKitEditor...
call "%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat" DevKitEditor Win64 Development -Project="%UPROJECT%" -WaitMutex
if errorlevel 1 goto :fail

echo [3/3] Opening editor...
start "" "%ENGINE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%UPROJECT%"
goto :end

:fail
echo Build failed.

:end
pause
