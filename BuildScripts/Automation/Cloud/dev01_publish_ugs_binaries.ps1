param(
  [string]$ProjectRoot = 'C:\Project\Dev01-P4',
  [string]$ReleaseRoot = 'C:\BuildAgent\Dev01\release',
  [string]$ReleaseClient = 'Dev01BuildAgentRelease',
  [string]$ArchiveDepotPath = '//Dev01Binaries/UGS/++Dev01+main-Editor.zip',
  [switch]$Submit
)

$ErrorActionPreference = 'Stop'
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

$changeLine = & p4 -c build_10_0_0_10_Dev01_main changes -s submitted -m 1 //Dev01/main/...
if ($LASTEXITCODE -ne 0 -or $changeLine -notmatch '^Change\s+(\d+)') {
  throw "Cannot determine latest project changelist: $changeLine"
}
$projectChange = [int]$Matches[1]

# UGS associates a PCB with the latest code change visible through the stream
# workspace, not necessarily with the latest project/content changelist which
# triggered the build. Imported Engine changes are included in this query.
$codeExtensions = @(
  '.c', '.cc', '.cpp', '.inl', '.m', '.mm', '.rc', '.cs', '.csproj',
  '.h', '.hpp', '.usf', '.ush', '.uproject', '.uplugin', '.sln',
  '.native.verse'
)
$visibleChanges = & p4 -c build_10_0_0_10_Dev01_main changes -s submitted -m 512 "//build_10_0_0_10_Dev01_main/...@1,$projectChange"
if ($LASTEXITCODE -ne 0) {
  throw 'Cannot enumerate changelists visible to the build workspace.'
}

$codeChange = 0
foreach ($changeEntry in $visibleChanges) {
  if ($changeEntry -notmatch '^Change\s+(\d+)') {
    continue
  }

  $candidateChange = [int]$Matches[1]
  $describe = & p4 -c build_10_0_0_10_Dev01_main describe -s $candidateChange
  if ($LASTEXITCODE -ne 0) {
    throw "Cannot inspect changelist $candidateChange while finding the last code change."
  }

  $containsCode = $false
  foreach ($describeLine in $describe) {
    if ($describeLine -match '^\.\.\.\s+(//\S+)#\d+\s+') {
      $depotFile = $Matches[1]
      if ($codeExtensions | Where-Object { $depotFile.EndsWith($_, [StringComparison]::OrdinalIgnoreCase) }) {
        $containsCode = $true
        break
      }
    }
  }

  if ($containsCode) {
    $codeChange = $candidateChange
    break
  }
}

if ($codeChange -le 0) {
  throw "Cannot determine the last code changelist at or before project CL $projectChange."
}

$ugsIni = & p4 -c build_10_0_0_10_Dev01_main print -q //Dev01/main/Build/UnrealGameSync.ini
if (($ugsIni -join "`n") -notmatch [regex]::Escape('ZippedBinariesPath=//Dev01Binaries/UGS/++Dev01+main-Editor.zip')) {
  throw 'UGS config does not point to the expected zipped binaries depot path.'
}

$stageRoot = Join-Path $ReleaseRoot 'stage_ugs_editor'
$zipLocalPath = Join-Path $ReleaseRoot 'UGS\++Dev01+main-Editor.zip'
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

if (Test-Path $zipLocalPath) {
  # A synced archive is normally not opened on the release client.  Do not
  # treat that expected P4 message as a publishing failure.
  & cmd.exe /d /c "p4 revert `"$ArchiveDepotPath`" 2>NUL" | Out-Null
  Remove-Item -LiteralPath $zipLocalPath -Force
}

$opened = & cmd.exe /d /c "p4 opened `"$ArchiveDepotPath`" 2>NUL"
if ($opened) {
  throw "Archive is already opened in P4: $opened"
}

& p4 sync $ArchiveDepotPath | Out-Host
$editOutput = & p4 edit $ArchiveDepotPath
$editOutput | Out-Host
$openedAfterEdit = & cmd.exe /d /c "p4 opened `"$ArchiveDepotPath`" 2>NUL"
$openedForEdit = $openedAfterEdit -match '\s-\sedit\b'

if (Test-Path $zipLocalPath) {
  Remove-Item -LiteralPath $zipLocalPath -Force
}
Compress-Archive -Path (Join-Path $stageRoot '*') -DestinationPath $zipLocalPath -CompressionLevel Optimal -Force
if (-not (Test-Path $zipLocalPath)) {
  throw "Archive was not created: $zipLocalPath"
}

if (-not $openedForEdit) {
  & p4 add $ArchiveDepotPath | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Unable to open archive for edit/add: $ArchiveDepotPath"
  }
}

# UGS parses the associated source changelist from this exact, zero-padded
# eight-digit prefix. Use the last visible code CL so content-only updates reuse
# the same PCB instead of deleting and redownloading an older archive.
$description = "[CL {0:D8}] Cloud-built DevKitEditor" -f $codeChange
if ($Submit) {
  $submitOutput = & p4 submit -d $description $ArchiveDepotPath
  $submitOutput | Out-Host
  if ($LASTEXITCODE -ne 0) {
    throw 'P4 submit failed.'
  }
} else {
  Write-Output "DRY_RUN_ARCHIVE_READY=$zipLocalPath"
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
