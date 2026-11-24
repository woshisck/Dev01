[CmdletBinding()]
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$node = 'C:\Project\Dev01-GitToP4-Tools-20261023\node.exe'
$worker = 'C:\Project\Dev01-GitToP4-Auto-20261024\v3-worker\auto-sync.mjs'
$stateRoot = 'C:\BuildAgent\Dev01\state\git-to-p4-v3'
$statusPath = Join-Path $stateRoot 'status.json'
$notificationPath = Join-Path $stateRoot 'notification.json'
$eventScript = 'C:\CodexBridge\scripts\Send-CloudWorkflowEvent.ps1'

if (-not (Test-Path -LiteralPath $node) -or -not (Test-Path -LiteralPath $worker)) {
    throw 'Auto mirror runtime is missing.'
}

function Read-FreshStatus([DateTime]$since) {
    if (-not (Test-Path -LiteralPath $statusPath)) { return $null }
    $item = Get-Item -LiteralPath $statusPath
    if ($item.LastWriteTimeUtc -lt $since.AddSeconds(-2)) { return $null }
    return Get-Content -LiteralPath $statusPath -Raw -Encoding UTF8 | ConvertFrom-Json
}

function Write-NotificationState([string]$token) {
    $temp = "$notificationPath.$PID.tmp"
    [IO.File]::WriteAllText($temp, (@{lastToken=$token;updatedAt=[DateTimeOffset]::UtcNow.ToString('o')} | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
    if (Test-Path -LiteralPath $notificationPath) {
        [IO.File]::Replace($temp, $notificationPath, "$notificationPath.previous")
    } else { [IO.File]::Move($temp, $notificationPath) }
}

function Notify-Once([string]$token,[string]$stage,[string]$message) {
    $prior = $null
    if (Test-Path -LiteralPath $notificationPath) {
        try { $prior = Get-Content -LiteralPath $notificationPath -Raw -Encoding UTF8 | ConvertFrom-Json }
        catch { throw 'Notification state unreadable; refusing duplicate event.' }
    }
    if ($prior -and $prior.lastToken -eq $token) { return }
    $hash = [Security.Cryptography.SHA256]::Create()
    try { $hashBytes = $hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($token)) }
    finally { $hash.Dispose() }
    $job = 'mirror-v3-' + ([BitConverter]::ToString($hashBytes)).Replace('-', '').Substring(0, 24).ToLowerInvariant()
    & $eventScript -Workflow dev01-mirror -JobId $job -Stage $stage -Attempt 1 -Message $message
    Write-NotificationState $token
}

$started = [DateTime]::UtcNow
$mode = if ($DryRun) { '--dry-run' } else { '--apply' }
$oldPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
try { & $node $worker $mode; $workerExit = $LASTEXITCODE }
finally { $ErrorActionPreference = $oldPreference }

$status = Read-FreshStatus $started
if (-not $status) { throw 'Auto mirror worker ended without a fresh status.' }
if ($DryRun) { exit $workerExit }

$remote = ''
if ($status.Status -eq 'Blocked') {
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $remoteLine = & git -C C:\Project\Dev01-GitHub ls-remote origin refs/heads/main 2>$null
        if ($LASTEXITCODE -eq 0 -and $remoteLine -match '^([0-9a-f]{40})\s+') { $remote = $Matches[1] }
    } finally { $ErrorActionPreference = $oldPreference }
}
$key = "$($status.Status):$($status.Reason):$($status.GitCommit):$($status.P4Change):$remote"
switch ([string]$status.Status) {
    'VerifiedNoSharedChanges' {
        if ($status.Reason -in @('SubmittedMetadataReusingPCB','SubmittedAndPCBVerified')) {
            Notify-Once $key 'completed' "Dev01 GitHub to P4 auto mirror complete. Git: $($status.GitCommit); P4 CL: $($status.P4Change); $($status.Reason)."
        }
        exit 0
    }
    'SubmittedAwaitingPCB' {
        Notify-Once $key 'publish' "Dev01 GitHub code was submitted to P4 CL $($status.P4Change). Waiting for project/plugin build and matching UGS PCB."
        exit 0
    }
    'Blocked' {
        Notify-Once $key 'needs_input' "Dev01 auto mirror paused: $($status.Reason). GitHub: $remote. Inspect the isolated client and C:\BuildAgent\Dev01\state\git-to-p4-v3\status.json. Unknown submit/publish outcomes are not retried."
        exit 1
    }
    default { throw "Unknown auto mirror status: $($status.Status)" }
}
