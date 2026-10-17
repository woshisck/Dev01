[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$Archive,
    [Parameter(Mandatory)][string]$ExpectedMD5,
    [switch]$Apply
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath $Project).Path.TrimEnd('\')
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
if (!(Test-Path -LiteralPath "$projectRoot\DevKit.uproject")) { throw 'Not a Dev01 project.' }
if ((Get-FileHash -LiteralPath $archivePath -Algorithm MD5).Hash -ne $ExpectedMD5) { throw 'PCB digest differs from P4.' }
$editors = @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe'")
if ($editors | Where-Object { !$_.CommandLine -or $_.CommandLine.Replace('/', '\').Contains($projectRoot) }) { throw 'Close this project editor before restoring binaries.' }
$engineId = (Get-Content -LiteralPath "$projectRoot\Engine\Binaries\Win64\UnrealEditor.modules" -Raw | ConvertFrom-Json).BuildId
if (!$engineId) { throw 'No installed Engine BuildId.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $files = @($zip.Entries | Where-Object { $_.Name })
    $seen = @{}
    foreach ($entry in $files) {
        $relative = $entry.FullName.Replace('\', '/')
        if ($relative.Contains(':') -or $relative -notmatch '^(Binaries/Win64/|Plugins/[^:]+/Binaries/Win64/)[^/]+\.(dll|pdb|modules|target)$' -or $relative.Split('/') -contains '..' -or $seen.ContainsKey($relative)) { throw "Unsafe or duplicate PCB path: $relative" }
        $seen[$relative] = $true
        $target = [IO.Path]::GetFullPath((Join-Path $projectRoot $relative))
        if (!$target.StartsWith($projectRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Path outside project.' }
        $ancestor = $target
        while ($ancestor -ne $projectRoot) {
            if (Test-Path -LiteralPath $ancestor) {
                if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse point: $ancestor" }
            }
            $ancestor = Split-Path -Parent $ancestor
        }
        if ($relative -match '\.(modules|target)$') {
            $reader = [IO.StreamReader]::new($entry.Open())
            try { $metadata = $reader.ReadToEnd() | ConvertFrom-Json } finally { $reader.Dispose() }
            $id = if ($relative.EndsWith('.target')) { $metadata.Version.BuildId } else { $metadata.BuildId }
            if ($id -ne $engineId) { throw "PCB BuildId mismatch: $relative" }
        }
    }
    foreach ($required in @('Binaries/Win64/UnrealEditor.modules', 'Binaries/Win64/DevKitEditor.target', 'Binaries/Win64/UnrealEditor-DevKitEditor.dll')) {
        if (!$seen.ContainsKey($required)) { throw "Missing required file: $required" }
    }
    Write-Output "Validated $($files.Count) binary files; Engine BuildId=$engineId; Apply=$Apply"
    if (!$Apply) { return }
    $recovery = Join-Path $projectRoot ('Saved\WorkflowHub\Recovery\Restore-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
    if (Test-Path -LiteralPath $recovery) { throw 'Recovery directory already exists.' }
    New-Item -ItemType Directory -Path $recovery | Out-Null
    $records = @()
    # Stage and hash every output before overwriting any live binary.
    foreach ($entry in $files) {
        $relative = $entry.FullName.Replace('/', '\')
        $staged = Join-Path "$recovery\Staged" $relative
        $target = Join-Path $projectRoot $relative
        $backup = Join-Path "$recovery\Before" $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $staged) -Force | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $staged, $false)
        $before = $null
        if (Test-Path -LiteralPath $target) {
            New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
            Copy-Item -LiteralPath $target -Destination $backup
            $before = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
            if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -ne $before) { throw 'Backup verification failed.' }
        }
        $records += [pscustomobject]@{path=$relative; beforeSHA256=$before; afterSHA256=(Get-FileHash -LiteralPath $staged -Algorithm SHA256).Hash; restored=$false}
    }
    $journal = [ordered]@{project=$projectRoot; archive=$archivePath; archiveMD5=$ExpectedMD5; engineBuildId=$engineId; entries=$records}
    $manifest = Join-Path $recovery 'manifest.json'
    $journal | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8
    foreach ($record in $records) {
        $target = Join-Path $projectRoot $record.path
        if ($record.beforeSHA256 -and (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $record.beforeSHA256) { throw "Live file changed: $target" }
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path "$recovery\Staged" $record.path) -Destination $target -Force
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $record.afterSHA256) { throw "Restored file verification failed: $target" }
        $record.restored = $true
        $journal | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8
    }
    Write-Output "Restored $($records.Count) files. Recoverable previous binaries and hash journal: $manifest"
} finally { $zip.Dispose() }
