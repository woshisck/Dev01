# Shared environment for data editor scripts.
# Dot-source from other ps1: . "$PSScriptRoot/_common.ps1"

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path "$PSScriptRoot/../..").Path
$UProject    = Join-Path $ProjectRoot "DevKit.uproject"
$UEEditorCmd = "D:/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"

if (-not (Test-Path $UProject))   { throw "Project not found: $UProject" }
if (-not (Test-Path $UEEditorCmd)) { throw "Editor not found: $UEEditorCmd" }

function Get-RunningUnrealEditors {
    @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -like "UnrealEditor*" })
}

function Stop-EditorIfRunning {
    $procs = Get-RunningUnrealEditors
    if ($procs) {
        Write-Host "Stopping running UnrealEditor processes ..." -ForegroundColor Yellow
        $procs | Stop-Process -Force
        Start-Sleep -Seconds 2
    }
}

function Assert-NoEditorRunning {
    $procs = Get-RunningUnrealEditors
    if ($procs) {
        $summary = ($procs | ForEach-Object { "$($_.ProcessName):$($_.Id)" }) -join ", "
        throw "UnrealEditor is already running ($summary). Close it first, or rerun this wrapper with -CloseEditor to stop editor processes before launching headless Python."
    }
}

function Invoke-UEPython {
    param(
        [Parameter(Mandatory)]
        [string]$ScriptPath,

        [string[]]$ScriptArgs = @(),

        [switch]$CloseEditor,

        [Alias("KeepEditorOpen")]
        [switch]$AllowRunningEditor
    )

    if ($CloseEditor -and $AllowRunningEditor) {
        throw "Use only one of -CloseEditor or -AllowRunningEditor."
    }
    if ($CloseEditor) {
        Stop-EditorIfRunning
    }
    elseif (-not $AllowRunningEditor) {
        Assert-NoEditorRunning
    }

    if (-not (Test-Path $ScriptPath)) { throw "Script not found: $ScriptPath" }

    # Quote the script path; pass extra args after the script path.
    # UE's -ExecutePythonScript expects a single quoted string with the
    # script path followed by space-separated args (Python parses sys.argv).
    $argString = $ScriptPath
    foreach ($a in $ScriptArgs) {
        $argString += " $a"
    }
    $payload = "`"$argString`""

    Write-Host "Running UE Python: $argString" -ForegroundColor Cyan

    & $UEEditorCmd $UProject `
        "-ExecutePythonScript=$payload" `
        -unattended -nopause -nullrhi -nosplash `
        -log -stdout -FullStdOutLogOutput 2>&1 | ForEach-Object { Write-Host $_ }

    $code = $LASTEXITCODE
    $color = 'Red'
    if ($code -eq 0) { $color = 'Green' }
    Write-Host "UE exited with code: $code" -ForegroundColor $color
    return [int]$code
}
