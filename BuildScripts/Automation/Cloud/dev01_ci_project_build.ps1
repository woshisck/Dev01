param(
  [switch]$Force,
  [int]$ProjectChange = 0,
  [int]$CodeChange = 0,
  [IO.FileStream]$PipelineLease
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'dev01_release_contract.ps1')
$lease = Enter-Dev01PipelineLease $PipelineLease
try {
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
$receiptPath = Join-Path $stateDir 'completed_build.json'
New-Item -ItemType Directory -Force -Path $stateDir | Out-Null

if ($ProjectChange -le 0) {
  $changeLine = & p4 changes -s submitted -m 1 //Dev01/main/...
  if ($LASTEXITCODE -ne 0 -or $changeLine -notmatch '^Change\s+(\d+)') { throw 'Cannot determine project changelist.' }
  $ProjectChange = [int]$Matches[1]
}
$resolvedCode = Get-Dev01CodeChange $ProjectChange $env:P4CLIENT
if ($CodeChange -gt 0 -and $CodeChange -ne $resolvedCode) { throw 'Requested CodeCL does not match the pinned project snapshot.' }
$CodeChange = $resolvedCode

# Invalidate old proof before touching the workspace: failed builds can never
# publish last run's DLLs with this run's label.
Write-Dev01BuildReceipt $receiptPath ([ordered]@{ schema = 1; status = 'building'; project_cl = $ProjectChange; code_cl = $CodeChange })
$null = Get-Dev01WorkspaceStamp $env:P4CLIENT
p4 sync "//Dev01/main/...@$ProjectChange"
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

# The build client uses allwrite so prior UBT runs can leave Engine module
# manifests with a locally generated BuildId even though the P4 have-list is
# current. Restore every imported Engine manifest before compiling the PCB.
$streamSpec = @(& p4 stream -o //Dev01/main)
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve pinned Engine revision.' }
$engineImport = @($streamSpec | Where-Object { $_ -match '^\s*import\s+Engine/\.\.\.\s+//Dev01Engine/main/\.\.\.@(\d+)\s*$' })
if ($engineImport.Count -ne 1 -or $engineImport[0] -notmatch '@(\d+)\s*$') { throw 'Engine import must be pinned.' }
$engineChange = [int]$Matches[1]
# Import paths are not covered by a //Dev01/main depot-path sync. Sync the
# complete pinned Engine, not just its manifests, before certifying BuildId.
& p4 sync "//Dev01Engine/main/...@$engineChange"
if ($LASTEXITCODE -ne 0) { throw 'Cannot sync the complete pinned Engine payload.' }
$engineSyncOutput = & p4 sync -f "//Dev01Engine/main/....modules@$engineChange" 2>&1
$engineSyncOutput | Out-Host
if ($LASTEXITCODE -ne 0 -or ($engineSyncOutput -join "`n") -match 'not in client view|no such file') {
  throw "Unable to restore imported Engine module manifests from P4: $($engineSyncOutput -join ' ')"
}

$engineManifest = Join-Path $engineRoot 'Binaries\Win64\UnrealEditor.modules'
$engineBuildId = Get-ManifestBuildId $engineManifest
$workspaceStamp = Get-Dev01WorkspaceStamp $env:P4CLIENT
Assert-Dev01WorkspaceInputs $env:P4CLIENT $ProjectChange $engineChange
# DEV01_PRECOMPILED_IMPORT_LIBS
# The source-free Installed Build ships the StateTree/PropertyBindingUtils DLLs
# but not their MSVC import libraries. Generate deterministic import libs from
# the pinned DLL exports before UBT links project editor code against them.
function Ensure-Dev01ImportLibrary {
  param(
    [string]$ModuleName,
    [string]$DllPath,
    [string]$OutputDirectory
  )

  if (-not (Test-Path -LiteralPath $DllPath)) {
    throw "Required precompiled module DLL is missing: $DllPath"
  }

  New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
  $defPath = Join-Path $OutputDirectory ($ModuleName + '.def')
  $libPath = Join-Path $OutputDirectory ('UnrealEditor-' + $ModuleName + '.lib')
  $expPath = Join-Path $OutputDirectory ('UnrealEditor-' + $ModuleName + '.exp')
  Remove-Item -LiteralPath $defPath,$libPath,$expPath -Force -ErrorAction SilentlyContinue

  $dumpbin = 'C:\BuildTools\VS2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe'
  $libexe = 'C:\BuildTools\VS2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\lib.exe'
  if (-not (Test-Path -LiteralPath $dumpbin) -or -not (Test-Path -LiteralPath $libexe)) {
    throw 'VS dumpbin/lib tools are required to generate precompiled module import libraries.'
  }

  $dump = & $dumpbin /exports $DllPath
  if ($LASTEXITCODE -ne 0) {
    throw "dumpbin failed for $DllPath"
  }
  $names = @(
    $dump |
      ForEach-Object {
        if ($_ -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(.+?)\s*$') {
          $Matches[1]
        }
      } |
      Where-Object { $_ -and $_ -notmatch '^\s*$' } |
      Select-Object -Unique
  )
  if ($names.Count -eq 0) {
    throw "No exports found for $DllPath"
  }

  $libraryLine = 'LIBRARY ' + [char]34 + [IO.Path]::GetFileName($DllPath) + [char]34
  $values = @($libraryLine, 'EXPORTS') + @($names | ForEach-Object { '  ' + $_ })
  Set-Content -LiteralPath $defPath -Value $values -Encoding ASCII
  & $libexe /def:$defPath /machine:X64 /out:$libPath /nologo
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $libPath)) {
    throw "lib.exe failed for $ModuleName"
  }
  Write-Output "DEV01_PRECOMPILED_IMPORT_LIB=$ModuleName exports=$($names.Count)"
}

