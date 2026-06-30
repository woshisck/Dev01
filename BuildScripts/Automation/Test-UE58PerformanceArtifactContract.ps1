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
$masterPlanMatch = Get-ChildItem -LiteralPath (Join-Path $RepoRoot "Docs") -Recurse -File -Filter "UE58_EpicHighMidLow_*.md" -ErrorAction SilentlyContinue | Select-Object -First 1
$masterPlanText = if ($masterPlanMatch) { Get-Content -LiteralPath $masterPlanMatch.FullName -Raw } else { "" }

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
Add-Check "MCP" "Material MCP audit has quick mode and full graph opt-in" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1" "FullExpressionGraph") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1" "ProbeAllOutputs") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1" "SkippedQuickMaterialOutputs") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1" "MaterialAttributesOnly") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialMcpAudit.ps1" "get_expressions")) `
    "Material MCP audit defaults to a bounded MaterialAttributes probe and keeps full expression graph traversal as an explicit opt-in."
Add-Check "Automation" "Codex heartbeat/report automation exists" `
    (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Invoke-UE58McpTool.ps1") `
    "Heartbeat script records MCP status through the Codex automation path."
Add-Check "Automation" "No Windows scheduled task automation is used" `
    ($scheduledTaskHits.Count -eq 0) `
    "Automation scripts contain no Register-ScheduledTask/schtasks usage."
Add-Check "Automation" "Heartbeat reports governance gate" `
    ((Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Governance Gate") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "UE58_EpicHighMidLow_") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Source/DevKitEditor") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "ResidencyRiskPlan") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Source/Proxy/Baked layer readiness") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Submission MaterialBatch dry-run ready") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Submission Source/Proxy asset evidence")) `
    "Heartbeat reports master-plan governance, module ownership, generated asset policy, submission MaterialBatch dry-run readiness, and residency/readiness gates."
Add-Check "Automation" "Heartbeat records 5-hour continuation target" `
    ((Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "ue58-epic-high-mid-low") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "RRULE:FREQ=HOURLY;INTERVAL=5") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Compilation is allowed for this goal") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "MaterialBatch dry-run latest report") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "auto-close the editor") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "auto-launch UE5.8 editor for MCP")) `
    "Heartbeat preserves the 5-hour continuation automation id, compile permission, dry-run resume evidence, auto-close compile policy, and MCP auto-launch policy."
Add-Check "Automation" "Current master plan records repository execution rules" `
    ([bool]$masterPlanMatch -and
        $masterPlanText -match "AGENTS" -and
        $masterPlanText -match "guide" -and
        $masterPlanText -match "Source/DevKit" -and
        $masterPlanText -match "Source/DevKitEditor" -and
        $masterPlanText -match "Config/DefaultDeviceProfiles" -and
        $masterPlanText -match "Docs/GeneratedReports" -and
        $masterPlanText -match "Commandlet" -and
        $masterPlanText -match "uasset" -and
        $masterPlanText -match "ResidencyRiskPlan" -and
        $masterPlanText -match "Source/Proxy/Baked layer readiness") `
    "Master plan embeds AGENTS/guide startup rules, module boundaries, generated asset policy, and Source/Proxy/Baked plus residency validation gates."
Add-Check "Automation" "MaterialBatch real-cluster dry-run automation exists" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "MaterialBatchAudit") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "MaterialBatchBuild") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "BlockedByBuild") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "AllowStaleBinaries") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "Actual layer evidence") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "Layer evidence mode") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "Source/Proxy asset evidence") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "Source/Proxy asset config set evidence") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58MaterialBatchDryRun.ps1" "Residency risk evidence")) `
    "MaterialBatch dry-run automation has a stale-binary gate and records actual StreamingLevel layer, Source/Proxy asset readiness/config-set, plus residency evidence before accepting a real cluster dry-run."
Add-Check "Automation" "Pilot cluster selection evidence exists" `
    ((Test-FileText "BuildScripts/Automation/Select-UE58PilotCluster.ps1" "Prison_S_01_SourceProxy") -and
        (Test-FileText "BuildScripts/Automation/Select-UE58PilotCluster.ps1" "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01") -and
        (Test-FileText "BuildScripts/Automation/Select-UE58PilotCluster.ps1" "ReadyForCommandlet") -and
        (Test-FileText "BuildScripts/Automation/Select-UE58PilotCluster.ps1" "Required EnvBatch Tags") -and
        (Test-FileText "BuildScripts/Automation/Select-UE58PilotCluster.ps1" "Invoke-UE58MaterialBatchDryRun.ps1") -and
        (Test-FileText "BuildScripts/Automation/Select-UE58PilotCluster.ps1" "Actor tag evidence captured")) `
    "Pilot cluster selection records the first real-cluster map, EnvBatch tag contract, dry-run command, and the split between commandlet readiness and captured actor evidence."
