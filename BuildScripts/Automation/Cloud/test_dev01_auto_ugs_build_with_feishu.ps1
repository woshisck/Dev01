[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$wrapperPath = Join-Path $scriptRoot 'dev01_auto_ugs_build_with_feishu.ps1'
$automationScripts = @(
  (Join-Path $scriptRoot 'dev01_auto_ugs_build.ps1')
  (Join-Path $scriptRoot 'dev01_ci_project_build.ps1')
  (Join-Path $scriptRoot 'dev01_publish_ugs_binaries.ps1')
)
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('dev01-feishu-wrapper-test-' + [Guid]::NewGuid().ToString('N'))

function Assert-True {
  param(
    [bool]$Condition,
    [string]$Message
  )

  if (-not $Condition) {
    throw $Message
  }
}

function Assert-PowerShellSyntax {
  param([string]$Path)

  $tokens = $null
  $parseErrors = $null
  [void][Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$parseErrors)
  if ($parseErrors.Count -gt 0) {
    throw "PowerShell syntax errors in $Path`: $($parseErrors.Message -join '; ')"
  }
}

try {
  Assert-PowerShellSyntax -Path $wrapperPath
  foreach ($scriptPath in $automationScripts) {
    Assert-PowerShellSyntax -Path $scriptPath
    $source = Get-Content -LiteralPath $scriptPath -Raw
    Assert-True ($source -match "P4USER\s*=\s*'Dev01BuildAgent'") "Missing Dev01BuildAgent identity in $scriptPath"
    Assert-True ($source -match [regex]::Escape('secrets\Dev01BuildAgent.ticket')) "Missing scoped ticket path in $scriptPath"
    Assert-True ($source -match [regex]::Escape('secrets\p4trust.txt')) "Missing scoped trust path in $scriptPath"
    Assert-True ($source -notmatch "P4USER\s*=\s*'Admin'") "Legacy Admin identity remains in $scriptPath"
  }

  New-Item -ItemType Directory -Path $testRoot | Out-Null
  $fakeAutoPath = Join-Path $testRoot 'fake_auto.ps1'
  $fakeReceiverPath = Join-Path $testRoot 'fake_receiver.ps1'
  $statusPath = Join-Path $testRoot 'status.json'
  $capturePath = Join-Path $testRoot 'payload.json'
  $env:DEV01_WRAPPER_TEST_STATUS = $statusPath
  $env:DEV01_WRAPPER_TEST_CAPTURE = $capturePath

  @'
param([switch]$Force)
[ordered]@{
  timestamp = (Get-Date).ToString('o')
  result = 'success'
  project_cl = 321
  code_cl = 320
  message = 'Local wrapper test completed.'
  log = 'C:\BuildAgent\Dev01\logs\test.log'
} | ConvertTo-Json | Set-Content -LiteralPath $env:DEV01_WRAPPER_TEST_STATUS -Encoding UTF8
exit 0
'@ | Set-Content -LiteralPath $fakeAutoPath -Encoding UTF8

  @'
[Console]::InputEncoding = [Text.UTF8Encoding]::new($false)
$encoded = [Console]::In.ReadToEnd().Trim()
$encoded = $encoded.TrimStart([char]0xFEFF)
$json = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($encoded))
[IO.File]::WriteAllText($env:DEV01_WRAPPER_TEST_CAPTURE, $json, [Text.UTF8Encoding]::new($false))
Write-Output 'QUEUED'
exit 0
'@ | Set-Content -LiteralPath $fakeReceiverPath -Encoding UTF8

  & powershell.exe `
    -NoProfile `
    -NonInteractive `
    -ExecutionPolicy Bypass `
    -File $wrapperPath `
    -Force `
    -AgentRoot $testRoot `
    -AutoBuildScript $fakeAutoPath `
    -StatusPath $statusPath `
    -ReceiverScript $fakeReceiverPath `
    -PowerShellExecutable 'powershell.exe'
  $wrapperExitCode = $LASTEXITCODE

  Assert-True ($wrapperExitCode -eq 0) "Wrapper returned unexpected exit code $wrapperExitCode"
  Assert-True (Test-Path -LiteralPath $capturePath) 'Notification payload was not captured.'

  $payload = Get-Content -LiteralPath $capturePath -Raw | ConvertFrom-Json
  Assert-True ($payload.version -eq 1) 'Payload version is invalid.'
  Assert-True ($payload.kind -eq 'codex.stop') 'Payload kind is invalid.'
  Assert-True ([string]$payload.message -match 'Result: success \(exit 0\)') 'Payload has no success result.'
  Assert-True ([string]$payload.message -match 'ProjectCL: 321') 'Payload has no ProjectCL.'
  Assert-True ([string]$payload.message -match 'CodeCL: 320') 'Payload has no CodeCL.'
  Assert-True ([string]$payload.message -match 'Duration: \d{2}:\d{2}:\d{2}') 'Payload has no duration.'
  Assert-True ([string]$payload.message -match [regex]::Escape('Log: C:\BuildAgent\Dev01\logs\test.log')) 'Payload has no log path.'
  Assert-True (($payload | ConvertTo-Json -Compress) -notmatch '(?i)password|token|app_secret|p4tickets') 'Payload contains a secret-like field.'

  $fakeSkipPath = Join-Path $testRoot 'fake_skip.ps1'
  @'
[ordered]@{
  timestamp = (Get-Date).ToString('o')
  result = 'skipped'
  project_cl = 321
  code_cl = 320
  message = 'No unpublished code change.'
  log = 'C:\BuildAgent\Dev01\logs\skip.log'
} | ConvertTo-Json | Set-Content -LiteralPath $env:DEV01_WRAPPER_TEST_STATUS -Encoding UTF8
exit 0
'@ | Set-Content -LiteralPath $fakeSkipPath -Encoding UTF8
  Remove-Item -LiteralPath $capturePath -Force
  $skipOutput = & powershell.exe `
    -NoProfile `
    -NonInteractive `
    -ExecutionPolicy Bypass `
    -File $wrapperPath `
    -AgentRoot $testRoot `
    -AutoBuildScript $fakeSkipPath `
    -StatusPath $statusPath `
    -ReceiverScript $fakeReceiverPath `
    -PowerShellExecutable 'powershell.exe'
  $skipExitCode = $LASTEXITCODE
  Assert-True ($skipExitCode -eq 0) "Skipped build returned unexpected exit code $skipExitCode"
  Assert-True ($skipOutput -contains 'FEISHU_NOTIFICATION_SKIPPED_NO_BUILD=1') 'Skipped build did not report notification suppression.'
  Assert-True (-not (Test-Path -LiteralPath $capturePath)) 'Skipped build incorrectly queued a notification.'

  $fakeFailurePath = Join-Path $testRoot 'fake_failure.ps1'
  @'
exit 7
'@ | Set-Content -LiteralPath $fakeFailurePath -Encoding UTF8
  Remove-Item -LiteralPath $statusPath -Force
  & powershell.exe `
    -NoProfile `
    -NonInteractive `
    -ExecutionPolicy Bypass `
    -File $wrapperPath `
    -AgentRoot $testRoot `
    -AutoBuildScript $fakeFailurePath `
    -StatusPath $statusPath `
    -ReceiverScript (Join-Path $testRoot 'missing_receiver.ps1') `
    -PowerShellExecutable 'powershell.exe'
  $failureExitCode = $LASTEXITCODE
  Assert-True ($failureExitCode -eq 7) "Notification failure masked build exit code; expected 7, got $failureExitCode"

  Write-Output 'DEV01_FEISHU_WRAPPER_TEST_OK=1'
} finally {
  Remove-Item Env:DEV01_WRAPPER_TEST_STATUS -ErrorAction SilentlyContinue
  Remove-Item Env:DEV01_WRAPPER_TEST_CAPTURE -ErrorAction SilentlyContinue
  $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
  $resolvedTest = [IO.Path]::GetFullPath($testRoot).TrimEnd('\')
  if ($resolvedTest.StartsWith($resolvedTemp + '\dev01-feishu-wrapper-test-', [StringComparison]::OrdinalIgnoreCase) -and [IO.Directory]::Exists($resolvedTest)) {
    [IO.Directory]::Delete($resolvedTest, $true)
  }
}
