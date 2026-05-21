$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $Root
try {
    node Tools\ProductionGraph\tests\storage_smoke.js
    node Tools\ProductionGraph\tests\server_smoke.js
    node Tools\ProductionGraph\tests\static_smoke.js
    node Tools\ProductionGraph\tests\ui_contract_smoke.js
}
finally {
    Pop-Location
}
