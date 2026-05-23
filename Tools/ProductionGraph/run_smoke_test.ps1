$ErrorActionPreference = "Stop"

node Tools\ProductionGraph\tests\server_smoke.js
node Tools\ProductionGraph\tests\ui_contract_smoke.js
node Tools\ProductionGraph\tests\story_metadata_smoke.js
node Tools\ProductionGraph\tests\story_pipeline_smoke.js
