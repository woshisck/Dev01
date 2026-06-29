param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\ArtifactContract"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Test-FileText {
    param(
        [string]$RelativePath,
        [string]$Pattern
    )

    $path = Join-Path $RepoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        return $false
    }

    return (Get-Content -LiteralPath $path -Raw) -match $Pattern
}

function Test-FileExists {
    param([string]$RelativePath)
    return Test-Path -LiteralPath (Join-Path $RepoRoot $RelativePath)
}

$checks = @()
$scheduledTaskHits = @(
    Get-ChildItem -LiteralPath (Join-Path $RepoRoot "BuildScripts/Automation") -Recurse -File -Include *.ps1,*.md |
        Where-Object { $_.Name -ne "Test-UE58PerformanceArtifactContract.ps1" } |
        Select-String -Pattern "Register-ScheduledTask|schtasks\.exe|schtasks /"
)

function Add-Check {
    param(
        [string]$Area,
        [string]$Name,
        [bool]$Passed,
        [string]$Evidence
    )

    $script:checks += [pscustomobject]@{
        Area = $Area
        Name = $Name
        Passed = $Passed
        Evidence = $Evidence
    }
}

Add-Check "MCP" "Codex MCP tool wrapper exists" `
    (Test-FileText "BuildScripts/Automation/Invoke-UE58McpTool.ps1" "method = `"initialize`"") `
    "BuildScripts/Automation/Invoke-UE58McpTool.ps1 initializes an MCP session."
Add-Check "MCP" "MCP tool wrapper can list toolsets" `
    (Test-FileText "BuildScripts/Automation/Invoke-UE58McpTool.ps1" "list_toolsets") `
    "Wrapper exposes the UE MCP list_toolsets flow."
Add-Check "Automation" "Codex heartbeat/report automation exists" `
    (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Invoke-UE58McpTool.ps1") `
    "Heartbeat script records MCP status through the Codex automation path."
Add-Check "Automation" "No Windows scheduled task automation is used" `
    ($scheduledTaskHits.Count -eq 0) `
    "Automation scripts contain no Register-ScheduledTask/schtasks usage."

Add-Check "PlayerSettings" "Graphics settings save model exists" `
    ((Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "struct FYogGraphicsSettings") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "EYogPerformanceTargetTier") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "SelectedTargetTier") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "DynamicLightQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "MaterialLightQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "MaterialLightMaxLightInfoCount") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "bUseLumenLite") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "bPreferBatchedGeometryProxies")) `
    "FYogGraphicsSettings stores profile, target tier, dynamic/material light budgets, Lumen Lite, and batch-proxy preferences."
Add-Check "PlayerSettings" "Runtime performance library applies profile CVars" `
    ((Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "ApplyGraphicsSettings") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.DynamicGlobalIlluminationMethod") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Nanite") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Shadow\.Virtual\.Enable")) `
    "Performance library applies GI, Nanite, VSM, resolution, and frame-rate settings."
Add-Check "PlayerSettings" "Target platform tier API exists" `
    ((Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "EYogPerformanceTargetTier") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "SteamDeck15W") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "Switch2Candidate") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "SteamDeck5W") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "MakeGraphicsSettingsForTargetTier") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "SelectedTargetTier = Tier") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "GetSelectablePerformanceTargetTiers")) `
    "Performance library exposes PC, Switch 2, Steam Deck 15W, Steam Deck 5W, and fallback target tiers."
Add-Check "PlayerSettings" "Graphics settings UI base exists" `
    ((Test-FileText "Source/DevKit/Public/UI/YogGraphicsSettingsWidgetBase.h" "class DEVKIT_API UYogGraphicsSettingsWidgetBase") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnApplyCustom") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "LumenLiteCheckBox")) `
    "Native settings widget exposes preset, custom apply, and Lumen Lite controls."
Add-Check "PlayerSettings" "Graphics settings UI exposes target platform tiers" `
    ((Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnTargetSteamDeck15W") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "HandleTargetSwitch2CandidateClicked") -and
        (Test-FileText "Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp" "TargetTierRow") -and
        (Test-FileText "Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp" "BtnTargetSteamDeck5W") -and
        (Test-FileText "Source/DevKit/Private/Tests/PerformanceSettingsTests.cpp" "BtnTargetFallbackLow")) `
    "Native fallback UI, generated WBP designer tree, and test contract expose PC/Deck/Switch/fallback target buttons."
Add-Check "PlayerSettings" "Graphics settings UI exposes detailed quality controls" `
    ((Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "ModelQualitySlider") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "ShadowQualitySlider") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "ReflectionQualitySlider") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "TextureQualitySlider") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "MaterialQualitySlider") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "DynamicLightQualitySlider") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "MaterialLightQualitySlider") -and
        (Test-FileText "Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp" "DetailedQualitySection")) `
    "Player graphics settings UI exposes model, shadow, reflection, texture, material, dynamic light, and material-light quality controls."
