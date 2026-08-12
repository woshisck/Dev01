[CmdletBinding()]
param(
  [switch]$Force,
  [string]$AgentRoot = 'C:\BuildAgent\Dev01',
  [string]$AutoBuildScript = 'C:\BuildAgent\Dev01\dev01_auto_ugs_build.ps1',
  [string]$StatusPath = 'C:\BuildAgent\Dev01\state\auto_ugs_build_status.json',
  [string]$ReceiverScript = 'C:\CodexBridge\scripts\Receive-DesktopNotification.ps1',
  [string]$PowerShellExecutable = 'powershell.exe'
)

$ErrorActionPreference = 'Stop'
$startedAt = Get-Date
$buildExitCode = 1
$buildInvocationError = $null
$statusNotBeforeUtc = [DateTime]::UtcNow

function Get-StatusValue {
  param(
    [object]$Status,
    [string]$PropertyName,
    [object]$Fallback
  )

  if ($null -ne $Status -and $Status.PSObject.Properties.Name -contains $PropertyName) {
    $value = $Status.$PropertyName
    if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
      return $value
    }
  }
  return $Fallback
}

function New-CompletionPayload {
  param(
    [int]$ExitCode,
    [object]$Status,
    [TimeSpan]$Elapsed,
    [string]$InvocationError
  )

  $statusResult = [string](Get-StatusValue -Status $Status -PropertyName 'result' -Fallback 'unknown')
  $projectCL = [int](Get-StatusValue -Status $Status -PropertyName 'project_cl' -Fallback 0)
  $codeCL = [int](Get-StatusValue -Status $Status -PropertyName 'code_cl' -Fallback 0)
  $logPath = [string](Get-StatusValue -Status $Status -PropertyName 'log' -Fallback '(unavailable)')
  $statusMessage = [string](Get-StatusValue -Status $Status -PropertyName 'message' -Fallback '')
  $succeeded = $ExitCode -eq 0 -and $statusResult -in @('success', 'skipped')
  $headline = if ($succeeded) { 'Dev01 UGS build and publish completed' } else { 'Dev01 UGS build or publish failed' }
  $displayResult = if ($statusResult -eq 'unknown') {
    if ($ExitCode -eq 0) { 'success' } else { 'failed' }
  } else {
    $statusResult
  }
  $duration = '{0:hh\:mm\:ss}' -f $Elapsed

  $messageLines = @(
    $headline
    "Result: $displayResult (exit $ExitCode)"
    "ProjectCL: $projectCL"
    "CodeCL: $codeCL"
    "Duration: $duration"
    "Log: $logPath"
  )
  if (-not [string]::IsNullOrWhiteSpace($statusMessage)) {
    $messageLines += "Detail: $statusMessage"
  }
  if (-not [string]::IsNullOrWhiteSpace($InvocationError)) {
    $messageLines += "Invocation: $InvocationError"
  }

  return [ordered]@{
    version = 1
    kind = 'codex.stop'
    createdAt = [DateTimeOffset]::UtcNow.ToString('o')
    cwd = $AgentRoot
    message = ($messageLines -join "`n")
    sessionId = 'dev01-cloud-ugs-build'
    turnId = [Guid]::NewGuid().ToString('N')
  }
}

try {
  if (-not (Test-Path -LiteralPath $AutoBuildScript)) {
    throw "Auto UGS build script is missing: $AutoBuildScript"
  }

  $childArguments = @(
    '-NoProfile'
    '-NonInteractive'
    '-ExecutionPolicy'
    'Bypass'
    '-File'
    $AutoBuildScript
  )
  if ($Force) {
    $childArguments += '-Force'
  }

  & $PowerShellExecutable @childArguments
  $buildExitCode = $LASTEXITCODE
} catch {
  $buildExitCode = 1
  $buildInvocationError = $_.Exception.Message
  Write-Error $buildInvocationError -ErrorAction Continue
} finally {
  $status = $null
  if (Test-Path -LiteralPath $StatusPath) {
    try {
      $statusFile = Get-Item -LiteralPath $StatusPath
      if ($statusFile.LastWriteTimeUtc -ge $statusNotBeforeUtc.AddSeconds(-2)) {
        $status = Get-Content -LiteralPath $StatusPath -Raw | ConvertFrom-Json
      } else {
        Write-Warning "Ignoring stale build status JSON: $StatusPath"
      }
    } catch {
      Write-Warning "Unable to read build status JSON: $($_.Exception.Message)"
    }
  }

  $statusResult = [string](Get-StatusValue -Status $status -PropertyName 'result' -Fallback 'unknown')
  $shouldNotify = -not ($buildExitCode -eq 0 -and $statusResult -eq 'skipped')
  if (-not $shouldNotify) {
    Write-Output 'FEISHU_NOTIFICATION_SKIPPED_NO_BUILD=1'
  } else {
    $payload = New-CompletionPayload `
      -ExitCode $buildExitCode `
      -Status $status `
      -Elapsed ((Get-Date) - $startedAt) `
      -InvocationError $buildInvocationError

    try {
      if (-not (Test-Path -LiteralPath $ReceiverScript)) {
        throw "Feishu notification receiver is missing: $ReceiverScript"
      }

    $payloadJson = $payload | ConvertTo-Json -Compress -Depth 4
    $payloadBytes = [Text.Encoding]::UTF8.GetBytes($payloadJson)
    try {
      $encodedPayload = [Convert]::ToBase64String($payloadBytes)
      $receiverProcessInfo = [Diagnostics.ProcessStartInfo]::new()
      $receiverProcessInfo.FileName = $PowerShellExecutable
      $receiverProcessInfo.Arguments = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File `"$ReceiverScript`""
      $receiverProcessInfo.UseShellExecute = $false
      $receiverProcessInfo.RedirectStandardInput = $true
      $receiverProcessInfo.RedirectStandardOutput = $true
      $receiverProcessInfo.RedirectStandardError = $true
      $receiverProcessInfo.StandardOutputEncoding = [Text.UTF8Encoding]::new($false)
      $receiverProcessInfo.StandardErrorEncoding = [Text.UTF8Encoding]::new($false)
      $receiverProcess = [Diagnostics.Process]::Start($receiverProcessInfo)
      $stdinBytes = [Text.Encoding]::ASCII.GetBytes($encodedPayload)
      $receiverProcess.StandardInput.BaseStream.Write($stdinBytes, 0, $stdinBytes.Length)
      $receiverProcess.StandardInput.BaseStream.Flush()
      $receiverProcess.StandardInput.Close()
      $receiverStdout = $receiverProcess.StandardOutput.ReadToEnd()
      $receiverStderr = $receiverProcess.StandardError.ReadToEnd()
      $receiverProcess.WaitForExit()
      $receiverOutput = @($receiverStdout.Trim(), $receiverStderr.Trim()) | Where-Object { $_ }
      $receiverExitCode = $receiverProcess.ExitCode
      [Array]::Clear($stdinBytes, 0, $stdinBytes.Length)
      $receiverOutput | ForEach-Object { Write-Output $_ }
      if ($receiverExitCode -ne 0) {
        throw "Feishu notification receiver exited with code $receiverExitCode."
      }
    } finally {
      if ($payloadBytes) {
        [Array]::Clear($payloadBytes, 0, $payloadBytes.Length)
      }
    }
    } catch {
      Write-Warning "Build completed with exit code $buildExitCode, but Feishu notification enqueue failed: $($_.Exception.Message)"
    }
  }
}

exit $buildExitCode
