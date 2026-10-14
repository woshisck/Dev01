param(
    [Parameter(Mandatory=$true)][string]$SourceProject,
    [Parameter(Mandatory=$true)][string]$Destination
)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path -LiteralPath $SourceProject).Path.TrimEnd('\')
$destinationRoot = [IO.Path]::GetFullPath($Destination).TrimEnd('\')
if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot 'DevKit.uproject'))) { throw 'Not a DevKit project.' }
if ($destinationRoot.StartsWith($sourceRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or $destinationRoot -eq $sourceRoot) { throw 'Development copy must be outside the main workspace.' }
if (Test-Path -LiteralPath $destinationRoot) { throw 'Destination already exists. This initial-copy tool never overwrites an existing project.' }
New-Item -ItemType Directory -Path $destinationRoot | Out-Null
foreach ($folder in @('Source','Shaders','Config','Build','Binaries','Intermediate','BuildScripts','Docs','Plugins')) {
    $inputPath = Join-Path $sourceRoot $folder
    if (-not (Test-Path -LiteralPath $inputPath)) { continue }
    & robocopy.exe $inputPath (Join-Path $destinationRoot $folder) /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /NFL /NDL /NJH /NJS /NP /XJ /XD Content .git Saved DerivedDataCache __pycache__ (Join-Path $sourceRoot 'BuildScripts\Workflow\dist') (Join-Path $sourceRoot 'BuildScripts\Workflow\build')
    if ($LASTEXITCODE -ge 8) { throw "Copy failed: $folder. Partial output is retained for inspection." }
}
foreach ($file in @('DevKit.uproject','Dev01_Workflow.bat','guide.md','AGENTS.md')) {
    Copy-Item -LiteralPath (Join-Path $sourceRoot $file) -Destination (Join-Path $destinationRoot $file)
}
$hostMarker = @{hostProject=$sourceRoot; contentPolicy='Shared asset junctions. Do not save assets during UI validation.'} | ConvertTo-Json
[IO.File]::WriteAllText((Join-Path $destinationRoot 'Dev01Development.json'), $hostMarker, [Text.UTF8Encoding]::new($false))
# Assets are shared for read-only UI testing. No Binaries or Intermediate junctions.
$contentPaths = @(Get-Item -LiteralPath (Join-Path $sourceRoot 'Content'))
$contentPaths += Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'Plugins') -Directory -Recurse |
    Where-Object { $_.Name -eq 'Content' -and $_.FullName -notmatch '\\(Saved|Intermediate|Binaries)\\' }
foreach ($contentPath in $contentPaths) {
    $relative = $contentPath.FullName.Substring($sourceRoot.Length + 1)
    $linkPath = Join-Path $destinationRoot $relative
    New-Item -ItemType Junction -Path $linkPath -Target $contentPath.FullName | Out-Null
}
Write-Output "DEVELOPMENT_PROJECT=$destinationRoot"
Write-Output 'Source/config/binaries are copied. Content junctions share original assets: do not save asset changes during UI tests.'
exit 0
