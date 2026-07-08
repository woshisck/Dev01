param(
    [switch]$Apply
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

$pathsToRemove = @(
    "Content\Blood_VFX_Pack",
    "Content\Collections",
    "Content\EssentialGreatSwordAnimationPack",
    "Content\External",
    "Content\Free_Magic",
    "Content\Lava_FX",
    "Content\MassiveGreatSwordAnimset",
    "Content\PoisonFX",
    "Content\SlashTrailElemental",
    "Content\_LevelUpSpawn"
)

Write-Host "Project root: $projectRoot"
Write-Host ""

$existingTargets = @()

foreach ($relativePath in $pathsToRemove) {
    $candidate = Join-Path $projectRoot $relativePath

    if (-not (Test-Path -LiteralPath $candidate)) {
        Write-Host "[Skip] Missing: $relativePath"
        continue
    }

    $resolved = (Resolve-Path -LiteralPath $candidate).Path
    $fullRoot = [System.IO.Path]::GetFullPath($projectRoot)
    $fullTarget = [System.IO.Path]::GetFullPath($resolved)

    if (-not $fullTarget.StartsWith($fullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to touch a path outside the project: $fullTarget"
    }

    $existingTargets += [PSCustomObject]@{
        RelativePath = $relativePath
        FullPath = $fullTarget
    }
}

if ($existingTargets.Count -eq 0) {
    Write-Host ""
    Write-Host "No local-only content folders found."
    exit 0
}

Write-Host ""
Write-Host "Local-only content folders:"
foreach ($target in $existingTargets) {
    Write-Host "  $($target.RelativePath)"
}

if (-not $Apply) {
    Write-Host ""
    Write-Host "Dry run only. Re-run with -Apply to delete these folders."
    exit 0
}

Write-Host ""
foreach ($target in $existingTargets) {
    Write-Host "[Delete] $($target.RelativePath)"
    Remove-Item -LiteralPath $target.FullPath -Recurse -Force
}

Write-Host ""
Write-Host "Cleanup complete."
