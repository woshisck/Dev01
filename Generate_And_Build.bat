@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%DevKit.uproject"
set "ENGINE_ROOT=D:\Dev02\UnrealEngine-5.8"
set "MCP_PORT=8765"
set "MCP_PATH=/mcp"
set "BUILD_ARGS=-WaitMutex -NoHotReload -MaxParallelActions=2"

if defined UE_MCP_PORT set "MCP_PORT=%UE_MCP_PORT%"
if defined UE_BUILD_ARGS set "BUILD_ARGS=%UE_BUILD_ARGS%"

if not exist "%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine not found: "%ENGINE_ROOT%"
    goto :fail
)
if not exist "%ENGINE_ROOT%\Engine\Source\Programs\UnrealBuildTool\UnrealBuildTool.csproj" (
    echo Unreal source engine is incomplete or UE_ENGINE_DIR points to the wrong folder:
    echo   "%ENGINE_ROOT%\Engine\Source\Programs\UnrealBuildTool\UnrealBuildTool.csproj"
    goto :fail
)

echo [1/3] Generating VS solution with "%ENGINE_ROOT%"...
call "%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat" -ProjectFiles -Project="%UPROJECT%" -Game -Engine
if errorlevel 1 goto :fail

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
