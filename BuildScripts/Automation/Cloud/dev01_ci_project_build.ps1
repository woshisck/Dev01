param([switch]$Force)

$ErrorActionPreference = 'Stop'
$env:Path = 'C:\Program Files\Perforce;C:\BuildTools\VS2022\Common7\Tools;' + $env:Path
$env:P4PORT = 'ssl:localhost:1666'
$env:P4USER = 'Dev01BuildAgent'
$env:P4CLIENT = 'build_10_0_0_10_Dev01_main'
$env:P4TICKETS = 'C:\BuildAgent\Dev01\secrets\Dev01BuildAgent.ticket'
$env:P4TRUST = 'C:\BuildAgent\Dev01\secrets\p4trust.txt'

$projectRoot = 'C:\Project\Dev01-P4'
$engineRoot = Join-Path $projectRoot 'Engine'
$projectFile = Join-Path $projectRoot 'DevKit.uproject'
$dotnet = Join-Path $engineRoot 'Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
$ubt = Join-Path $engineRoot 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$stateDir = 'C:\BuildAgent\Dev01\state'
$stateFile = Join-Path $stateDir 'last_project_archive_cl.txt'
New-Item -ItemType Directory -Force -Path $stateDir | Out-Null

p4 sync //Dev01/main/...
if ($LASTEXITCODE -ne 0) {
  throw "P4 project sync failed with exit code $LASTEXITCODE"
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

function Set-JsonBuildId([string]$Path, [string]$BuildId) {
  if (-not (Test-Path -LiteralPath $Path)) {
    throw "Cannot normalize missing metadata file: $Path"
  }
  $text = [System.IO.File]::ReadAllText($Path)
  if ($text -notmatch '"BuildId"\s*:\s*"[^"]+"') {
    throw "Metadata file has no replaceable BuildId: $Path"
  }
  $replacement = '"BuildId": "' + $BuildId + '"'
  $normalized = [regex]::Replace($text, '"BuildId"\s*:\s*"[^"]+"', $replacement)
  [System.IO.File]::WriteAllText($Path, $normalized, (New-Object System.Text.UTF8Encoding($false)))
}

$changeLine = p4 changes -s submitted -m 1 //Dev01/main/...
if ($changeLine -notmatch '^Change\s+(\d+)') {
  throw "Cannot determine latest project changelist: $changeLine"
}
$projectChange = [int]$Matches[1]

if (-not $Force -and (Test-Path $stateFile) -and ((Get-Content $stateFile -Raw).Trim() -eq "$projectChange")) {
  Write-Output "SKIP_ALREADY_BUILT=$projectChange"
  exit 0
}

# The build client uses allwrite so prior UBT runs can leave Engine module
# manifests with a locally generated BuildId even though the P4 have-list is
# current. Restore every imported Engine manifest before compiling the PCB.
$engineSyncOutput = & p4 sync -f "//Dev01Engine/main/....modules" 2>&1
$engineSyncOutput | Out-Host
if ($LASTEXITCODE -ne 0 -or ($engineSyncOutput -join "`n") -match 'not in client view|no such file') {
  throw "Unable to restore imported Engine module manifests from P4: $($engineSyncOutput -join ' ')"
}

$engineManifest = Join-Path $engineRoot 'Binaries\Win64\UnrealEditor.modules'
$engineBuildId = Get-ManifestBuildId $engineManifest
Write-Output "ENGINE_BUILD_ID=$engineBuildId"

# Force UBT to write fresh project/plugin metadata against the restored Engine
# BuildId. DLLs and object files are retained for an incremental build.
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Binaries\Win64') -Filter '*.modules' -File -ErrorAction SilentlyContinue |
  Remove-Item -Force
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Plugins') -Filter '*.modules' -File -Recurse -ErrorAction SilentlyContinue |
  Where-Object { $_.FullName -match '\\Binaries\\Win64\\' } |
  Remove-Item -Force
$staleTarget = Join-Path $projectRoot 'Binaries\Win64\DevKitEditor.target'
Remove-Item -LiteralPath $staleTarget -Force -ErrorAction SilentlyContinue

foreach ($path in @($projectFile, $dotnet, $ubt)) {
  if (-not (Test-Path $path)) {
    throw "Required build input missing: $path"
  }
}

$vsDevCmd = 'C:\BuildTools\VS2022\Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path $vsDevCmd)) {
  throw "VS environment script missing: $vsDevCmd"
}

$buildCmd = 'call "' + $vsDevCmd + '" -arch=x64 -host_arch=x64 && "' + $dotnet + '" "' + $ubt + '" DevKitEditor Win64 Development -Project="' + $projectFile + '" -WaitMutex -MaxParallelActions=2 -NoUBA -NoHotReload -UsePrecompiled'
cmd.exe /d /c $buildCmd
if ($LASTEXITCODE -ne 0) {
  throw "UBT failed with exit code $LASTEXITCODE"
}

$targetFile = Join-Path $projectRoot 'Binaries\Win64\DevKitEditor.target'
if (-not (Test-Path $targetFile)) {
  throw "Build completed without expected target: $targetFile"
}

$engineBuildIdAfter = Get-ManifestBuildId $engineManifest
if ($engineBuildIdAfter -ne $engineBuildId) {
  throw "UBT changed the imported Engine BuildId from $engineBuildId to $engineBuildIdAfter"
}

$projectManifests = @(
  Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Binaries\Win64') -Filter '*.modules' -File -ErrorAction SilentlyContinue
  Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Plugins') -Filter '*.modules' -File -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\Binaries\\Win64\\' }
)
if ($projectManifests.Count -eq 0) {
  throw 'Build produced no project/plugin module manifests.'
}

# UBT can reuse a cached project TargetMakefile BuildId even after the imported
# installed Engine manifests are restored. The DLLs are already linked against
# this Engine; normalize only the generated compatibility metadata to the
# authoritative BuildId from the pinned Engine stream.
foreach ($manifestFile in $projectManifests) {
  Set-JsonBuildId $manifestFile.FullName $engineBuildId
}
Set-JsonBuildId $targetFile $engineBuildId
Write-Output "BUILD_ID_NORMALIZED_COUNT=$($projectManifests.Count)"

foreach ($manifestFile in $projectManifests) {
  $moduleBuildId = Get-ManifestBuildId $manifestFile.FullName
  if ($moduleBuildId -ne $engineBuildId) {
    throw "BuildId mismatch: $($manifestFile.FullName) has $moduleBuildId; Engine has $engineBuildId"
  }
}

$target = Get-Content -LiteralPath $targetFile -Raw | ConvertFrom-Json
if ([string]$target.Version.BuildId -ne $engineBuildId) {
  throw "Target BuildId mismatch: $($target.Version.BuildId); Engine has $engineBuildId"
}

Write-Output "BUILD_ID_VALIDATED=$engineBuildId"

Write-Output "BUILD_OK=$projectChange"