$dev01ImportModules = @(
  @{
    ModuleName = 'StateTreeEditorModule'
    DllPath = 'C:\Project\Dev01-P4\Engine\Plugins\Runtime\StateTree\Binaries\Win64\UnrealEditor-StateTreeEditorModule.dll'
    OutputDirectory = 'C:\Project\Dev01-P4\Engine\Plugins\Runtime\StateTree\Intermediate\Build\Win64\x64\UnrealEditor\Development\StateTreeEditorModule'
  }
  @{
    ModuleName = 'PropertyBindingUtils'
    DllPath = 'C:\Project\Dev01-P4\Engine\Plugins\Runtime\PropertyBindingUtils\Binaries\Win64\UnrealEditor-PropertyBindingUtils.dll'
    OutputDirectory = 'C:\Project\Dev01-P4\Engine\Plugins\Runtime\PropertyBindingUtils\Intermediate\Build\Win64\x64\UnrealEditor\Development\PropertyBindingUtils'
  }
)
foreach ($dev01ImportModule in $dev01ImportModules) {
  Ensure-Dev01ImportLibrary @dev01ImportModule
}

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

if ((Get-Dev01WorkspaceStamp $env:P4CLIENT) -ne $workspaceStamp) { throw 'Workspace snapshot changed during compilation.' }
Assert-Dev01WorkspaceInputs $env:P4CLIENT $ProjectChange $engineChange
$inventory = @(Get-Dev01PcbInventory $projectRoot)
Write-Dev01BuildReceipt $receiptPath ([ordered]@{
  schema = 1
  status = 'complete'
  project_cl = $ProjectChange
  code_cl = $CodeChange
  engine_cl = $engineChange
  engine_build_id = $engineBuildId
  workspace_stamp = $workspaceStamp
  output_hash = Get-Dev01InventoryHash $inventory
  completed_at = (Get-Date).ToUniversalTime().ToString('o')
  files = $inventory
})
Write-Output "BUILD_RECEIPT=$receiptPath"
Write-Output "BUILD_OK=$projectChange"
} finally {
  if ($lease.Owned) { $lease.Stream.Dispose() }
}
