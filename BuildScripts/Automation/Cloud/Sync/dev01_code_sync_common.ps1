Set-StrictMode -Version 2.0

function ConvertTo-Dev01RelativePath {
    param([Parameter(Mandatory=$true)][string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path) -or $Path -match '[\x00-\x1f]') { throw 'Invalid sync path.' }
    $normalized = $Path -replace '\\', '/'
    while ($normalized.StartsWith('./', [StringComparison]::Ordinal)) {
        $normalized = $normalized.Substring(2)
    }
    if ($normalized.StartsWith('/', [StringComparison]::Ordinal) -or $normalized -match '^[A-Za-z]:' -or $normalized.Contains(':')) {
        throw "Rooted paths and alternate data streams are not allowed in code sync: $Path"
    }
    $segments = @($normalized.Split('/'))
    if ($segments -contains '..' -or $segments -contains '.' -or $segments -contains '') {
        throw "Dot path segments are not allowed in code sync: $Path"
    }
    return $normalized
}

function Get-Dev01SyncPolicy {
    param([string]$Path = 'C:\BuildAgent\Dev01\dev01_code_sync_policy.json')
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Code sync policy is missing: $Path"
    }
    return (Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json)
}

function Get-Dev01PathClass {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)]$Policy
    )

    $normalized = ConvertTo-Dev01RelativePath $Path
    $lower = $normalized.ToLowerInvariant()
    $segments = @($lower.Split('/') | Where-Object { $_ -ne '' })
    $extension = [IO.Path]::GetExtension($lower)

    $isV2 = $Policy.PSObject.Properties.Name -contains 'allowedSharedExtensions'
    if ($isV2) {
        foreach ($segment in $Policy.generatedPathSegments) {
            if ($segments -contains ([string]$segment).ToLowerInvariant()) { return 'forbidden' }
        }
        if ($Policy.excludedExactPaths -contains $normalized) { return 'forbidden' }
    }

    # Binary/engine/generated bans are checked before narrowly scoped Content code exceptions.
    if ($Policy.forbiddenExtensions -contains $extension -or $segments[0] -eq 'engine') { return 'forbidden' }
    foreach ($segment in $Policy.forbiddenPathSegments) {
        if ($segment -ne 'Content' -and $segments -contains ([string]$segment).ToLowerInvariant()) { return 'forbidden' }
    }
    if ($isV2) {
        if ($Policy.gitOnlyExactPaths -contains $normalized) { return 'git-only' }
        foreach ($prefix in $Policy.sharedLuaPrefixes) {
            if ($lower.StartsWith(([string]$prefix).ToLowerInvariant()) -and $extension -eq '.lua') { return 'shared' }
        }
        if ($Policy.sharedTextExactPaths -contains $normalized) { return 'shared' }
        if ($Policy.sharedBinaryExactPaths -contains $normalized) { return 'shared' }
        if ($lower -match '^plugins/[^/]+/(readme\.md|license(\.md|\.txt)?|notice(\.md|\.txt)?)$') { return 'shared' }
    }

    foreach ($prefix in $Policy.forbiddenPrefixes) {
        if ($lower.StartsWith(([string]$prefix).ToLowerInvariant())) {
            return 'forbidden'
        }
    }
    if ($Policy.forbiddenExtensions -contains $extension) {
        return 'forbidden'
    }
    foreach ($segment in $Policy.forbiddenPathSegments) {
        if ($segments -contains ([string]$segment).ToLowerInvariant()) {
            return 'forbidden'
        }
    }

    foreach ($exact in $Policy.gitOnlyExactPaths) {
        if ($lower -eq ([string]$exact).ToLowerInvariant()) {
            return 'git-only'
        }
    }
    foreach ($prefix in $Policy.gitOnlyPrefixes) {
        if ($lower.StartsWith(([string]$prefix).ToLowerInvariant())) {
            return 'git-only'
        }
    }

    foreach ($exact in $Policy.sharedExactPaths) {
        if ($lower -eq ([string]$exact).ToLowerInvariant()) {
            return 'shared'
        }
    }
    $eligibleText = -not $isV2 -or $Policy.allowedSharedExtensions -contains $extension -or
        [IO.Path]::GetFileName($lower) -in @('cmakelists.txt','makefile','license','notice','.gitignore','.gitattributes','.editorconfig','.ignore','.p4ignore')
    if ($segments.Count -eq 1 -and $Policy.sharedRootScriptExtensions -contains $extension) {
        return 'shared'
    }
    foreach ($prefix in $Policy.sharedPrefixes) {
        if ($lower.StartsWith(([string]$prefix).ToLowerInvariant())) {
            if ($eligibleText) { return 'shared' }
            return 'unknown'
        }
    }

    if ($lower.StartsWith('plugins/')) {
        if ($extension -eq '.uplugin') {
            return 'shared'
        }
        foreach ($segment in $Policy.sharedPluginSegments) {
            if ($segments -contains ([string]$segment).ToLowerInvariant()) {
                if ($eligibleText -or ($segment -eq 'Resources' -and $extension -in @('.png','.ico','.svg'))) { return 'shared' }
                return 'unknown'
            }
        }
    }
    return 'unknown'
}

