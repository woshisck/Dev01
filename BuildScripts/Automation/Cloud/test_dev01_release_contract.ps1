[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'dev01_release_contract.ps1')
$script:passed = 0
function Assert-True([bool]$Value, [string]$Name) {
  if (-not $Value) { throw "FAIL: $Name" }
  $script:passed++
  Write-Output "PASS: $Name"
}
function Assert-Rejected([scriptblock]$Action, [string]$Name) {
  $rejected = $false
  try { & $Action } catch { $rejected = $true }
  Assert-True $rejected $Name
}
foreach ($name in @('dev01_release_contract.ps1', 'dev01_ci_project_build.ps1', 'dev01_publish_ugs_binaries.ps1', 'dev01_auto_ugs_build.ps1')) {
  $tokens = $null; $errors = $null
  [void][Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot $name), [ref]$tokens, [ref]$errors)
  Assert-True ($errors.Count -eq 0) "$name parses"
}
$inventory = @([pscustomobject]@{ path = 'Binaries/Win64/DevKitEditor.target'; length = 12; sha256 = 'abc' },
  [pscustomobject]@{ path = 'Plugins/Example/Binaries/Win64/UnrealEditor-Example.dll'; length = 20; sha256 = 'def' })
$receipt = [pscustomobject]@{ schema = 1; status = 'complete'; project_cl = 127; code_cl = 127; engine_build_id = 'engine-id'; workspace_stamp = 'have-127'; output_hash = Get-Dev01InventoryHash $inventory }
Assert-Dev01BuildReceipt $receipt 127 127 'engine-id' 'have-127' $inventory
Assert-True $true 'completed snapshot accepted even if depot head later advances'
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 129 127 'engine-id' 'have-127' $inventory } 'new project label rejected'
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 129 'engine-id' 'have-127' $inventory } 'new code label rejected'
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 127 'other' 'have-127' $inventory } 'Engine BuildId drift rejected'
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 127 'engine-id' 'have-129' $inventory } 'workspace/import drift rejected'
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 127 'engine-id' 'have-127' @($inventory[0]) } 'missing DLL rejected'
$changed = @([pscustomobject]@{ path = $inventory[0].path; length = 12; sha256 = 'changed' }, $inventory[1])
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 127 'engine-id' 'have-127' $changed } 'modified target rejected'
$added = @($inventory) + @([pscustomobject]@{ path = 'Binaries/Win64/unexpected.dll'; length = 1; sha256 = 'xyz' })
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 127 'engine-id' 'have-127' $added } 'unproven output rejected'
$receipt.status = 'building'
Assert-Rejected { Assert-Dev01BuildReceipt $receipt 127 127 'engine-id' 'have-127' $inventory } 'failed/interrupted build cannot reuse receipt'
$receipt.status = 'complete'
Assert-True ((Get-Dev01InventoryHash @($inventory[1], $inventory[0])) -eq $receipt.output_hash) 'inventory ordering is stable'

# No real Perforce or production build processes are invoked by this test.
$script:mockEngineCL = 76
$script:mockDirty = $false
$script:mockMissing = $false
$script:mockNeedsSync = $false
function p4 {
  $global:LASTEXITCODE = 0
  $command = $args -join ' '
  if ($command -match 'opened') { return }
  if ($command -match ' have ') { return '//Dev01/main/Source/file.cpp#1 - C:\Project\file.cpp' }
  if ($command -match 'stream -o') { return "`timport Engine/... //Dev01Engine/main/...@$script:mockEngineCL" }
  if ($command -match 'sync -n') {
    if ($script:mockNeedsSync) { return '//Dev01Engine/main/Binaries/Win64/UnrealEditor.exe#2 - updating C:\Project\Engine\UnrealEditor.exe' }
    return '//Dev01/main/...@127 - file(s) up-to-date.'
  }
  if ($command -match ' diff ') {
    if ($script:mockDirty -and $command -match '-se') { return 'C:\Project\Source\modified.cpp' }
    if ($script:mockMissing -and $command -match '-sd') { return 'C:\Project\Source\missing.cpp' }
    return
  }
  if ($command -match ' changes ') { return 'Change 127 on 2026/09/08 by test@client' }
  if ($command -match ' describe ') { return '... //Dev01/main/Source/file.cpp#1 edit' }
  throw "Unexpected mock command: $command"
}
Assert-True ((Get-Dev01CodeChange 127 'test-client') -eq 127) 'CodeCL resolved at selected snapshot'
$stamp = Get-Dev01WorkspaceStamp 'test-client'
$script:mockEngineCL = 77
Assert-True ((Get-Dev01WorkspaceStamp 'test-client') -ne $stamp) 'Engine import is part of the workspace fingerprint'
Assert-Dev01WorkspaceInputs 'test-client' 127 76
Assert-True $true 'clean bytes and full pinned view accepted'
$script:mockDirty = $true
Assert-Rejected { Assert-Dev01WorkspaceInputs 'test-client' 127 76 } 'allwrite un-opened input modification rejected'
$script:mockDirty = $false
$script:mockMissing = $true
Assert-Rejected { Assert-Dev01WorkspaceInputs 'test-client' 127 76 } 'missing tracked local path rejected'
$script:mockMissing = $false
$script:mockNeedsSync = $true
Assert-Rejected { Assert-Dev01WorkspaceInputs 'test-client' 127 76 } 'incomplete pinned Engine rejected'
Remove-Item Function:\p4
function p4 { & $env:ComSpec /d /c 'echo File(s) not opened on this client. 1>&2' }
$stderrResult = Invoke-Dev01P4Capture @('opened')
Assert-True (($stderrResult.Lines -join ' ') -match 'not opened') 'PowerShell 5.1 expected native stderr remains inspectable'
Assert-True ($ErrorActionPreference -eq 'Stop') 'native wrapper restores caller error policy'
Remove-Item Function:\p4

$publisher = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'dev01_publish_ugs_binaries.ps1') -Raw
$builder = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'dev01_ci_project_build.ps1') -Raw
$auto = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'dev01_auto_ugs_build.ps1') -Raw
Assert-True ($publisher -notmatch 'changes -s submitted') 'publisher never labels DLLs from a new depot head query'
Assert-True ($publisher -notmatch 'p4 revert') 'publisher never reverts existing work'
Assert-True ($builder.Contains('"//Dev01/main/...@$ProjectChange"')) 'build sync is pinned'
Assert-True ($auto.Contains('& $buildScript -Force -ProjectChange $projectChange -CodeChange $codeChange') -and
  $auto.Contains('& $publishScript -Submit -ProjectChange $projectChange -CodeChange $codeChange')) 'orchestrator passes same snapshot end-to-end'
$submitBranch = $publisher.IndexOf('if ($Submit)')
Assert-True ($publisher.IndexOf('& p4 edit') -gt $submitBranch -and $publisher.IndexOf('& p4 sync') -gt $submitBranch) 'dry run cannot open or sync archive'
Assert-True ($builder.Contains('Enter-Dev01PipelineLease') -and $publisher.Contains('Enter-Dev01PipelineLease') -and
  $auto.Contains('-PipelineLease $lockStream')) 'direct and orchestrated runs share one lease'
Assert-True ($publisher.Contains('Build receipt was replaced during packaging.') -and
  $publisher.Contains('Staged ZIP copy failed verification')) 'receipt and final ZIP are rechecked before submit'
Write-Output "RELEASE_CONTRACT_TESTS_PASSED=$script:passed"
