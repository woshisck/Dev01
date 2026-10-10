@echo off
setlocal EnableDelayedExpansion

rem Launch the DevKit editor with the MCP server on a known port, then block until
rem the port is actually LISTENING so you know the server really came up.
rem
rem Prefer starting the editor BEFORE Claude Code. Claude Code caches the MCP
rem session id, so the first /mcp after an editor restart usually fails; a
rem second /mcp re-handshakes and succeeds.
rem
rem Must use the custom G: engine: the CelesLight plugin needs engine-side
rem EStylizedCharacterLightMode, which stock UE_5.8 installs do not have.

set "ENGINE=G:\GitHub\Dev02\UnrealEngine-5.8"
set "PROJECT=%~dp0DevKit.uproject"
rem Must stay above the TCP ephemeral range (1024-15000 on this machine, per
rem "netsh int ipv4 show dynamicport tcp"). 8765 was inside it and svchost.exe
rem held it permanently, so the MCP listener could never bind.
set "MCP_PORT=18765"
set "EDITOR=%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe"

echo.
echo  Engine : %ENGINE%
echo  Project: %PROJECT%
echo  MCP    : http://localhost:%MCP_PORT%/mcp
echo.

if not exist "%EDITOR%" (
    echo  [X] Editor not found: %EDITOR%
    exit /b 1
)
if not exist "%PROJECT%" (
    echo  [X] Project not found: %PROJECT%
    exit /b 1
)

rem An already-running editor holds the MCP port and keeps stale module DLLs in
rem memory. Refuse rather than kill it, since it may have unsaved work.
tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo  [X] UnrealEditor.exe is already running.
    echo      Close it first - it holds port %MCP_PORT% and stale module DLLs.
    echo      Nothing was launched.
    exit /b 1
)

rem Fail early if something else squats the port, otherwise the server logs
rem "Starting MCP server" and then silently never binds.
netstat -ano | findstr /C:":%MCP_PORT% " | findstr /C:"LISTENING" >nul
if not errorlevel 1 (
    echo  [X] Port %MCP_PORT% is already LISTENING - another process owns it.
    netstat -ano | findstr /C:":%MCP_PORT% " | findstr /C:"LISTENING"
    exit /b 1
)

echo  [*] Launching editor...
start "" "%EDITOR%" "%PROJECT%" -ModelContextProtocolPort=%MCP_PORT%

echo  [*] Waiting for MCP to bind (up to 5 min; first load is slow)...
for /L %%i in (1,1,100) do (
    netstat -ano | findstr /C:":%MCP_PORT% " | findstr /C:"LISTENING" >nul
    if not errorlevel 1 goto :listening

    tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>nul | find /I "UnrealEditor.exe" >nul
    if errorlevel 1 (
        echo  [X] Editor exited before MCP bound. Check Saved\Logs\DevKit.log
        exit /b 1
    )
    timeout /t 3 /nobreak >nul
)

echo  [X] MCP never bound to port %MCP_PORT%.
echo      Check Saved\Logs\DevKit.log for LogModelContextProtocol lines.
exit /b 1

:listening
echo.
echo  [OK] MCP is LISTENING on port %MCP_PORT%.
echo.
echo       Claude Code NOT running yet -^> just start it, it connects on launch.
echo       Claude Code ALREADY running  -^> run /mcp, and if it reports
echo         "Failed to reconnect to unreal", simply run /mcp AGAIN. The first
echo         attempt tends to replay the session id from the previous editor,
echo         which this server rejects with a 404; the retry re-handshakes.
echo         Only restart Claude Code if several retries in a row fail.
echo.
exit /b 0
