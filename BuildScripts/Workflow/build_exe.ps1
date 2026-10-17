$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    python -m PyInstaller --noconfirm --onefile --windowed --name Dev01Workflow --distpath dist --workpath build --specpath build workflow_app.py
    if ($LASTEXITCODE -ne 0) { throw 'Dev01Workflow packaging failed.' }
    Get-Item -LiteralPath (Join-Path $PSScriptRoot 'dist\Dev01Workflow.exe') | Select-Object FullName,Length,LastWriteTime
} finally { Pop-Location }
