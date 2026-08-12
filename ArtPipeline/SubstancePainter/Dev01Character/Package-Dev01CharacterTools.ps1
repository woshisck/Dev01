[CmdletBinding()]
param(
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $PSScriptRoot 'Dist\Dev01CharacterPainterTools.zip'
}

$PackageFiles = @(
    (Join-Path $PSScriptRoot 'Dev01_StylizedCharacter.glsl'),
    (Join-Path $PSScriptRoot 'dev01_character_tools.py'),
    (Join-Path $PSScriptRoot 'Install-Dev01CharacterTools.ps1'),
    (Join-Path $PSScriptRoot 'README.md')
)

foreach ($PackageFile in $PackageFiles) {
    if (-not (Test-Path -LiteralPath $PackageFile -PathType Leaf)) {
        throw "Missing package file: $PackageFile"
    }
}

$ResolvedOutput = [System.IO.Path]::GetFullPath($OutputPath)
$OutputDirectory = Split-Path -Parent $ResolvedOutput
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

Compress-Archive -LiteralPath $PackageFiles -DestinationPath $ResolvedOutput -CompressionLevel Optimal -Force

$ArchiveHash = (Get-FileHash -LiteralPath $ResolvedOutput -Algorithm SHA256).Hash
Write-Output 'DEV01_SP_PACKAGE_OK=1'
Write-Output "PACKAGE=$ResolvedOutput"
Write-Output "PACKAGE_SHA256=$ArchiveHash"
