param(
  [switch]$Force
)

$ErrorActionPreference = 'Stop'
$agentRoot = 'C:\BuildAgent\Dev01'
$logRoot = Join-Path $agentRoot 'logs'
$stateRoot = Join-Path $agentRoot 'state'
$lockPath = Join-Path $stateRoot 'auto_ugs_build.lock'
$statusPath = Join-Path $stateRoot 'auto_ugs_build_status.json'
$buildScript = Join-Path $agentRoot 'dev01_ci_project_build.ps1'
$publishScript = Join-Path $agentRoot 'dev01_publish_ugs_binaries.ps1'
$buildClient = 'build_10_0_0_10_Dev01_main'
$archivePath = '//Dev01Binaries/UGS/++Dev01+main-Editor.zip'

New-Item -ItemType Directory -Force -Path $logRoot, $stateRoot | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$transcriptPath = Join-Path $logRoot "auto_ugs_build_$stamp.log"
$lockStream = $null
$transcriptStarted = $false

function Write-Status {
  param(
    [string]$Result,
    [int]$ProjectCL = 0,
    [int]$CodeCL = 0,
    [string]$Message = ''
  )

  [ordered]@{
    timestamp = (Get-Date).ToString('o')
    result = $Result
    project_cl = $ProjectCL
    code_cl = $CodeCL
    message = $Message
    log = $transcriptPath
  } | ConvertTo-Json | Set-Content -LiteralPath $statusPath -Encoding UTF8
}

try {
  try {
    $lockStream = [System.IO.File]::Open(
      $lockPath,
      [System.IO.FileMode]::OpenOrCreate,
      [System.IO.FileAccess]::ReadWrite,
      [System.IO.FileShare]::None
    )
  } catch [System.IO.IOException] {
    Write-Output 'SKIP_BUILD_ALREADY_RUNNING=1'
    exit 0
  }

  Start-Transcript -LiteralPath $transcriptPath -Force | Out-Null
  $transcriptStarted = $true

  $env:Path = 'C:\Program Files\Perforce;' + $env:Path
  $env:P4PORT = 'ssl:localhost:1666'
  $env:P4USER = 'Admin'
  $env:P4CLIENT = $buildClient
  $env:P4TICKETS = Join-Path $agentRoot '.p4tickets'
  $env:P4TRUST = Join-Path $agentRoot 'p4trust.txt'
  $env:P4IGNORE = '.p4ignore'

  foreach ($requiredPath in @($buildScript, $publishScript, $env:P4TICKETS, $env:P4TRUST)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
      throw "Required automation input is missing: $requiredPath"
    }
  }

  $loginStatus = & p4 login -s 2>&1
  if ($LASTEXITCODE -ne 0) {
    throw "Perforce login is not valid: $($loginStatus -join ' ')"
  }

  $changeLine = & p4 changes -s submitted -m 1 //Dev01/main/...
  if ($LASTEXITCODE -ne 0 -or $changeLine -notmatch '^Change\s+(\d+)') {
    throw "Cannot determine latest project changelist: $changeLine"
  }
  $projectChange = [int]$Matches[1]

  $codeExtensions = @(
    '.c', '.cc', '.cpp', '.inl', '.m', '.mm', '.rc', '.cs', '.csproj',
    '.h', '.hpp', '.usf', '.ush', '.uproject', '.uplugin', '.sln',
    '.native.verse'
  )
  $visibleChanges = & p4 changes -s submitted -m 2048 "//$buildClient/...@1,$projectChange"
  if ($LASTEXITCODE -ne 0) {
    throw 'Cannot enumerate changelists visible to the cloud build workspace.'
  }

  $codeChange = 0
  foreach ($changeEntry in $visibleChanges) {
    if ($changeEntry -notmatch '^Change\s+(\d+)') {
      continue
    }

    $candidateChange = [int]$Matches[1]
    $describe = & p4 describe -s $candidateChange
    if ($LASTEXITCODE -ne 0) {
      throw "Cannot inspect changelist $candidateChange."
    }

    $containsCode = $false
    foreach ($describeLine in $describe) {
      if ($describeLine -match '^\.\.\.\s+(//\S+)#\d+\s+') {
        $depotFile = $Matches[1]
        if ($codeExtensions | Where-Object { $depotFile.EndsWith($_, [StringComparison]::OrdinalIgnoreCase) }) {
          $containsCode = $true
          break
        }
      }
    }

    if ($containsCode) {
      $codeChange = $candidateChange
      break
    }
  }

  if ($codeChange -le 0) {
    throw "Cannot determine the last code changelist at or before project CL $projectChange."
  }

  $lastCodeState = Join-Path $stateRoot 'last_code_archive_cl.txt'
  $lastPublishedCode = 0
  if (Test-Path -LiteralPath $lastCodeState) {
    [void][int]::TryParse((Get-Content -LiteralPath $lastCodeState -Raw).Trim(), [ref]$lastPublishedCode)
  }

  Write-Output "LATEST_PROJECT_CL=$projectChange"
  Write-Output "LATEST_CODE_CL=$codeChange"
  Write-Output "LAST_PUBLISHED_CODE_CL=$lastPublishedCode"

  if (-not $Force -and $codeChange -le $lastPublishedCode) {
    $message = "No unpublished code change; PCB for code CL $lastPublishedCode is current."
    Write-Output "SKIP_NO_NEW_CODE=$codeChange"
    Write-Status -Result 'skipped' -ProjectCL $projectChange -CodeCL $codeChange -Message $message
    exit 0
  }

  Write-Output "BUILD_START_CODE_CL=$codeChange"
  & $buildScript -Force
  if ($LASTEXITCODE -ne 0) {
    throw "Cloud DevKitEditor build failed with exit code $LASTEXITCODE."
  }

  $targetPath = 'C:\Project\Dev01-P4\Binaries\Win64\DevKitEditor.target'
  if (-not (Test-Path -LiteralPath $targetPath)) {
    throw "Cloud build did not produce $targetPath"
  }

  Write-Output "PUBLISH_START_CODE_CL=$codeChange"
  & $publishScript -Submit
  if ($LASTEXITCODE -ne 0) {
    throw "UGS PCB publishing failed with exit code $LASTEXITCODE."
  }

  $verifyOutput = & p4 verify -q $archivePath
  if ($LASTEXITCODE -ne 0 -or $verifyOutput) {
    throw "Final P4 archive verification failed: $($verifyOutput -join ' ')"
  }

  $publishedCode = 0
  if (Test-Path -LiteralPath $lastCodeState) {
    [void][int]::TryParse((Get-Content -LiteralPath $lastCodeState -Raw).Trim(), [ref]$publishedCode)
  }
  if ($publishedCode -ne $codeChange) {
    throw "Published PCB state is $publishedCode but expected code CL $codeChange."
  }

  Write-Output "AUTO_UGS_BUILD_OK=$codeChange"
  Write-Status -Result 'success' -ProjectCL $projectChange -CodeCL $codeChange -Message 'Cloud build and PCB publish succeeded.'
} catch {
  $message = $_.Exception.Message
  Write-Error $message
  Write-Status -Result 'failed' -Message $message
  exit 1
} finally {
  if ($transcriptStarted) {
    Stop-Transcript | Out-Null
  }
  if ($lockStream) {
    $lockStream.Dispose()
  }
}
