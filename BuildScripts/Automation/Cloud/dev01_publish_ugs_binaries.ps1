param(
  [string]$ProjectRoot = 'C:\Project\Dev01-P4',
  [string]$ReleaseRoot = 'C:\BuildAgent\Dev01\release',
  [string]$ReleaseClient = 'Dev01BuildAgentRelease',
  [string]$ArchiveDepotPath = '//Dev01Binaries/UGS/++Dev01+main-Editor.zip',
  [int]$ProjectChange = 0,
  [int]$CodeChange = 0,
  [string]$BuildReceiptPath = 'C:\BuildAgent\Dev01\state\completed_build.json',
  [IO.FileStream]$PipelineLease,
  [switch]$Submit
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'dev01_release_contract.ps1')
$lease = Enter-Dev01PipelineLease $PipelineLease
try {
$env:Path = 'C:\Program Files\Perforce;' + $env:Path
$env:P4PORT = 'ssl:localhost:1666'
$env:P4USER = 'Dev01BuildAgent'
$env:P4CLIENT = $ReleaseClient
$env:P4TICKETS = 'C:\BuildAgent\Dev01\secrets\Dev01BuildAgent.ticket'
$env:P4TRUST = 'C:\BuildAgent\Dev01\secrets\p4trust.txt'

$projectTarget = Join-Path $ProjectRoot 'Binaries\Win64\DevKitEditor.target'
if (-not (Test-Path $projectTarget)) {
  throw "Expected build output is missing: $projectTarget"
}

function Get-ManifestBuildId([string]$Path) {
  if (-not (Test-Path -LiteralPath $Path)) {
    throw "Required module manifest missing: $Path"
  }
  $manifest = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
  if ([string]::IsNullOrWhiteSpace([string]$manifest.BuildId)) {
    throw "Module manifest has no BuildId: $Path"
  }
  return [string]$manifest.BuildId
}

$engineManifest = Join-Path $ProjectRoot 'Engine\Binaries\Win64\UnrealEditor.modules'
$engineBuildId = Get-ManifestBuildId $engineManifest
$targetJson = Get-Content -LiteralPath $projectTarget -Raw | ConvertFrom-Json
if ([string]$targetJson.Version.BuildId -ne $engineBuildId) {
  throw "Refusing to publish mismatched target BuildId $($targetJson.Version.BuildId); Engine has $engineBuildId"
}

if (-not (Test-Path -LiteralPath $BuildReceiptPath)) { throw 'No completed build receipt. Run the updated build script first.' }
$receipt = Get-Content -LiteralPath $BuildReceiptPath -Raw | ConvertFrom-Json
$receiptHash = (Get-FileHash -LiteralPath $BuildReceiptPath -Algorithm SHA256).Hash
$inventory = @(Get-Dev01PcbInventory $ProjectRoot)
$workspaceStamp = Get-Dev01WorkspaceStamp 'build_10_0_0_10_Dev01_main'
Assert-Dev01BuildReceipt $receipt $ProjectChange $CodeChange $engineBuildId $workspaceStamp $inventory
$projectChange = [int]$receipt.project_cl
$codeChange = [int]$receipt.code_cl
Assert-Dev01WorkspaceInputs 'build_10_0_0_10_Dev01_main' $projectChange ([int]$receipt.engine_cl)

$ugsIni = & p4 -c build_10_0_0_10_Dev01_main print -q "//Dev01/main/Build/UnrealGameSync.ini@$projectChange"
if ($LASTEXITCODE -ne 0 -or ($ugsIni -join "`n") -notmatch [regex]::Escape("ZippedBinariesPath=$ArchiveDepotPath")) {
  throw 'UGS config does not point to the expected zipped binaries depot path.'
}

$stageRoot = Join-Path $ReleaseRoot 'stage_ugs_editor'
$zipLocalPath = Join-Path $ReleaseRoot 'UGS\++Dev01+main-Editor.zip'
$candidateZip = Join-Path $ReleaseRoot 'candidate_ugs_editor.zip'
# Validate the exact recursive-delete target before removing only our staging payload.
$resolvedReleaseRoot = [IO.Path]::GetFullPath($ReleaseRoot).TrimEnd('\') + '\'
$stageRoot = [IO.Path]::GetFullPath($stageRoot)
if (-not $stageRoot.StartsWith($resolvedReleaseRoot, [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetFileName($stageRoot) -ne 'stage_ugs_editor') { throw 'Unsafe staging directory.' }
if (Test-Path $stageRoot) {
  Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $zipLocalPath) | Out-Null

$binaryDirs = New-Object System.Collections.Generic.List[string]
$projectBin = Join-Path $ProjectRoot 'Binaries\Win64'
if (Test-Path $projectBin) {
  $binaryDirs.Add($projectBin)
}

$pluginsRoot = Join-Path $ProjectRoot 'Plugins'
if (Test-Path $pluginsRoot) {
  # GameFeature plugins are commonly nested (for example Plugins\GameFeatures\CountDownTime),
  # so scan recursively instead of only considering direct children of Plugins.
  Get-ChildItem -Path $pluginsRoot -Directory -Recurse | ForEach-Object {
    $pluginBin = Join-Path $_.FullName 'Binaries\Win64'
    if (Test-Path $pluginBin) {
      $binaryDirs.Add($pluginBin)
    }
  }
}

if ($binaryDirs.Count -eq 0) {
  throw 'No project or plugin Binaries\Win64 folders were found to archive.'
}

$moduleManifests = @(
  foreach ($dir in $binaryDirs) {
    Get-ChildItem -LiteralPath $dir -Filter '*.modules' -File -ErrorAction SilentlyContinue
  }
)
if ($moduleManifests.Count -eq 0) {
  throw 'No project or plugin module manifests were found to archive.'
}
foreach ($manifestFile in $moduleManifests) {
  $moduleBuildId = Get-ManifestBuildId $manifestFile.FullName
  if ($moduleBuildId -ne $engineBuildId) {
    throw "Refusing to publish BuildId mismatch: $($manifestFile.FullName) has $moduleBuildId; Engine has $engineBuildId"
  }
}
Write-Output "PUBLISH_BUILD_ID_VALIDATED=$engineBuildId"

foreach ($dir in $binaryDirs) {
  $projectUri = New-Object System.Uri (($ProjectRoot.TrimEnd('\') + '\'))
  $dirUri = New-Object System.Uri (($dir.TrimEnd('\') + '\'))
  $relativeDir = [System.Uri]::UnescapeDataString($projectUri.MakeRelativeUri($dirUri).ToString()).Replace('/', '\').TrimEnd('\')
  $destDir = Join-Path $stageRoot $relativeDir
  New-Item -ItemType Directory -Force -Path $destDir | Out-Null
  Copy-Item -Path (Join-Path $dir '*') -Destination $destDir -Recurse -Force
}

# Engine files are distributed exclusively by the read-only //Dev01Engine import.
# Do not duplicate them in the PCB archive: UGS extracts PCBs after syncing the
# stream and cannot overwrite read-only imported Engine files.

Assert-Dev01BuildReceipt $receipt $projectChange $codeChange $engineBuildId $workspaceStamp @(Get-Dev01PcbInventory $stageRoot)
Compress-Archive -Path (Join-Path $stageRoot '*') -DestinationPath $candidateZip -CompressionLevel Optimal -Force
if (-not (Test-Path -LiteralPath $candidateZip)) { throw 'Archive was not created.' }
$candidateHash = (Get-FileHash -LiteralPath $candidateZip -Algorithm SHA256).Hash

# UGS parses the associated source changelist from this exact, zero-padded
# eight-digit prefix. Use the last visible code CL so content-only updates reuse
# the same PCB instead of deleting and redownloading an older archive.
$description = "[CL {0:D8}] Cloud-built DevKitEditor" -f $codeChange
if ($Submit) {
  # These gates run BEFORE touching the depot archive. Never revert another run's work.
  $currentStamp = Get-Dev01WorkspaceStamp 'build_10_0_0_10_Dev01_main'
  Assert-Dev01WorkspaceInputs 'build_10_0_0_10_Dev01_main' $projectChange ([int]$receipt.engine_cl)
  if ((Get-FileHash -LiteralPath $BuildReceiptPath -Algorithm SHA256).Hash -ne $receiptHash) { throw 'Build receipt was replaced during packaging.' }
  Assert-Dev01BuildReceipt $receipt $projectChange $codeChange $engineBuildId $currentStamp @(Get-Dev01PcbInventory $ProjectRoot)
  Assert-Dev01BuildReceipt $receipt $projectChange $codeChange $engineBuildId $currentStamp @(Get-Dev01PcbInventory $stageRoot)
  if ((Get-FileHash -LiteralPath $candidateZip -Algorithm SHA256).Hash -ne $candidateHash) { throw 'Candidate ZIP changed before publication.' }
  $opened = Invoke-Dev01P4Capture @('-ztag', 'opened', '-a', $ArchiveDepotPath)
  if (($opened.Lines -join "`n") -match 'depotFile') { throw "Archive is already opened: $($opened.Lines)" }
  if ($opened.ExitCode -ne 0 -and ($opened.Lines -join "`n") -notmatch 'not opened|no file') { throw "Cannot inspect archive state: $($opened.Lines)" }
  $where = @(& p4 -ztag where $ArchiveDepotPath)
  if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve release workspace archive path.' }
  $mapped = @($where | Where-Object { $_ -match '^\.\.\. path ' })
  if ($mapped.Count -ne 1 -or $mapped[0].Substring(9) -ine $zipLocalPath) { throw 'ReleaseRoot does not match the archive client mapping.' }
  & p4 sync $ArchiveDepotPath | Out-Host
  if ($LASTEXITCODE -ne 0) { throw 'Cannot sync existing archive; bootstrap publication requires explicit setup.' }
  & p4 edit $ArchiveDepotPath | Out-Host
  if ($LASTEXITCODE -ne 0) { throw 'Cannot open archive for edit.' }
  Copy-Item -LiteralPath $candidateZip -Destination $zipLocalPath -Force
  if ((Get-FileHash -LiteralPath $zipLocalPath -Algorithm SHA256).Hash -ne $candidateHash) { throw 'Staged ZIP copy failed verification; leaving opened file for inspection.' }
  $submitOutput = & p4 submit -d $description $ArchiveDepotPath
  $submitOutput | Out-Host
  if ($LASTEXITCODE -ne 0) {
    throw 'P4 submit failed.'
  }
} else {
  Write-Output "DRY_RUN_ARCHIVE_READY=$candidateZip"
  Write-Output "DRY_RUN_PROJECT_CL=$projectChange"
  Write-Output "DRY_RUN_CODE_CL=$codeChange"
  Write-Output 'Re-run with -Submit to submit the UGS archive.'
  exit 0
}

$verify = & p4 verify -q $ArchiveDepotPath
if ($LASTEXITCODE -ne 0) {
  throw "p4 verify failed for $ArchiveDepotPath"
}
if ($verify) {
  throw "p4 verify reported output for $ArchiveDepotPath`: $verify"
}

$stateDir = 'C:\BuildAgent\Dev01\state'
New-Item -ItemType Directory -Force -Path $stateDir | Out-Null
Set-Content -Path (Join-Path $stateDir 'last_project_archive_cl.txt') -Value $projectChange -Encoding ASCII
Set-Content -Path (Join-Path $stateDir 'last_code_archive_cl.txt') -Value $codeChange -Encoding ASCII

Write-Output "UGS_ARCHIVE_OK=$ArchiveDepotPath"
Write-Output "PROJECT_CL=$projectChange"
Write-Output "CODE_CL=$codeChange"
Write-Output "ZIP_LOCAL=$zipLocalPath"
} finally {
  if ($lease.Owned) { $lease.Stream.Dispose() }
}
