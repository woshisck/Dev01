# Shared, side-effect-free build provenance helpers. Dot-source from the build,
# publisher and tests; never infer the built revision from depot head at publish time.
function Invoke-Dev01P4Capture([string[]]$Arguments) {
  $previousPreference = $ErrorActionPreference
  try {
    # Windows PowerShell 5.1 turns native stderr into terminating errors under
    # Stop, even for expected "not opened" responses. Interpret exit + text here.
    $ErrorActionPreference = 'Continue'
    $output = @(& p4 @Arguments 2>&1 | ForEach-Object { [string]$_ })
    $code = $LASTEXITCODE
  } finally { $ErrorActionPreference = $previousPreference }
  return [pscustomobject]@{ ExitCode = $code; Lines = $output }
}

function Enter-Dev01PipelineLease([IO.FileStream]$ExistingLease) {
  $path = 'C:\BuildAgent\Dev01\state\auto_ugs_build.lock'
  if ($ExistingLease) {
    if (-not $ExistingLease.CanWrite -or $ExistingLease.Name -ine $path) { throw 'Invalid inherited pipeline lease.' }
    return [pscustomobject]@{ Stream = $ExistingLease; Owned = $false }
  }
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $path) | Out-Null
  try { $stream = [IO.File]::Open($path, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None) }
  catch [IO.IOException] { throw 'Another Dev01 build or publisher holds the pipeline lease.' }
  return [pscustomobject]@{ Stream = $stream; Owned = $true }
}

function Get-Dev01TextHash([string[]]$Lines) {
  $sha = [Security.Cryptography.SHA256]::Create()
  try {
    $bytes = [Text.Encoding]::UTF8.GetBytes(($Lines -join "`n"))
    return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
  } finally { $sha.Dispose() }
}

function Get-Dev01CodeChange([int]$ProjectChange, [string]$BuildClient) {
  if ($ProjectChange -le 0) { throw 'A submitted project changelist is required.' }
  $extensions = @('.c', '.cc', '.cpp', '.inl', '.m', '.mm', '.rc', '.cs', '.csproj', '.h', '.hpp', '.usf', '.ush', '.uproject', '.uplugin', '.sln', '.native.verse')
  $changes = @(& p4 -c $BuildClient changes -s submitted -m 2048 "//$BuildClient/...@1,$ProjectChange")
  if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate code changelists.' }
  foreach ($entry in $changes) {
    if ($entry -notmatch '^Change\s+(\d+)') { continue }
    $candidate = [int]$Matches[1]
    $description = @(& p4 -c $BuildClient describe -s $candidate)
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect CL $candidate." }
    foreach ($line in $description) {
      if ($line -match '^\.\.\.\s+(//\S+)#\d+\s+') {
        $file = $Matches[1]
        if ($extensions | Where-Object { $file.EndsWith($_, [StringComparison]::OrdinalIgnoreCase) }) { return $candidate }
      }
    }
  }
  throw "Cannot resolve CodeCL at project CL $ProjectChange."
}

