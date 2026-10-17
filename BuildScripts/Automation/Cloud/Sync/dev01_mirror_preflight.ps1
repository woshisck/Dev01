param([switch]$DefineOnly,
    [string]$AgentRoot='C:\BuildAgent\Dev01',
    [string]$GitRoot='C:\Project\Dev01-GitHub',
    [string]$PolicyPath='', [string]$CommonScript='', [string]$ReviewScript='', [string]$StatusPath='')
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
Set-StrictMode -Version 2.0

function Get-Dev01MirrorDirection {
    param([int]$GitShared, [int]$P4Shared)
    if ($GitShared -gt 0 -and $P4Shared -gt 0) { return 'Conflict' }
    if ($GitShared -gt 0) { return 'GitToP4Review' }
    if ($P4Shared -gt 0) { return 'P4ToGitReview' }
    return 'NoSharedChanges'
}

function Invoke-Dev01MirrorRead {
    param([string]$Executable, [string]$Arguments, [int[]]$AllowedExit = @(0), [int]$TimeoutMs = 45000)
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $Executable
    $start.Arguments = $Arguments
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.StandardOutputEncoding = [Text.UTF8Encoding]::new($false)
    $start.StandardErrorEncoding = [Text.UTF8Encoding]::new($false)
    $process = [Diagnostics.Process]::Start($start)
    $outTask = $process.StandardOutput.ReadToEndAsync()
    $errTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutMs)) {
        # Only this owned read-only probe tree is stopped, including Git's HTTPS child.
        $killInfo = New-Object Diagnostics.ProcessStartInfo
        $killInfo.FileName = 'C:\Windows\System32\taskkill.exe'
        $killInfo.Arguments = "/PID $($process.Id) /T /F"
        $killInfo.UseShellExecute = $false
        $killInfo.CreateNoWindow = $true
        $killInfo.RedirectStandardOutput = $true
        $killInfo.RedirectStandardError = $true
        $killer = [Diagnostics.Process]::Start($killInfo)
        $killOutput = $killer.StandardOutput.ReadToEndAsync()
        $killErrors = $killer.StandardError.ReadToEndAsync()
        $killer.WaitForExit()
        $killer.Dispose()
        if (-not $process.WaitForExit(5000)) { $process.Kill() }
        $process.WaitForExit()
        $process.Dispose()
        throw 'ReadOnlyProbeTimeout'
    }
    $result = [pscustomobject]@{ExitCode=$process.ExitCode;Text=($outTask.Result + $errTask.Result).Trim()}
    $process.Dispose()
    if ($AllowedExit -notcontains $result.ExitCode) { throw 'ReadOnlyProbeFailed' }
    return $result
}

if ($DefineOnly) { return }
if(-not $PolicyPath){$PolicyPath=Join-Path $AgentRoot 'dev01_code_sync_policy.json'}
if(-not $CommonScript){$CommonScript=Join-Path $AgentRoot 'dev01_code_sync_common.ps1'}
if(-not $ReviewScript){$ReviewScript=Join-Path $PSScriptRoot 'dev01_sync_review.ps1'}
if(-not $StatusPath){$StatusPath=Join-Path $AgentRoot 'state\safe_mirror_preflight_status.json'}
$env:P4PORT = 'ssl:localhost:1666'
$env:P4USER = 'Dev01BuildAgent'
$env:P4CLIENT = 'bridge_10_0_0_10_Dev01_main'
$env:P4TICKETS = 'C:\BuildAgent\Dev01\secrets\Dev01BuildAgent.ticket'
$env:P4TRUST = 'C:\BuildAgent\Dev01\secrets\p4trust.txt'
$env:GIT_NO_LAZY_FETCH = '1'
$env:GIT_OPTIONAL_LOCKS = '0'
$p4 = 'C:\Program Files\Perforce\p4.exe'
$git = (Get-Command git.exe -ErrorAction Stop).Source
. $CommonScript
. $ReviewScript
$lock = $null
$status = [ordered]@{Timestamp=(Get-Date).ToUniversalTime().ToString('o');Mode='ProtectedPreflightOnly';Status='Blocked';Stage='Initial';Reason='NotChecked';GitCommit=$null;LocalGitCommit=$null;CachedGitCommit=$null;P4Change=$null;GitShared=$null;P4Shared=$null;GitExcluded=0;P4Excluded=0;OverlapCount=0;InvalidPaths=@();PolicyMigration='';Direction=$null;PipelineBuildEntry='Dev01 UGS PCB Build';WritesToGitOrP4=$false;RequiresReviewedConflictRecovery=$true}

