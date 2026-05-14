@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%DevKit.uproject"
set "ENGINE_ROOT=D:\Epic Library\UE_5.4"

if not exist "%PROJECT_DIR%DevKit.sln" (
    echo [1/3] Generating VS solution...
    call "%ENGINE_ROOT%\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="%UPROJECT%" -game -engine
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