function Get-Dev01WorkspaceStamp([string]$BuildClient) {
  $opened = Invoke-Dev01P4Capture @('-c', $BuildClient, '-ztag', 'opened', "//$BuildClient/...")
  if (($opened.Lines -join "`n") -match 'depotFile') { throw 'Build workspace contains opened files; refusing publication.' }
  if ($opened.ExitCode -ne 0 -and ($opened.Lines -join "`n") -notmatch 'not opened|no file') { throw "Cannot inspect opened files: $($opened.Lines)" }
  $have = @(& p4 -c $BuildClient have "//$BuildClient/...")
  if ($LASTEXITCODE -ne 0 -or $have.Count -eq 0) { throw 'Cannot fingerprint build workspace have-list.' }
  $stream = @(& p4 stream -o //Dev01/main)
  if ($LASTEXITCODE -ne 0) { throw 'Cannot read the project stream.' }
  $imports = @($stream | Where-Object { $_ -match '^\s*import\s+Engine/\.\.\.\s+//Dev01Engine/main/\.\.\.@(\d+)\s*$' })
  if ($imports.Count -ne 1) { throw 'Engine import must pin one explicit submitted changelist.' }
  return Get-Dev01TextHash (@($have | Sort-Object) + $imports)
}

function Assert-Dev01WorkspaceInputs([string]$BuildClient, [int]$ProjectChange, [int]$EngineChange) {
  if ($ProjectChange -le 0 -or $EngineChange -le 0) { throw 'Explicit project and Engine snapshots are required.' }
  # A stable have-list alone is insufficient on an allwrite build workspace.
  foreach ($spec in @("//Dev01/main/...@$ProjectChange", "//Dev01Engine/main/...@$EngineChange")) {
    $preview = Invoke-Dev01P4Capture @('-c', $BuildClient, 'sync', '-n', $spec)
    $text = $preview.Lines -join "`n"
    if ($preview.ExitCode -ne 0 -and $text -notmatch 'up-to-date') { throw "Cannot verify pinned have-list: $text" }
    $unexpected = @($preview.Lines | Where-Object { $_.Trim() -and $_ -notmatch 'file\(s\) up-to-date\.' })
    if ($unexpected.Count -gt 0) { throw "Workspace does not have the complete pinned snapshot: $text" }
  }
  foreach ($mode in @('-se', '-sd')) {
    $diff = Invoke-Dev01P4Capture @('-c', $BuildClient, 'diff', $mode, "//$BuildClient/...")
    $text = $diff.Lines -join "`n"
    if ($diff.ExitCode -ne 0 -and $text -notmatch 'no such file|not opened') { throw "Cannot validate workspace input bytes: $text" }
    # diff -se/-sd normally prints LOCAL filesystem paths, not depot paths.
    # Only the explicit empty-result message may be ignored; fail closed on
    # every other output so allwrite modifications cannot pass certification.
    $unexpected = @($diff.Lines | Where-Object { $_.Trim() -and $_ -notmatch 'no such file|not opened' })
    if ($unexpected.Count -gt 0) { throw "Modified or missing tracked build inputs ($mode): $text" }
  }
}

function Get-Dev01PcbInventory([string]$ProjectRoot) {
  $root = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
  $dirs = @()
  $projectBin = Join-Path $root 'Binaries\Win64'
  if (Test-Path -LiteralPath $projectBin) { $dirs += $projectBin }
  $plugins = Join-Path $root 'Plugins'
  if (Test-Path -LiteralPath $plugins) {
    $dirs += @(Get-ChildItem -LiteralPath $plugins -Directory -Recurse | Where-Object {
      $_.Name -eq 'Win64' -and $_.Parent.Name -eq 'Binaries'
    } | Select-Object -ExpandProperty FullName)
  }
  $inventory = @(foreach ($dir in $dirs) {
    foreach ($file in Get-ChildItem -LiteralPath $dir -File -Recurse) {
      $relative = $file.FullName.Substring($root.Length).Replace('\', '/')
      [pscustomobject]@{ path = $relative; length = $file.Length; sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
  })
  if ($inventory.Count -eq 0) { throw 'No PCB output files to fingerprint.' }
  return @($inventory | Sort-Object path)
}

function Get-Dev01InventoryHash([object[]]$Inventory) {
  return Get-Dev01TextHash @($Inventory | Sort-Object path | ForEach-Object { '{0}|{1}|{2}' -f $_.path, $_.length, $_.sha256 })
}

function Assert-Dev01BuildReceipt($Receipt, [int]$ProjectChange, [int]$CodeChange, [string]$BuildId, [string]$WorkspaceStamp, [object[]]$Inventory) {
  if ($Receipt.schema -ne 1 -or $Receipt.status -ne 'complete') { throw 'Missing completed build receipt; rebuild before publishing.' }
  if ($Receipt.project_cl -le 0 -or $Receipt.code_cl -le 0 -or $Receipt.code_cl -gt $Receipt.project_cl) { throw 'Invalid changelists in build receipt.' }
  if ($ProjectChange -gt 0 -and $Receipt.project_cl -ne $ProjectChange) { throw 'ProjectCL differs from the completed build.' }
  if ($CodeChange -gt 0 -and $Receipt.code_cl -ne $CodeChange) { throw 'CodeCL differs from the completed build.' }
  if ($Receipt.engine_build_id -ne $BuildId) { throw 'Engine BuildId differs from the completed build.' }
  if ($Receipt.workspace_stamp -ne $WorkspaceStamp) { throw 'Workspace revisions or Engine import changed after build.' }
  if ($Receipt.output_hash -ne (Get-Dev01InventoryHash $Inventory)) { throw 'PCB output was changed, added or removed after build.' }
}

function Write-Dev01BuildReceipt([string]$Path, $Receipt) {
  $parent = Split-Path -Parent $Path
  New-Item -ItemType Directory -Force -Path $parent | Out-Null
  $temp = $Path + '.' + [Guid]::NewGuid().ToString('N') + '.tmp'
  [IO.File]::WriteAllText($temp, ($Receipt | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
  Move-Item -LiteralPath $temp -Destination $Path -Force
}
