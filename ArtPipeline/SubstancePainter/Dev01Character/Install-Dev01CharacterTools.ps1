[CmdletBinding()]
param(
    [string]$PainterDocumentsRoot = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Adobe\Adobe Substance 3D Painter')
)

$ErrorActionPreference = 'Stop'

$SourceRoot = $PSScriptRoot
$ShaderSource = Join-Path $SourceRoot 'Dev01_StylizedCharacter.glsl'
$PluginSource = Join-Path $SourceRoot 'dev01_character_tools.py'
$ShaderDestinationRoot = Join-Path $PainterDocumentsRoot 'assets\shaders'
$PluginDestinationRoot = Join-Path $PainterDocumentsRoot 'python\plugins'
$ShaderDestination = Join-Path $ShaderDestinationRoot 'Dev01_StylizedCharacter.glsl'
$PluginDestination = Join-Path $PluginDestinationRoot 'dev01_character_tools.py'

foreach ($SourceFile in @($ShaderSource, $PluginSource)) {
    if (-not (Test-Path -LiteralPath $SourceFile -PathType Leaf)) {
        throw "Missing source file: $SourceFile"
    }
}

New-Item -ItemType Directory -Path $ShaderDestinationRoot -Force | Out-Null
New-Item -ItemType Directory -Path $PluginDestinationRoot -Force | Out-Null
Copy-Item -LiteralPath $ShaderSource -Destination $ShaderDestination -Force
Copy-Item -LiteralPath $PluginSource -Destination $PluginDestination -Force

$ShaderHash = (Get-FileHash -LiteralPath $ShaderDestination -Algorithm SHA256).Hash
$PluginHash = (Get-FileHash -LiteralPath $PluginDestination -Algorithm SHA256).Hash

Write-Output "DEV01_SP_INSTALL_OK=1"
Write-Output "SHADER=$ShaderDestination"
Write-Output "SHADER_SHA256=$ShaderHash"
Write-Output "PLUGIN=$PluginDestination"
Write-Output "PLUGIN_SHA256=$PluginHash"
Write-Output 'Restart Painter, enable Python > dev01_character_tools once, select Shader Settings > Dev01_StylizedCharacter, then run Dev01 Character > Apply Neutral Character Preview Display.'