Add-Check "Automation" "Runtime profiling plan fallback report exists" `
    ((Test-FileText "BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1" "UE58RuntimeProfilingPlanReport.md") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1" "Baseline_LumenOff_NoBatch") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1" "BatchProxy_LumenLite") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1" "Commandlet fallback") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1" "profilegpu")) `
    "Runtime profiling plan has a no-link fallback report writer while the commandlet class is unavailable in the current locked editor binaries."
Add-Check "Automation" "Commandlet availability report exists" `
    ((Test-FileText "BuildScripts/Automation/Test-UE58CommandletAvailability.ps1" "ClassMissingUntilCleanLink") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58CommandletAvailability.ps1" "GraphicsSettingsWidgetSetup") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58CommandletAvailability.ps1" "MaterialBatchMaterialAudit") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58CommandletAvailability.ps1" "UE58RuntimeProfilingPlan") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58CommandletAvailability.ps1" "BlockedByCleanLink")) `
    "Commandlet availability report records report-only commandlet class-missing results caused by locked stale editor binaries."
Add-Check "Automation" "Post-clean-link validation continuation exists" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1" "RunReportCommandlets") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1" "RunMaterialBatchDryRun") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1" "BlockedByOpenEditor") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1" "This script never closes UnrealEditor") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1" "CommandletAvailability") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58PostCleanLinkValidation.ps1" "MaterialBatchDryRun") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Post-clean-link validation")) `
    "Heartbeat refreshes a safe post-clean-link continuation report; the run switches rerun report-only commandlets and then the real MaterialBatch dry-run without closing UnrealEditor."
