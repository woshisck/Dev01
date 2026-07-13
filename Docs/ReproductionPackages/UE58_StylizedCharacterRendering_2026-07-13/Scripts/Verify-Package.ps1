[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$PackageRoot = Split-Path -Parent $PSScriptRoot
$ManifestPath = Join-Path $PackageRoot 'FILE_MANIFEST.csv'

if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw "Manifest not found: $ManifestPath"
}

$Failures = [System.Collections.Generic.List[string]]::new()
foreach ($Entry in Import-Csv -LiteralPath $ManifestPath) {
    if ($Entry.Root -notin @('Engine', 'Project')) {
        $Failures.Add("Invalid root: $($Entry.Root)")
        continue
    }

    $RelativeFile = $Entry.PackagePath.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $FilePath = Join-Path $PackageRoot $RelativeFile
    if (-not (Test-Path -LiteralPath $FilePath -PathType Leaf)) {
        $Failures.Add("Missing: $($Entry.PackagePath)")
        continue
    }

    $ActualHash = (Get-FileHash -LiteralPath $FilePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($ActualHash -ne $Entry.SHA256.ToLowerInvariant()) {
        $Failures.Add("Hash mismatch: $($Entry.PackagePath)")
    }
}

if ($Failures.Count -gt 0) {
    $Failures | ForEach-Object { Write-Error $_ }
    throw "Package verification failed with $($Failures.Count) problem(s)."
}

Write-Host "Package verification succeeded: $((Import-Csv -LiteralPath $ManifestPath).Count) file(s)."
