# Emits a non-active review artifact under Saved; never replaces the cloud ledger.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$RepositoryRoot,[int]$P4Snapshot=165)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'dev01_sync_review.ps1')
$gitBase=(& git -C $RepositoryRoot rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0){throw 'Git revision lookup failed.'}
$tree=(& git -C $RepositoryRoot write-tree).Trim()
if($LASTEXITCODE -ne 0){throw 'Unresolved index; no checkpoint candidate emitted.'}
$policyHash=(Get-FileHash (Join-Path $PSScriptRoot 'dev01_code_sync_policy.json')).Hash
$candidate=New-Dev01SyncCheckpointCandidate -GitBase $gitBase -MergedTree $tree -P4Snapshot $P4Snapshot -PolicyHash $policyHash
$outputDirectory=Join-Path $RepositoryRoot 'Saved\SyncRepairValidation'
New-Item -ItemType Directory -Path $outputDirectory -Force|Out-Null
$outputPath=Join-Path $outputDirectory 'CheckpointCandidate.json'
[IO.File]::WriteAllText($outputPath,($candidate|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
Write-Output "NON_ACTIVE_CHECKPOINT_CANDIDATE=$outputPath"