function Get-Dev01SharedFileMap {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)]$Policy
    )
    $resolvedRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    if (-not (Test-Path -LiteralPath $resolvedRoot)) {
        throw "Tree root is missing: $resolvedRoot"
    }
    $map = @{}
    foreach ($file in Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Force -ErrorAction Stop) {
        $relative = ConvertTo-Dev01RelativePath $file.FullName.Substring($resolvedRoot.Length).TrimStart('\')
        if ((Get-Dev01PathClass -Path $relative -Policy $Policy) -eq 'shared') {
            if($file.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse point in shared source; manual review required.'}
            $map[$relative.ToLowerInvariant()] = [pscustomobject]@{
                Relative = $relative
                FullName = $file.FullName
                Length = [int64]$file.Length
            }
        }
    }
    return $map
}

function Compare-Dev01SharedTrees {
    param(
        [Parameter(Mandatory=$true)][string]$SourceRoot,
        [Parameter(Mandatory=$true)][string]$TargetRoot,
        [Parameter(Mandatory=$true)]$Policy
    )
    $sourceMap = Get-Dev01SharedFileMap -Root $SourceRoot -Policy $Policy
    $targetMap = Get-Dev01SharedFileMap -Root $TargetRoot -Policy $Policy
    $sourceOnly = [System.Collections.Generic.List[object]]::new()
    $targetOnly = [System.Collections.Generic.List[object]]::new()
    $different = [System.Collections.Generic.List[object]]::new()

    foreach ($key in $sourceMap.Keys) {
        if (-not $targetMap.ContainsKey($key)) {
            $sourceOnly.Add($sourceMap[$key])
            continue
        }
        $source = $sourceMap[$key]
        $target = $targetMap[$key]
        if ($source.Length -ne $target.Length) {
            $different.Add([pscustomobject]@{ Relative=$source.Relative; Source=$source; Target=$target; Reason='length' })
            continue
        }
        $sourceHash = (Get-FileHash -LiteralPath $source.FullName -Algorithm SHA256).Hash
        $targetHash = (Get-FileHash -LiteralPath $target.FullName -Algorithm SHA256).Hash
        if ($sourceHash -ne $targetHash) {
            $different.Add([pscustomobject]@{ Relative=$source.Relative; Source=$source; Target=$target; Reason='hash' })
        }
    }
    foreach ($key in $targetMap.Keys) {
        if (-not $sourceMap.ContainsKey($key)) {
            $targetOnly.Add($targetMap[$key])
        }
    }
    return [pscustomobject]@{
        SourceMap = $sourceMap
        TargetMap = $targetMap
        SourceOnly = @($sourceOnly)
        TargetOnly = @($targetOnly)
        Different = @($different)
        ChangeCount = $sourceOnly.Count + $targetOnly.Count + $different.Count
    }
}

function Resolve-Dev01SafeChildPath {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$Relative
    )
    $Relative = ConvertTo-Dev01RelativePath $Relative
    $resolvedRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $candidate = [IO.Path]::GetFullPath((Join-Path $resolvedRoot ($Relative -replace '/', '\')))
    if (-not $candidate.StartsWith($resolvedRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path escapes root: $Relative"
    }
    $checkPath = $candidate
    while ($checkPath -and $checkPath.StartsWith($resolvedRoot, [StringComparison]::OrdinalIgnoreCase)) {
        if (Test-Path -LiteralPath $checkPath) {
            if ((Get-Item -LiteralPath $checkPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw 'Reparse point in synchronization target; manual review required.'
            }
        }
        if ($checkPath -eq $resolvedRoot) { break }
        $checkPath = Split-Path -Parent $checkPath
    }
    return $candidate
}

function Set-Dev01FileWritable {
    param([Parameter(Mandatory=$true)][string]$Path)
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        $item = Get-Item -LiteralPath $Path -Force
        if ($item.IsReadOnly) { $item.IsReadOnly = $false }
    }
}

function Apply-Dev01SharedTreeComparison {
    param(
        [Parameter(Mandatory=$true)]$Comparison,
        [Parameter(Mandatory=$true)][string]$TargetRoot,
        [switch]$AllowReviewedDeletes
    )
    if (@($Comparison.TargetOnly).Count -gt 0 -and -not $AllowReviewedDeletes) {
        throw 'Target-only files require an explicit reviewed deletion plan; no files changed.'
    }
    foreach ($entry in $Comparison.TargetOnly) {
        $targetPath = Resolve-Dev01SafeChildPath -Root $TargetRoot -Relative $entry.Relative
        Set-Dev01FileWritable -Path $targetPath
        if (Test-Path -LiteralPath $targetPath -PathType Leaf) {
            Remove-Item -LiteralPath $targetPath -Force
        }
    }
    $copyEntries = @($Comparison.SourceOnly) + @($Comparison.Different | ForEach-Object { $_.Source })
    foreach ($entry in $copyEntries) {
        $targetPath = Resolve-Dev01SafeChildPath -Root $TargetRoot -Relative $entry.Relative
        $parent = Split-Path -Parent $targetPath
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
        Set-Dev01FileWritable -Path $targetPath
        Copy-Item -LiteralPath $entry.FullName -Destination $targetPath -Force
        Set-Dev01FileWritable -Path $targetPath
    }
}

function Enter-Dev01SyncLock {
    param([string]$Path = 'C:\BuildAgent\Dev01\state\code-sync.lock')
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    try {
        $stream = [IO.File]::Open($Path, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    } catch {
        throw "Another Dev01 synchronization is already running. Lock: $Path"
    }
    $stream.SetLength(0)
    $text = "PID=$PID`r`nStarted=$((Get-Date).ToString('o'))`r`n"
    $bytes = [Text.Encoding]::UTF8.GetBytes($text)
    $stream.Write($bytes, 0, $bytes.Length)
    $stream.Flush()
    return $stream
}

function Get-Dev01SyncState {
    param([string]$Path = 'C:\BuildAgent\Dev01\state\code-sync-state.json')
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    return (Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json)
}

function Set-Dev01SyncState {
    param(
        [Parameter(Mandatory=$true)]$State,
        [string]$Path = 'C:\BuildAgent\Dev01\state\code-sync-state.json'
    )
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    $temporary = "$Path.tmp"
    $State | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $temporary -Encoding utf8
    Move-Item -LiteralPath $temporary -Destination $Path -Force
}

function Initialize-Dev01P4BridgeEnvironment {
    $env:P4PORT = 'ssl:localhost:1666'
    $env:P4USER = 'Dev01BuildAgent'
    $env:P4CLIENT = 'bridge_10_0_0_10_Dev01_main'
}

function Assert-Dev01P4BridgeClean {
    param([string]$Root = 'C:\Project\Dev01-BridgeP4')
    Initialize-Dev01P4BridgeEnvironment
    $info = (& p4 info 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw "p4 info failed: $info" }
    if ($info -notmatch [regex]::Escape('Client root: C:\Project\Dev01-BridgeP4')) {
        throw 'P4 bridge client root mismatch.'
    }
    if ($info -notmatch [regex]::Escape('Client stream: //Dev01/main')) {
        throw 'P4 bridge client stream mismatch.'
    }
    $opened = @(& cmd.exe /d /c "p4 opened -C $env:P4CLIENT 2>&1" | Where-Object { $_ -match '^//[^/]+/' })
    if ($opened.Count -gt 0) {
        throw "P4 has opened files; synchronization stopped.`n$($opened -join "`n")"
    }
    $preview = @(& cmd.exe /d /c 'p4 reconcile -n -I -e -a -d //Dev01/main/... 2>&1')
    $changes = @($preview | Where-Object { $_ -match '^//Dev01/main/.+#\d+ - opened for ' })
    if ($changes.Count -gt 0) {
        throw "P4 bridge workspace has local differences; synchronization stopped.`n$($changes[0..([math]::Min(49,$changes.Count-1))] -join "`n")"
    }
}

function Assert-Dev01GitClean {
    param([string]$Repo = 'C:\Project\Dev01-GitHub')
    $status = @(& git -C $Repo status --porcelain=v1 2>&1)
    if ($LASTEXITCODE -ne 0) { throw "git status failed: $($status -join "`n")" }
    if ($status.Count -gt 0) {
        throw "Git workspace is not clean; synchronization stopped.`n$($status -join "`n")"
    }
}

function Invoke-Dev01GitChecked {
    param(
        [Parameter(Mandatory=$true)][string[]]$Arguments,
        [int]$Attempts = 1,
        [string]$Description = 'git command'
    )
    $lastOutput = @()
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        $previousPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            $lastOutput = @(& git @Arguments 2>&1)
            $exitCode = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $previousPreference
        }
        if ($exitCode -eq 0) {
            return $lastOutput
        }
        if ($attempt -lt $Attempts) {
            $delay = [math]::Min(20, 2 * $attempt)
            Write-Warning "$Description failed on attempt $attempt/$Attempts; retrying in $delay seconds."
            Start-Sleep -Seconds $delay
        }
    }
    throw "$Description failed after $Attempts attempts.`n$($lastOutput -join "`n")"
}

function Invoke-Dev01EncodedRead {
    param([Parameter(Mandatory)][string]$Executable,[string]$Arguments='', [int]$CodePage=65001)
    $start=[Diagnostics.ProcessStartInfo]::new()
    $start.FileName=$Executable;$start.Arguments=$Arguments;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
    $start.StandardOutputEncoding=[Text.Encoding]::GetEncoding($CodePage)
    $start.StandardErrorEncoding=[Text.Encoding]::GetEncoding($CodePage)
    $process=[Diagnostics.Process]::Start($start)
    try {
        $outTask=$process.StandardOutput.ReadToEndAsync();$errTask=$process.StandardError.ReadToEndAsync()
        if(-not $process.WaitForExit(45000)){$process.Kill();$process.WaitForExit();throw 'ReadOnlyProbeTimeout'}
        return [pscustomobject]@{ExitCode=$process.ExitCode;Lines=@($outTask.Result -split '\r?\n'|Where-Object {$_});ErrorPresent=($errTask.Result.Length -gt 0)}
    } finally {$process.Dispose()}
}

function Invoke-Dev01P4Read {
    param([Parameter(Mandatory)][string]$Arguments)
    # This non-Unicode Windows P4 depot stores path bytes as CP936, unlike UTF-8 Git.
    Invoke-Dev01EncodedRead -Executable (Get-Command p4.exe -ErrorAction Stop).Source -Arguments $Arguments -CodePage 936
}

function Get-Dev01P4HeadChange {
    Initialize-Dev01P4BridgeEnvironment
    $read=Invoke-Dev01P4Read 'changes -m 1 -s submitted //Dev01/main/...'
    if($read.ExitCode -ne 0){throw 'P4HeadReadFailed'}
    $line = $read.Lines | Select-Object -First 1
    if ($line -notmatch '^Change\s+(\d+)') { throw "Unable to determine P4 head: $line" }
    return [int]$Matches[1]
}

function Get-Dev01P4ChangedPaths {
    param([int]$AfterChange)
    Initialize-Dev01P4BridgeEnvironment
    $head = Get-Dev01P4HeadChange
    if ($AfterChange -ge $head) { return @() }
    $range = "//$('Dev01/main')/...@$($AfterChange + 1),$head"
    $read=Invoke-Dev01P4Read "changes -s submitted $range"
    if($read.ExitCode -ne 0){throw 'P4ChangesReadFailed'}
    $changeLines = @($read.Lines)
    $changes = @($changeLines | ForEach-Object { if ($_ -match '^Change\s+(\d+)') { [int]$Matches[1] } })
    $result = @{}
    foreach ($change in $changes) {
        $read=Invoke-Dev01P4Read "describe -s $change"
        if($read.ExitCode -ne 0){throw 'P4DescriptionReadFailed'}
        $description = @($read.Lines)
        foreach ($line in $description) {
            if ($line -match '^\.\.\.\s+//Dev01/main/(.+)#\d+\s+(add|edit|delete|move/add|move/delete|integrate|branch)') {
                $relative = ConvertTo-Dev01RelativePath $Matches[1]
                # Changes are newest-first. Never replace the current action with an older add/edit.
                if (-not $result.ContainsKey($relative.ToLowerInvariant())) {
                    $result[$relative.ToLowerInvariant()] = [pscustomobject]@{ Relative=$relative; Change=$change; Action=$Matches[2] }
                }
            }
        }
    }
    return @($result.Values)
}

function Get-Dev01GitChangedPaths {
    param(
        [Parameter(Mandatory=$true)][string]$Repo,
        [Parameter(Mandatory=$true)][string]$AfterCommit,
        [string]$ToCommit = 'HEAD'
    )
    if($AfterCommit -cnotmatch '^[a-fA-F0-9]{40}$' -or $ToCommit -cnotmatch '^[a-zA-Z0-9/_-]+$'){throw 'InvalidGitRevision'}
    $git=(Get-Command git.exe -ErrorAction Stop).Source
    $ancestry=Invoke-Dev01EncodedRead -Executable $git -Arguments "-C `"$Repo`" merge-base --is-ancestor $AfterCommit $ToCommit"
    if ($ancestry.ExitCode -ne 0) {
        throw "Git sync base $AfterCommit is not an ancestor of $ToCommit. Manual recovery required."
    }
    $read=Invoke-Dev01EncodedRead -Executable $git -Arguments "-C `"$Repo`" -c core.quotepath=false diff --name-status --no-renames $AfterCommit..$ToCommit"
    if($read.ExitCode -ne 0){throw 'GitChangedPathsReadFailed'}
    $lines = @($read.Lines)
    $result = @{}
    foreach ($line in $lines) {
        $parts = $line -split "`t", 2
        if ($parts.Count -ne 2) { continue }
        $relative = ConvertTo-Dev01RelativePath $parts[1]
        $result[$relative.ToLowerInvariant()] = [pscustomobject]@{ Relative=$relative; Action=$parts[0] }
    }
    return @($result.Values)
}

function Write-Dev01SyncReport {
    param(
        [Parameter(Mandatory=$true)]$Report,
        [Parameter(Mandatory=$true)][string]$Prefix
    )
    $logRoot = 'C:\BuildAgent\Dev01\logs'
    New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
    $path = Join-Path $logRoot ($Prefix + '_' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')
    $Report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $path -Encoding utf8
    return $path
}