Add-Check "Automation" "Build validation report exists without automatic heartbeat compile" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58BuildValidation.ps1" "RunBuild") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58BuildValidation.ps1" "AllowOpenEditor") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58BuildValidation.ps1" "LatestBuildBlockedByOpenEditor") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58BuildValidation.ps1" "AutoCloseEditor") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58BuildValidation.ps1" "Editor close attempted") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Build validation") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "heartbeat only refreshes the non-compiling build-status report")) `
    "Heartbeat records latest UBT/open-editor build status without compiling; explicit -RunBuild can auto-close a blocking editor and records the compile attempt."
Add-Check "Automation" "Runtime profiling capture prepared report is refreshed by heartbeat" `
    ((Test-FileText "BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1" "Z:\\GZA_Software\\RealityCapture\\UE_5.8") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1" "Status: Prepared") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1" "Baseline_LumenOff_NoBatch") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58RuntimeProfilingCapture.ps1" "LumenLite_NoBatch") -and
        (Test-FileText "BuildScripts/Automation/Run-UE58PerformanceGoalHeartbeat.ps1" "Runtime profiling capture")) `
    "Heartbeat refreshes the non-running runtime profiling capture report; explicit -Run remains required for measured GPU evidence."
Add-Check "Automation" "Submission and upload gates require commandlet availability" `
    ((Test-FileText "BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1" "Commandlet availability ready") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1" "commandletAvailabilityReady") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1" "BlockedByCleanLink") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "Commandlet availability ready")) `
    "Submission and final-upload gates require report-only commandlets to be available before real dry-run or upload."
Add-Check "Automation" "Submission gate requires MaterialBatch real-cluster evidence" `
    ((Test-FileText "BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1" "MaterialBatch dry-run ready") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1" "MaterialBatch dry-run Source/Proxy asset evidence") -and
        (Test-FileText "BuildScripts/Automation/Write-UE58SubmissionGateReport.ps1" "materialBatchDryRunReady") -and
        (Test-FileText "BuildScripts/Automation/Invoke-UE58FinalUpload.ps1" "MaterialBatch dry-run ready")) `
    "Submission and final-upload gates require the real-cluster MaterialBatch dry-run evidence before allowing Phase 1 commit/upload."

Add-Check "PlayerSettings" "Graphics settings save model exists" `
    ((Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "struct FYogGraphicsSettings") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "EYogPerformanceTargetTier") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "SelectedTargetTier") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "MaterialQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "DynamicOverlayQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "VTAtlasQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "DynamicLightQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "MaterialLightQuality") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "MaterialLightMaxLightInfoCount") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "bUseLumenLite") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "bPreferBatchedGeometryProxies")) `
    "FYogGraphicsSettings stores profile, target tier, material/overlay/VT budgets, dynamic/material light budgets, Lumen Lite, and batch-proxy preferences."
Add-Check "PlayerSettings" "Runtime performance library applies profile CVars" `
    ((Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "ApplyGraphicsSettings") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.DynamicGlobalIlluminationMethod") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Nanite") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Shadow\.Virtual\.Enable") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Yog\.MaterialQuality") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Yog\.DynamicOverlayQuality") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Yog\.BatchProxyPreference") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "r\.Yog\.VTAtlasQuality")) `
    "Performance library applies GI, Nanite, VSM, resolution, frame-rate, material, overlay, proxy, and VT atlas settings."
Add-Check "PlayerSettings" "Four formal target tier API exists" `
    ((Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "EYogPerformanceTargetTier") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "Epic") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "High") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "Mid") -and
        (Test-FileText "Source/DevKit/Public/SaveGame/YogSettingsSave.h" "Low") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "MakeGraphicsSettingsForTargetTier") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "SelectedTargetTier = Tier") -and
        (Test-FileText "Source/DevKit/Private/System/YogPerformanceSettingsLibrary.cpp" "GetSelectablePerformanceTargetTiers")) `
    "Performance library exposes Epic, High, Mid, and Low target tiers."
Add-Check "PlayerSettings" "Graphics settings UI base exists" `
    ((Test-FileText "Source/DevKit/Public/UI/YogGraphicsSettingsWidgetBase.h" "class DEVKIT_API UYogGraphicsSettingsWidgetBase") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnApplyCustom") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "LumenLiteCheckBox")) `
    "Native settings widget exposes preset, custom apply, and Lumen Lite controls."
Add-Check "PlayerSettings" "Graphics settings UI exposes four formal tiers" `
    ((Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnTierEpic") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnTierHigh") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnTierMid") -and
        (Test-FileText "Source/DevKit/Private/UI/YogGraphicsSettingsWidgetBase.cpp" "BtnTierLow") -and
        (Test-FileText "Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp" "TargetTierRow") -and
        (Test-FileText "Source/DevKitEditor/UI/GraphicsSettingsWidgetSetupCommandlet.cpp" "BtnTierEpic") -and
        (Test-FileText "Source/DevKit/Private/Tests/PerformanceSettingsTests.cpp" "BtnTierLow")) `
    "Native fallback UI, generated WBP designer tree, and test contract expose Epic/High/Mid/Low target buttons."
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
Add-Check "Scalability" "Epic/High/Mid/Low device profiles exist" `
    ((Test-FileText "Config/DefaultDeviceProfiles.ini" "\[DevKit_Epic DeviceProfile\]") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "\[DevKit_High DeviceProfile\]") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "\[DevKit_Mid DeviceProfile\]") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "\[DevKit_Low DeviceProfile\]") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "r\.Yog\.VTAtlasQuality") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "r\.Yog\.BatchProxyPreference") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "r\.Nanite\.ProjectEnabled=0") -and
        (Test-FileText "Config/DefaultDeviceProfiles.ini" "r\.Shadow\.Virtual\.Enable=0")) `
    "Device profiles define the four formal tiers with project CVar budgets, Nanite disabled, and VSM disabled."

Add-Check "MaterialBatch" "Material batch commandlets exist" `
    ((Test-FileExists "Source/DevKitEditor/MaterialBatch/MaterialBatchAuditCommandlet.cpp") -and
        (Test-FileExists "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp") -and
        (Test-FileExists "Source/DevKitEditor/MaterialBatch/MaterialBatchMaterialAuditCommandlet.cpp")) `
    "Editor commandlets cover audit, build, and material audit flows."
Add-Check "MaterialBatch" "Runtime mapping asset exists" `
    ((Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "UMaterialBatchMappingDataAsset") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingTextureArraySlice") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingVTAtlasEntry") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "SliceIndex") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "TextureBackend") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "VTAtlasEntries") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "VTAtlasPackage") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "SourceProxyExclusivityGroup")) `
    "Runtime data asset can store material-batch mapping metadata, VT atlas metadata, and source/proxy exclusivity metadata."
Add-Check "MaterialBatch" "Material batch build accepts new tier and VT atlas options" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "TextureBackend=") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "SurfaceKind=") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "BakePolicy=") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "ApplyVTAtlasOnly") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "ValidateSourceProxyExclusivity")) `
    "MaterialBatchBuild records VT atlas, surface, bake, and source/proxy validation options."
Add-Check "MaterialBatch" "Source/proxy and decal diagnostics are emitted" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildTagDiagnostics") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "ReportStaticDecals") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "ValidateSourceProxyExclusivity") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "EnvBatch\.BakeStaticDecal") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "EnvBatch\.RuntimeDecal") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "EnvBatch Tag Diagnostics") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "tagDiagnostics") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "envBatchTags")) `
    "MaterialBatchBuild reports EnvBatch tag counts, source/proxy conflicts, static/runtime decal warnings, and per-entry tags."
