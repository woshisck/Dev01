# Batch operation runner for RuneDA / EffectDA.
#
# Usage:
#   pwsh Tools/DataEditor/batch_ops.ps1 -Op set_rune_gold -Value 99
#   pwsh Tools/DataEditor/batch_ops.ps1 -Op set_effect_duration -Value 5.0
#   pwsh Tools/DataEditor/batch_ops.ps1 -Op set_effect_maxstack -Value 3
#   pwsh Tools/DataEditor/batch_ops.ps1 -Op set_rune_rarity -Value Rare

param(
    [Parameter(Mandatory)]
    [ValidateSet('set_rune_gold', 'set_rune_rarity', 'set_effect_duration', 'set_effect_maxstack')]
    [string]$Op,

    [Parameter(Mandatory)]
    [string]$Value,

    [switch]$CloseEditor
)

. "$PSScriptRoot/_common.ps1"

$Script = Join-Path $PSScriptRoot "batch_ops.py"
$code = Invoke-UEPython -ScriptPath $Script -ScriptArgs @("--op", $Op, "--value", $Value) -CloseEditor:$CloseEditor

if ($code -eq 0) {
    Write-Host ""
    Write-Host "Batch op done. Modified assets were saved by the headless script." -ForegroundColor Yellow
    Write-Host "Review the asset diff in editor or source control. Use git restore for rollback after save." -ForegroundColor Yellow
}

exit $code
