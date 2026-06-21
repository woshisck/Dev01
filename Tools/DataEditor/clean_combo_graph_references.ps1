# Clear legacy ComboGraph references from selected project assets.
#
# Usage:
#   pwsh Tools/DataEditor/clean_combo_graph_references.ps1 -DryRun
#   pwsh Tools/DataEditor/clean_combo_graph_references.ps1 -Apply

param(
    [switch]$Apply,
    [switch]$DryRun,
    [switch]$CloseEditor
)

. "$PSScriptRoot/_common.ps1"

if ($Apply -and $DryRun) {
    throw "Use only one of -Apply or -DryRun."
}

$Script = Join-Path $PSScriptRoot "clean_combo_graph_references.py"
$args = @()
if ($Apply) {
    $args += "--apply"
}
else {
    $args += "--dry-run"
}

$code = Invoke-UEPython -ScriptPath $Script -ScriptArgs $args -CloseEditor:$CloseEditor

if ($code -eq 0) {
    Write-Host ""
    if ($Apply) {
        Write-Host "ComboGraph reference cleanup applied. Review binary asset diffs before committing." -ForegroundColor Yellow
    }
    else {
        Write-Host "Dry run complete. Re-run with -Apply to save modified assets." -ForegroundColor Yellow
    }
}

exit $code