Add-Check "PlayerSettings" "Main menu settings entry exists" `
    ((Test-FileText "Source/DevKit/Public/System/YogGameInstanceBase.h" "GraphicsSettingsMenuClass") -and
        (Test-FileText "Source/DevKit/Private/System/YogGameInstanceBase.cpp" "ShowGraphicsSettingsMenuWidget") -and
        (Test-FileText "Source/DevKit/Private/System/YogGameInstanceBase.cpp" "Graphics Settings")) `
    "Game instance can open the graphics settings menu from the frontend."

Add-Check "Scalability" "Lumen Lite scalability tier exists" `
    ((Test-FileText "Config/DefaultScalability.ini" "\[GlobalIlluminationQuality@1\]") -and
        (Test-FileText "Config/DefaultScalability.ini" "r\.Lumen\.DiffuseIndirect\.Allow=1") -and
        (Test-FileText "Config/DefaultScalability.ini" "r\.Lumen\.TraceMeshSDFs\.Allow=0")) `
    "GI quality 1 defines the handheld Lumen Lite candidate path."
Add-Check "Scalability" "Handheld and low device profiles exist" `
    ((Test-FileText "Config/DefaultDeviceProfiles.ini" "\[DevKit_Medium DeviceProfile\]") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "\[DevKit_Low DeviceProfile\]") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "r\.Nanite\.ProjectEnabled=0") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "r\.Shadow\.Virtual\.Enable=0")) `
    "Device profiles define handheld/low tiers with Nanite and VSM disabled."

Add-Check "MaterialBatch" "Material batch commandlets exist" `
    ((Test-FileExists "Source/DevKitEditor/MaterialBatch/MaterialBatchAuditCommandlet.cpp") -and
        (Test-FileExists "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp") -and
        (Test-FileExists "Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAuditCommandlet.cpp")) `
    "Editor commandlets cover audit, build, and material audit flows."
Add-Check "MaterialBatch" "Runtime mapping asset exists" `
    ((Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "UMaterialBatchMappingDataAsset") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingTextureArraySlice") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "SliceIndex")) `
    "Runtime data asset can store material-batch mapping metadata."

Add-Check "Tests" "Automation tests cover settings and batch rules" `
    ((Test-FileExists "Source/DevKit/Private/Tests/PerformanceSettingsTests.cpp") -and
        (Test-FileExists "Source/DevKitEditor/Private/Tests/MaterialBatchCandidateRulesTests.cpp") -and
        (Test-FileExists "Source/DevKitEditor/Private/Tests/UE58ScenePerformanceAuditTests.cpp") -and
        (Test-FileText "Source/DevKit/Private/Tests/PerformanceSettingsTests.cpp" "TargetTierMappings")) `
    "C++ automation test sources exist for player settings, material batching, and scene audit."
Add-Check "Tests" "Required test runner includes UE suites" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58RequiredTests.ps1" "DevKit\.Performance\.Settings") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58RequiredTests.ps1" "DevKitEditor\.MaterialBatch") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58RequiredTests.ps1" "-Run")) `
    "Required test runner can run the focused UE automation suites."

Add-Check "UploadGate" "Final upload fetches and audits remote first" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "Fetch latest remote before upload") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "Remote audit required before upload") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "Invoke-UE58RemoteDeltaAudit.ps1")) `
    "Final upload automation fetches origin and reruns remote audit before commit/push."
Add-Check "UploadGate" "Final upload compiles before push" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "Invoke-UEBuild") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "Required automation tests failed") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "git push origin main")) `
    "Final upload path builds, runs tests, builds again, then commits and pushes."

$failedChecks = @($checks | Where-Object { -not $_.Passed })
$status = if ($failedChecks.Count -eq 0) { "Passed" } else { "Failed" }
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58ArtifactContract_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$lines = @(
    "# UE5.8 Performance Artifact Contract",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Checks: $($checks.Count)",
    "- Failed checks: $($failedChecks.Count)",
    "",
    "## Checks",
    "",
    "| Area | Check | Passed | Evidence |",
    "| --- | --- | --- | --- |"
)

foreach ($check in $checks) {
    $lines += "| $($check.Area) | $($check.Name) | $($check.Passed) | $($check.Evidence) |"
}

if ($failedChecks.Count -gt 0) {
    $lines += @(
        "",
        "## Failed Checks",
        ""
    )
    foreach ($check in $failedChecks) {
        $lines += "- $($check.Area): $($check.Name)"
    }
}

$lines += @(
    "",
    "## Notes",
    "",
    "- This is a static Codex-side contract check. It does not compile, open Unreal, run commandlets, stage files, commit, or push.",
    "- Passing this check does not replace fresh UE automation tests and a pre-upload compile."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote artifact contract report: $reportPath"
Write-Output "Updated artifact contract latest: $latestPath"
Write-Output "Status: $status"

if ($status -ne "Passed") {
    exit 1
}
