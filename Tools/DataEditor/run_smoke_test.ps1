param(
    [switch]$CloseEditor
)

# Read-only smoke test for DataEditor toolkit.
# By default this refuses to run while an interactive UnrealEditor is open.
# Pass -CloseEditor only when you intentionally want this wrapper to close it.

. "$PSScriptRoot/_common.ps1"

$Script = Join-Path $PSScriptRoot "smoke_test.py"
$code = Invoke-UEPython -ScriptPath $Script -CloseEditor:$CloseEditor

# Show latest report file if any
$reportRoot = Join-Path $ProjectRoot "Saved/Balance"
if (Test-Path $reportRoot) {
    $latest = Get-ChildItem $reportRoot -Directory -Filter "SmokeTest_*" |
              Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latest) {
        $report = Join-Path $latest.FullName "report.txt"
        if (Test-Path $report) {
            Write-Host ""
            Write-Host "===== Smoke Test Report ($($latest.Name)) =====" -ForegroundColor Cyan
            Get-Content $report
        }
    }
}

exit $code