Add-Check "MaterialBatch" "Source/proxy/baked layer plan is emitted for all tiers" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildSourceProxyLayerPlan") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildSourceProxyLayerReadiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildSourceProxyLayerPlan") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildSourceProxyLayerReadiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "Source/Proxy/Baked Layer Plan") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "Source/Proxy/Baked Layer Readiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "sourceProxyLayerPlan") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "sourceProxyLayerReadiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildSourceProxyAssetReadiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildSourceProxyAssetReadiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "Source/Proxy Asset Readiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "sourceProxyAssetReadiness") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildSourceProxyAssetConfigSet") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildSourceProxyAssetConfigSet") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "Source/Proxy Asset Config Set") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "sourceProxyAssetConfigSet") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "ImportSettingsOrArtAssetManagerRequired") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "sourceLODIndex") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "proxyLODIndex") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "MissingSourceAssetReference") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "LayerBackend") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "actualLayers") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "MatchedExpectedLayer") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "GetActorStreamingLayerNames") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "FlushLevelStreaming") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "SL_%s") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "Prefer generated Proxy layer with VT atlas batch material") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingSourceProxyLayerPlan") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingSourceProxyLayerReadiness") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingSourceProxyAssetReadiness") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingSourceProxyAssetConfigSet") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "ConfigSource") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "ActualLayerNames") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "ActualStreamingLevelName") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "TierSelections")) `
    "MaterialBatchBuild writes deterministic Source/Proxy/Baked layer names, Epic/High/Mid/Low selections, per-entry StreamingLevel readiness, Source/Proxy asset pairing readiness, Source/Proxy asset config sets, and actual layer membership checks into report, manifest, and mapping data."
Add-Check "MaterialBatch" "EnvBatch tagger exposes static ground and wall bake tags" `
    ((Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "EnvBatch\.Baked\.Ground\.Mid") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "EnvBatch\.Baked\.Ground\.Low") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "EnvBatch\.Baked\.Wall\.Mid") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "EnvBatch\.Baked\.Wall\.Low") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.h" "ApplyBakedWallMidTag") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.h" "ApplyBakedWallLowTag")) `
    "Native EnvBatch Tagger can mark both ground and wall static-bake replacements for Mid/Low tier validation."
Add-Check "MaterialBatch" "EnvBatch tagger is tag-only and reports selected tag state" `
    ((Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.h" "GetAssetReadinessSummaryText") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "EnvBatch Actor Tag") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "Level Instance") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SEnvBatchTaggerWidget.cpp" "StaticMeshComponent") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "_asset_readiness_summary") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "MissingSourceAssetReference")) `
    "Native EnvBatch Tagger only writes Actor Tags and reports selected tag state; Python fallback keeps Source/Proxy readiness logging."
Add-Check "MaterialBatch" "Performance tools expose model compliance and texture rules windows" `
    ((Test-FileText "Source/DevKitEditor/Private/Tools/SModelAssetComplianceWidget.cpp" "/Game/Art") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SModelAssetComplianceWidget.cpp" "LODMappingNote") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SModelAssetComplianceWidget.cpp" "LODMappingReady") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SModelAssetComplianceWidget.cpp" "MissingLOD1") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SModelAssetComplianceWidget.cpp" "LODCount < 2") -and
        (-not (Test-Path -LiteralPath (Join-Path $RepoRoot "Source/DevKitEditor/Private/Tools/SModelPerformanceConfigWidget.cpp"))) -and
        (-not (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "DevKitModelPerformanceConfig")) -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SMaterialTextureRulesWidget.cpp" "CheckSelectedTextureNaming") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SMaterialTextureRulesWidget.cpp" "DA_MaterialTextureNamingRules_Default") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SMaterialBatchToolsWidget.cpp" "MaterialBatchBuild") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SMaterialBatchToolsWidget.cpp" "RequireTag") -and
        (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "DevKitMaterialBatchTools") -and
        (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "DevKitModelAssetCompliance") -and
        (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "DevKitMaterialTextureRules")) `
    "Editor performance tools expose art-facing model asset compliance and texture naming checks; the removed per-model config window is not registered, and the material batch command widget remains registered for formal packaging/TA paths."

