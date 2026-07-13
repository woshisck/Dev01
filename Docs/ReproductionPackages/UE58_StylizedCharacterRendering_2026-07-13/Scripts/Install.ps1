[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,

    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [switch]$SkipEngineFiles,
    [switch]$SkipProjectFiles,
    [switch]$AllowDifferentEngineCommit
)

$ErrorActionPreference = 'Stop'
$ExpectedEngineCommit = '6673776aad735f49a5ce3bbed474ffcc701e7a8e'
$PackageRoot = Split-Path -Parent $PSScriptRoot
$ManifestPath = Join-Path $PackageRoot 'FILE_MANIFEST.csv'

& (Join-Path $PSScriptRoot 'Verify-Package.ps1')

$EngineRoot = [IO.Path]::GetFullPath($EngineRoot)
$ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot)

if (-not (Test-Path -LiteralPath (Join-Path $EngineRoot 'Engine') -PathType Container)) {
    throw "Engine root is invalid: $EngineRoot"
}
if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot 'DevKit.uproject') -PathType Leaf)) {
    throw "Project root is invalid: $ProjectRoot"
}

if (-not $SkipEngineFiles) {
    $ActualCommit = (& git -C $EngineRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to read Engine git commit from $EngineRoot"
    }
    if (-not $AllowDifferentEngineCommit -and $ActualCommit -ne $ExpectedEngineCommit) {
        throw "Engine commit is $ActualCommit; expected $ExpectedEngineCommit. Use -AllowDifferentEngineCommit only after reviewing merge risk."
    }
}

$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$BackupRoot = Join-Path $ProjectRoot "Saved\StylizedCharacterRenderingBackups\$Timestamp"
$Installed = 0

foreach ($Entry in Import-Csv -LiteralPath $ManifestPath) {
    if ($Entry.Root -notin @('Engine', 'Project')) {
        throw "Unsupported manifest root: $($Entry.Root)"
    }

    if ($Entry.Root -eq 'Engine' -and $SkipEngineFiles) { continue }
    if ($Entry.Root -eq 'Project' -and $SkipProjectFiles) { continue }

    $TargetRoot = if ($Entry.Root -eq 'Engine') { $EngineRoot } else { $ProjectRoot }
    $RelativeTarget = $Entry.RelativePath.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $SourcePath = Join-Path $PackageRoot ($Entry.PackagePath.Replace('/', [IO.Path]::DirectorySeparatorChar))
    $TargetPath = Join-Path $TargetRoot $RelativeTarget

    if (Test-Path -LiteralPath $TargetPath -PathType Leaf) {
        $BackupPath = Join-Path $BackupRoot (Join-Path $Entry.Root $RelativeTarget)
        $BackupDirectory = Split-Path -Parent $BackupPath
        New-Item -ItemType Directory -Path $BackupDirectory -Force | Out-Null
        Copy-Item -LiteralPath $TargetPath -Destination $BackupPath -Force
    }

    $TargetDirectory = Split-Path -Parent $TargetPath
    New-Item -ItemType Directory -Path $TargetDirectory -Force | Out-Null
    Copy-Item -LiteralPath $SourcePath -Destination $TargetPath -Force
    $Installed++
}

Write-Host "Installed $Installed file(s)."
Write-Host "Backup root: $BackupRoot"
