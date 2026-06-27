@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%DevKit.uproject"
set "ENGINE_ROOT="
set "MCP_PORT=8765"
set "MCP_PATH=/mcp"
set "BUILD_ARGS=-WaitMutex -MaxParallelActions=4"

if defined UE_ENGINE_DIR (
    if exist "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" set "ENGINE_ROOT=%UE_ENGINE_DIR%"
    if not defined ENGINE_ROOT if exist "%UE_ENGINE_DIR%\Build\BatchFiles\Build.bat" set "ENGINE_ROOT=%UE_ENGINE_DIR%\.."
)
if not defined ENGINE_ROOT if exist "D:\Code\UE_Engine\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "ENGINE_ROOT=D:\Code\UE_Engine\UE_5.8"
if not defined ENGINE_ROOT if exist "D:\Epic Library\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "ENGINE_ROOT=D:\Epic Library\UE_5.8"
if defined UE_MCP_PORT set "MCP_PORT=%UE_MCP_PORT%"
if defined UE_BUILD_ARGS set "BUILD_ARGS=%UE_BUILD_ARGS%"

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
call "%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat" DevKitEditor Win64 Development -Project="%UPROJECT%" %BUILD_ARGS%
if errorlevel 1 goto :fail

echo [3/3] Opening editor with Unreal MCP at http://127.0.0.1:%MCP_PORT%%MCP_PATH% ...
start "" "%ENGINE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%UPROJECT%" -ModelContextProtocolPort=%MCP_PORT% -ModelContextProtocolStartServer
goto :end

:fail
echo Build failed.

:end
pause
