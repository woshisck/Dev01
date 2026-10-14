@echo off
setlocal
set "DEV01_PROJECT=%~dp0"
set "DEV01_APP=%~dp0BuildScripts\Workflow\workflow_app.py"
set "DEV01_EXE=%~dp0BuildScripts\Workflow\dist\Dev01Workflow.exe"
if exist "%DEV01_EXE%" (
    start "Dev01 Workflow" "%DEV01_EXE%" --project "%DEV01_PROJECT%."
    exit /b 0
)
if exist "C:\Python313\pythonw.exe" (
    start "Dev01 Workflow" "C:\Python313\pythonw.exe" "%DEV01_APP%" --project "%DEV01_PROJECT%."
    exit /b 0
)
where pythonw.exe >nul 2>nul
if not errorlevel 1 (
    start "Dev01 Workflow" pythonw.exe "%DEV01_APP%" --project "%DEV01_PROJECT%."
    exit /b 0
)
echo Python with Tkinter or the packaged Dev01Workflow.exe is required.
pause
exit /b 1
