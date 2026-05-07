# Two-phase RuneID -> RuneIdTag migration runner.
#
# Phase 1: writes Config/Tags/RuneIDs.ini, no asset modification.
# (between phases) RESTART THE EDITOR so GameplayTagsManager picks up new tags.
# Phase 2: applies the tags to RuneDA assets and saves modified packages.
#
# Usage:
#   pwsh Tools/DataEditor/run_migration.ps1 -Phase prepare
#   # close editor, restart manually, then:
#   pwsh Tools/DataEditor/run_migration.ps1 -Phase apply

param(
    [Parameter(Mandatory)]
    [ValidateSet('prepare', 'apply')]
    [string]$Phase,

    [switch]$CloseEditor
)

. "$PSScriptRoot/_common.ps1"

$Script = Join-Path $PSScriptRoot "run_migration.py"
$code = Invoke-UEPython -ScriptPath $Script -ScriptArgs @("--phase", $Phase) -CloseEditor:$CloseEditor

if ($Phase -eq 'prepare' -and $code -eq 0) {
    Write-Host ""
    Write-Host "Phase 1 done. RESTART the editor manually, then run:" -ForegroundColor Yellow
    Write-Host "  pwsh Tools/DataEditor/run_migration.ps1 -Phase apply" -ForegroundColor Yellow
}

if ($Phase -eq 'apply' -and $code -eq 0) {
    Write-Host ""
    Write-Host "Phase 2 done. Modified RuneDA assets were saved by the headless script." -ForegroundColor Yellow
    Write-Host "Review the asset diff in editor or source control. Use git restore for rollback after save." -ForegroundColor Yellow
}

exit $code
