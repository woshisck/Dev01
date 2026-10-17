# Read-only planning primitives. Nothing in this file submits, pushes or advances a ledger.
Set-StrictMode -Version 2.0

function Get-Dev01SyncChangeSummary {
    param([object[]]$GitChanges=@(),[object[]]$P4Changes=@(),[Parameter(Mandatory)]$Policy)
    $gitClassified=@($GitChanges | ForEach-Object { [pscustomobject]@{Path=$_.Relative;Action=$_.Action;Class=(Get-Dev01PathClass $_.Relative $Policy)} })
    $p4Classified=@($P4Changes | ForEach-Object { [pscustomobject]@{Path=$_.Relative;Action=$_.Action;Class=(Get-Dev01PathClass $_.Relative $Policy)} })
    $gitShared=@($gitClassified|Where-Object Class -EQ 'shared')
    $p4Shared=@($p4Classified|Where-Object Class -EQ 'shared')
    $p4Paths=@($p4Shared|Select-Object -ExpandProperty Path)
    return [pscustomobject]@{
        GitShared=$gitShared;P4Shared=$p4Shared
        GitExcluded=@($gitClassified|Where-Object Class -In @('git-only','forbidden'))
        P4Excluded=@($p4Classified|Where-Object Class -EQ 'forbidden')
        GitInvalid=@($gitClassified|Where-Object Class -EQ 'unknown')
        P4Invalid=@($p4Classified|Where-Object Class -In @('unknown','git-only'))
        Overlap=@($gitShared|Where-Object {$_.Path -in $p4Paths})
    }
}

function Get-Dev01ThreeWayDecision {
    param([string]$BaseGitHash='', [string]$BaseP4Hash='', [string]$GitHash='', [string]$P4Hash='')
    foreach($hash in @($BaseGitHash,$BaseP4Hash,$GitHash,$P4Hash)) {
        if($hash -and $hash -cnotmatch '^[a-fA-F0-9]{64}$'){throw 'Invalid content fingerprint.'}
    }
    if($GitHash -eq $P4Hash){return 'Converged'}
    $gitChanged=$GitHash -ne $BaseGitHash
    $p4Changed=$P4Hash -ne $BaseP4Hash
    if($gitChanged -and $p4Changed){return 'NeedsThreeWayMerge'}
    if(-not $gitChanged -and -not $p4Changed){return 'PreserveReviewedBaselineDifference'}
    if(($gitChanged -and -not $GitHash) -or ($p4Changed -and -not $P4Hash)){return 'DeletionRequiresReview'}
    if($gitChanged){return 'ReviewGitToP4'}
    return 'ReviewP4ToGit'
}

function Test-Dev01SyncPolicyMigration {
    param([Parameter(Mandatory)]$Policy,[Parameter(Mandatory)]$State,[Parameter(Mandatory)][string]$CurrentPolicyHash)
    if($State.PolicySha256 -eq $CurrentPolicyHash){return 'Current'}
    if($Policy.PSObject.Properties.Name -contains 'classificationOnlyMigration' -and
        $Policy.classificationOnlyMigration -eq $true -and $Policy.previousPolicySha256 -eq $State.PolicySha256){return 'ClassificationOnlyReviewRequired'}
    throw 'MirrorPolicyChanged'
}

function New-Dev01SyncCheckpointCandidate {
    param([Parameter(Mandatory)][string]$GitBase,[Parameter(Mandatory)][string]$MergedTree,
        [Parameter(Mandatory)][int]$P4Snapshot,[Parameter(Mandatory)][string]$PolicyHash)
    if($GitBase -cnotmatch '^[a-f0-9]{40}$' -or $MergedTree -cnotmatch '^[a-f0-9]{40}$' -or
        $P4Snapshot -le 0 -or $PolicyHash -cnotmatch '^[a-fA-F0-9]{64}$'){throw 'Invalid checkpoint candidate.'}
    return [ordered]@{SchemaVersion=2;State='PendingReviewAndPublication';GitBase=$GitBase;MergedGitTree=$MergedTree;
        P4Snapshot=$P4Snapshot;PolicySha256=$PolicyHash;ReadyToActivate=$false;RequiresGitPublication=$true;
        RequiresP4Reconciliation=$true;RequiresPerPathByteVerification=$true;WritesToGitOrP4=$false}
}
