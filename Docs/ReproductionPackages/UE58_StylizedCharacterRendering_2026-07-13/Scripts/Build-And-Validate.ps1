[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,

    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [switch]$RunSetup,
    [int]$EditorValidationTimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot)
$ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot)
$ProjectFile = Join-Path $ProjectRoot 'DevKit.uproject'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$GenerateBat = Join-Path $EngineRoot 'GenerateProjectFiles.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$LogRoot = Join-Path $ProjectRoot 'Saved\Logs'

New-Item -ItemType Directory -Path $LogRoot -Force | Out-Null

if ($RunSetup) {
    & (Join-Path $EngineRoot 'Setup.bat')
    if ($LASTEXITCODE -ne 0) { throw 'Setup.bat failed.' }
}

& $GenerateBat $ProjectFile -Game -Engine
if ($LASTEXITCODE -ne 0) { throw 'GenerateProjectFiles.bat failed.' }

& $BuildBat DevKitEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'DevKitEditor build failed.' }

$MaterialLog = Join-Path $LogRoot 'StylizedCharacterMaterialSetup-Reproduction.log'
& $EditorCmd $ProjectFile -run=StylizedCharacterMaterialSetup -Apply -unattended -nop4 -nosplash -NullRHI -NoSound "-abslog=$MaterialLog"
if ($LASTEXITCODE -ne 0) { throw 'StylizedCharacterMaterialSetup commandlet failed.' }

$MaterialText = Get-Content -LiteralPath $MaterialLog -Raw
if ($MaterialText -notmatch 'Success - 0 error\(s\)') {
    throw "Material commandlet did not report zero errors. See $MaterialLog"
}

$ShaderLog = Join-Path $LogRoot 'StylizedCharacterShaderValidation-Reproduction.log'
$ArgumentLine = '"' + $ProjectFile + '" -d3d12 -unattended -nop4 -nosplash -NoSound -abslog="' + $ShaderLog + '" -ExecCmds="RecompileShaders Changed,Quit"'
$Process = Start-Process -FilePath $Editor -ArgumentList $ArgumentLine -PassThru -WindowStyle Hidden
$Exited = $Process.WaitForExit($EditorValidationTimeoutSeconds * 1000)

$Deadline = (Get-Date).AddSeconds(30)
while (-not (Test-Path -LiteralPath $ShaderLog -PathType Leaf) -and (Get-Date) -lt $Deadline) {
    Start-Sleep -Milliseconds 250
}

$ShaderText = if (Test-Path -LiteralPath $ShaderLog) { Get-Content -LiteralPath $ShaderLog -Raw } else { '' }
$HasCommand = $ShaderText -match 'Cmd: RecompileShaders Changed'
$HasResult = $ShaderText -match 'No Shader changes found|Shaders left to compile 0'
$HasShaderError = $ShaderText -match 'Shader compiler errors|errors compiling global shaders|Fatal error|Ensure condition failed'

if (-not $Exited -and -not $Process.HasExited) {
    Stop-Process -Id $Process.Id -Force
}

if (-not $HasCommand -or -not $HasResult -or $HasShaderError) {
    throw "D3D12 Shader validation failed. See $ShaderLog"
}

Write-Host 'Build, material generation, and D3D12 Shader validation succeeded.'
Write-Host "Material log: $MaterialLog"
Write-Host "Shader log: $ShaderLog"