Add-Check "MaterialBatch" "Material batch command UI is not art-facing" `
    ((-not (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "LauncherOpenMaterialBatchTools")) -and
        (-not (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "OpenMaterialBatchToolsLabel")) -and
        (Test-FileText "Source/DevKitEditor/DevKitEditor.cpp" "DevKitModelAssetCompliance") -and
        (Test-FileText "Source/DevKitEditor/Private/Tools/SModelAssetComplianceWidget.cpp" "BatchDeferred")) `
    "The editor launcher/menu no longer exposes material batch commands to artists; asset compliance is the art-facing model workflow."
Add-Check "MaterialBatch" "EnvBatch Python fallback matches native baked tags" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "EnvBatch\.Baked\.Ground\.Mid") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "EnvBatch\.Baked\.Ground\.Low") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "EnvBatch\.Baked\.Wall\.Mid") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "EnvBatch\.Baked\.Wall\.Low") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "EnvBatch\.Proxy\.\{_group_name\(\)\}\.Mid") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py" "MUTUALLY_EXCLUSIVE_PREFIXES")) `
    "Python fallback tagger uses the same Source/Proxy/Baked mutual-exclusion and ground/wall Mid/Low baked-surface tags as the native editor tool."
Add-Check "MaterialBatch" "EnvBatch tag tool contract automation exists" `
    ((Test-FileText "BuildScripts/Automation/Test-UE58EnvBatchTagTools.ps1" "EnvBatch\.Baked\.Ground\.Mid") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58EnvBatchTagTools.ps1" "EnvBatch\.Baked\.Wall\.Low") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58EnvBatchTagTools.ps1" "py_compile") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58EnvBatchTagTools.ps1" "MUTUALLY_EXCLUSIVE_PREFIXES") -and
        (Test-FileText "BuildScripts/Automation/Test-UE58EnvBatchTagTools.ps1" "EnvBatchTagTools")) `
    "EnvBatch native/Python tag parity has a standalone no-Unreal contract report."
Add-Check "MaterialBatch" "VT atlas build plan and residency risk are emitted" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildVTAtlasEntry") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.h" "FMaterialBatchBuildVTAtlasPayload") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildVTAtlasEntries") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildVTAtlasPayload") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "DeterministicGridByUniqueTexture") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "UDIMStyleGrid") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "udimNumber") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "tilePaddingPixels") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "PlannedForMergedProxyUVRemap") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BaseVTUVMinX") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "baseVTUVMaxX") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "NormalVTUVMinX") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "normalVTUVMaxX") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "ORMVTUVMinX") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "ormVTUVMaxX") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "duplicateResidencyRisk") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BuildResidencyRiskPlan") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "VT/Non-VT Residency Risk Plan") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "BlockedUntilSourceProxyUnloaded") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "allowTextureArrayFallbackInProduction") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "FMaterialBatchMappingResidencyRiskPlan") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "UdimNumber") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "UVRemapStatus") -and
        (Test-FileText "Source/DevKit/Public/System/MaterialBatchMappingDataAsset.h" "ResidencyGate") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "EstimatedVTPoolMB") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildPlan.cpp" "VT_Atlas")) `
    "MaterialBatchBuild emits VT/UDIM-style atlas entries, tile metadata, property-texture UV rect columns, pool estimates, duplicate-residency gates, and VT atlas material binding."
Add-Check "MaterialBatch" "Batch parent material setup uses VT atlas sampling" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.cpp" "VT_Atlas") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.cpp" "Texture2DSample\(VT_Atlas") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.cpp" "17.0f, -1060, 260") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.cpp" "BuildVTAtlasLookupCode")) `
    "MaterialBatchParentMaterialSetup generates the VT_Atlas + _PropTexture UVRect parent-material contract."
Add-Check "MaterialBatch" "ApplyVTAtlasOnly writes a virtual texture atlas asset" `
    ((Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "SaveVTAtlasAsset") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "CopyTextureSourceIntoAtlasCell") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "TSF_BGRA8") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "VirtualTextureStreaming = true") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "bApplyVTAtlasOnly") -and
        (Test-FileText "Source/DevKitEditor/MaterialBatch/MaterialBatchBuildCommandlet.cpp" "bSavedVTAtlas")) `
    "MaterialBatchBuild can write a virtual-texture-enabled Texture2D atlas for eligible BGRA8 source textures."

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