try {
    $lock = [IO.File]::Open((Join-Path $agentRoot 'state\safe_mirror_preflight.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    if (Test-Path -LiteralPath (Join-Path $agentRoot 'state\code-sync-pending.json')) { throw 'PendingTransactionRequiresReview' }
    $policy = Get-Dev01SyncPolicy -Path $policyPath
    $state = Get-Dev01SyncState
    if ($null -eq $state -or [int]$state.SchemaVersion -ne 1 -or $state.P4Stream -ne '//Dev01/main' -or $state.GitBranch -ne 'main') { throw 'UnexpectedMirrorLedger' }
    $status.PolicyMigration=Test-Dev01SyncPolicyMigration -Policy $policy -State $state -CurrentPolicyHash (Get-FileHash -LiteralPath $policyPath -Algorithm SHA256).Hash
    $status.Stage = 'BridgeIdentity'
    $info = (Invoke-Dev01MirrorRead $p4 'info').Text
    if ($info -notmatch 'Client root: C:\\Project\\Dev01-BridgeP4' -or $info -notmatch 'Client stream: //Dev01/main') { throw 'UnexpectedBridgeWorkspace' }
    $status.Stage = 'BridgeOpenedGuard'
    $opened = (Invoke-Dev01MirrorRead $p4 'opened' @(0,1)).Text
    if ($opened -notmatch '^File\(s\) not opened on this client\.$') { throw 'BridgeOpenedOrUnreadable' }
    $status.Stage = 'BridgePendingGuard'
    $pending = (Invoke-Dev01MirrorRead $p4 'changes -s pending -c bridge_10_0_0_10_Dev01_main').Text
    if ($pending) { throw 'BridgePendingChanges' }
    $status.Stage = 'GitWorkspaceGuard'
    $gitStatus = (Invoke-Dev01MirrorRead $git "-C `"$gitRoot`" status --porcelain=v1").Text
    if ($gitStatus) { throw 'GitWorkspaceDirty' }
    $origin = (Invoke-Dev01MirrorRead $git "-C `"$gitRoot`" remote get-url origin").Text
    if ($origin -cne 'https://github.com/woshisck/Dev01.git') { throw 'UnexpectedGitRepository' }
    $branch = (Invoke-Dev01MirrorRead $git "-C `"$gitRoot`" branch --show-current").Text
    if ($branch -ne 'main') { throw 'UnexpectedGitBranch' }
    $status.Stage = 'CachedChangeAnalysis'
    $status.P4Change = Get-Dev01P4HeadChange
    # Cached data is diagnostic only. It is not certified as the online GitHub head.
    $cachedHead = (Invoke-Dev01MirrorRead $git "-C `"$gitRoot`" rev-parse origin/main").Text
    $status.CachedGitCommit = $cachedHead
    $status.LocalGitCommit = (Invoke-Dev01MirrorRead $git "-C `"$gitRoot`" rev-parse HEAD").Text
    $gitChanges = @(Get-Dev01GitChangedPaths -Repo $gitRoot -AfterCommit $state.GitCommit -ToCommit $cachedHead)
    $p4Changes = @(Get-Dev01P4ChangedPaths -AfterChange ([int]$state.P4Change))
    $summary=Get-Dev01SyncChangeSummary -GitChanges $gitChanges -P4Changes $p4Changes -Policy $policy
    $gitShared=@($summary.GitShared);$p4Shared=@($summary.P4Shared)
    $status.GitShared = $gitShared.Count
    $status.P4Shared = $p4Shared.Count
    $status.GitExcluded=@($summary.GitExcluded).Count;$status.P4Excluded=@($summary.P4Excluded).Count
    $status.OverlapCount=@($summary.Overlap).Count
    $status.InvalidPaths=@(@($summary.GitInvalid)+@($summary.P4Invalid)|Select-Object -First 30)
    $status.Direction = Get-Dev01MirrorDirection -GitShared $gitShared.Count -P4Shared $p4Shared.Count
    $status.Stage = 'GitRemoteProbe'
    $advertised = (Invoke-Dev01MirrorRead $git "-C `"$gitRoot`" -c http.version=HTTP/1.1 -c http.lowSpeedLimit=1 -c http.lowSpeedTime=15 ls-remote origin refs/heads/main" @(0) 45000).Text
    if ($advertised -notmatch '^([0-9a-fA-F]{40})\s+refs/heads/main$') { throw 'UnreadableGitRemoteHead' }
    $status.GitCommit = $Matches[1].ToLowerInvariant()
    if (@($summary.GitInvalid).Count -gt 0 -or @($summary.P4Invalid).Count -gt 0) { throw 'ChangedPathsViolateWhitelist' }
    if ($status.GitCommit -ne $cachedHead) { throw 'RemoteHeadChangedRequiresReviewedFetch' }
    if ($status.LocalGitCommit -ne $status.GitCommit) { throw 'LocalGitHeadDiffersFromRemote' }
    if ($status.Direction -eq 'Conflict') { throw 'SharedChangesOnBothSides' }
    if ($status.Direction -ne 'NoSharedChanges') { throw 'ReviewedNonDestructiveTransactionRequired' }
    if($status.PolicyMigration -ne 'Current'){throw 'CheckpointMigrationRequiresVerifiedReconciliation'}
    # The costly all-tree check is only needed before a no-change verdict, not after an earlier blocker.
    $status.Stage = 'BridgeByteGuard'
    $dirty = (Invoke-Dev01MirrorRead $p4 'reconcile -n -I -e -a -d //Dev01/main/...' @(0,1) 90000).Text
    if ($dirty -and $dirty -notmatch '^No file\(s\) to reconcile\.$') { throw 'BridgeDirtyOrUnreadable' }
    $status.Status = 'VerifiedNoSharedChanges'
    $status.Reason = 'NoSharedChangesSinceLedger'
} catch {
    # Persist stable categories only, never credentials or raw native stderr.
    $reason = $_.Exception.Message
    if ($reason -notmatch '^[A-Za-z][A-Za-z0-9]+$') { $reason = 'PreflightFailedSeeAudit' }
    $status.Reason = $reason
} finally {
    if ($lock) { $lock.Dispose() }
}
$status | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $statusPath -Encoding UTF8
$status | ConvertTo-Json -Depth 4 | Write-Output
if ($status.Status -eq 'Blocked') { exit 2 }
