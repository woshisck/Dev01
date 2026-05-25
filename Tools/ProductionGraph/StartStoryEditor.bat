@echo off
setlocal EnableExtensions

set "PORT=4783"
set "URL=http://localhost:%PORT%/story.html"
set "TOOL_DIR=%~dp0"

pushd "%TOOL_DIR%\..\.." >nul 2>nul
if errorlevel 1 (
    echo Failed to locate project root.
    pause
    exit /b 1
)

where node >nul 2>nul
if errorlevel 1 (
    echo Node.js was not found. Please install Node.js or add it to PATH.
    pause
    popd >nul 2>nul
    exit /b 1
)

set "LISTENER="
for /f "tokens=5" %%P in ('netstat -ano ^| findstr /R /C:":%PORT% .*LISTENING"') do (
    set "LISTENER=%%P"
)

if not defined LISTENER (
    echo Starting ProductionGraph server on port %PORT%...
    start "ProductionGraph Server" /min node "Tools\ProductionGraph\server.js" --port %PORT%
    call :WaitForServer
) else (
    echo ProductionGraph server is already running on port %PORT%.
)

echo Opening %URL%
explorer.exe "%URL%" >nul 2>nul
popd >nul 2>nul
exit /b 0

:WaitForServer
for /l %%I in (1,1,20) do (
    set "READY="
    for /f "tokens=5" %%P in ('netstat -ano ^| findstr /R /C:":%PORT% .*LISTENING"') do (
        set "READY=%%P"
    )
    if defined READY exit /b 0
    timeout /t 1 /nobreak >nul
)
echo Warning: server did not report as listening yet. Opening the page anyway.
exit /b 0
