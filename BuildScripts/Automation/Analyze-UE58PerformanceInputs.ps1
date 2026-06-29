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
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAudit"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Get-ConfigValue {
    param(
        [string[]]$Lines,
        [string]$Section,
        [string]$Key
    )

    $inSection = $false
    foreach ($line in $Lines) {
        $trimmed = $line.Trim()
        if ($trimmed -match '^\[(.+)\]$') {
            $inSection = ($Matches[1] -eq $Section)
            continue
        }
        if ($inSection -and $trimmed -match "^$([regex]::Escape($Key))=(.*)$") {
            return $Matches[1].Trim()
        }
    }
    return ""
}

function Count-Files {
    param([string]$Path, [string]$Filter = "*.uasset")
    if (-not (Test-Path -LiteralPath $Path)) {
        return 0
    }
    return (Get-ChildItem -LiteralPath $Path -Recurse -File -Filter $Filter -ErrorAction SilentlyContinue | Measure-Object).Count
}

function Count-ByNamePattern {
    param([string]$Path, [string]$Pattern)
    if (-not (Test-Path -LiteralPath $Path)) {
        return 0
    }
    return (Get-ChildItem -LiteralPath $Path -Recurse -File -Filter "*.uasset" -ErrorAction SilentlyContinue |
        Where-Object { $_.BaseName -match $Pattern } |
        Measure-Object).Count
}

function Get-MarkdownBulletValue {
    param(
        [string]$Text,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    $pattern = "(?m)^- $([regex]::Escape($Label)): (.+)$"
    if ($Text -match $pattern) {
        return $Matches[1].Trim()
    }

    return ""
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58PerformanceAudit_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$defaultEnginePath = Join-Path $RepoRoot "Config\DefaultEngine.ini"
$defaultGamePath = Join-Path $RepoRoot "Config\DefaultGame.ini"
$defaultEditorSettingsPath = Join-Path $RepoRoot "Config\DefaultEditorPerProjectUserSettings.ini"
$defaultScalabilityPath = Join-Path $RepoRoot "Config\DefaultScalability.ini"
$defaultDeviceProfilesPath = Join-Path $RepoRoot "Config\DefaultDeviceProfiles.ini"
$settingsSavePath = Join-Path $RepoRoot "Source\DevKit\Public\SaveGame\YogSettingsSave.h"
$performanceLibraryHeaderPath = Join-Path $RepoRoot "Source\DevKit\Public\System\YogPerformanceSettingsLibrary.h"
$performanceLibraryCppPath = Join-Path $RepoRoot "Source\DevKit\Private\System\YogPerformanceSettingsLibrary.cpp"
$performanceSettingsTestsPath = Join-Path $RepoRoot "Source\DevKit\Private\Tests\PerformanceSettingsTests.cpp"
$gameInstanceCppPath = Join-Path $RepoRoot "Source\DevKit\Private\System\YogGameInstanceBase.cpp"
$graphicsSettingsWidgetHeaderPath = Join-Path $RepoRoot "Source\DevKit\Public\UI\YogGraphicsSettingsWidgetBase.h"
$graphicsSettingsWidgetCppPath = Join-Path $RepoRoot "Source\DevKit\Private\UI\YogGraphicsSettingsWidgetBase.cpp"
$graphicsSettingsWidgetSetupCommandletPath = Join-Path $RepoRoot "Source\DevKitEditor\UI\GraphicsSettingsWidgetSetupCommandlet.cpp"
$graphicsSettingsWidgetAssetPath = Join-Path $RepoRoot "Content\UI\Frontend\WBP_GraphicsSettingsWidget.uasset"
$graphicsSettingsWidgetSetupReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\GraphicsSettingsWidgetSetupReport.md"
$ue58MaterialMcpAuditReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\UE58MaterialMcpAudit_LATEST.md"
$ue58BatchVisualMcpAuditReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit\LATEST.md"
$ue58BatchVisualMcpAuditRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit"
$ue58SceneParityMcpAuditReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SceneParityAudit\LATEST.md"
$ue58SceneParityMcpAuditPngPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\SceneParityAudit\scene_parity_side_by_side.png"
$ue58RuntimeProfilingMcpSmokeReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingSmoke\LATEST.md"
$ue58RuntimeProfilingCaptureScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58RuntimeProfilingCapture.ps1"
$ue58RuntimeProfilingCaptureReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture\LATEST.md"
$materialBatchMappingDataAssetHeaderPath = Join-Path $RepoRoot "Source\DevKit\Public\System\MaterialBatchMappingDataAsset.h"
$materialBatchMappingDataAssetCppPath = Join-Path $RepoRoot "Source\DevKit\Private\System\MaterialBatchMappingDataAsset.cpp"
$materialBatchAuditCommandletPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\MaterialBatchAuditCommandlet.cpp"
$materialBatchBuildCommandletPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\MaterialBatchBuildCommandlet.cpp"
$materialBatchBuildPlanPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\MaterialBatchBuildPlan.cpp"
$materialBatchParentMaterialSetupCommandletPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\MaterialBatchParentMaterialSetupCommandlet.cpp"
$materialBatchMaterialAuditCommandletPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\MaterialBatchMaterialAuditCommandlet.cpp"
$materialBatchCandidateRulesPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\MaterialBatchCandidateRules.cpp"
$materialBatchCandidateTestsPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tests\MaterialBatchCandidateRulesTests.cpp"
$materialBatchAuditReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchAuditReport.md"
$materialBatchBuildReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchBuildReport.md"
$materialBatchBuildManifestPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchBuildManifest.json"
$materialBatchParentMaterialSetupReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchParentMaterialSetupReport.md"
$materialBatchMaterialAuditReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\MaterialBatchMaterialAuditReport.md"
$ue58ScenePerformanceAuditReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\UE58ScenePerformanceAuditReport.md"
$ue58RuntimeProfilingPlanCommandletPath = Join-Path $RepoRoot "Source\DevKitEditor\Performance\UE58RuntimeProfilingPlanCommandlet.cpp"
$ue58RuntimeProfilingPlanPath = Join-Path $RepoRoot "Source\DevKitEditor\Performance\UE58RuntimeProfilingPlan.cpp"
$ue58ScenePerformanceTestsPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tests\UE58ScenePerformanceAuditTests.cpp"
$ue58RuntimeProfilingPlanReportPath = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports\UE58RuntimeProfilingPlanReport.md"
$materialBatchGeneratedMappingAssetPath = Join-Path $RepoRoot "Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\DA_MaterialBatchMap_FloorBrick03_Probe.uasset"
$materialBatchGeneratedPropertyTexturePath = Join-Path $RepoRoot "Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\T_PropTexture_FloorBrick03_Probe.uasset"
$materialBatchGeneratedProxyMeshPath = Join-Path $RepoRoot "Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\SM_BatchProxy_FloorBrick03_Probe.uasset"
$materialBatchGeneratedBatchMaterialPath = Join-Path $RepoRoot "Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe\MI_Env_Batch_FloorBrick03_Probe.uasset"
$materialBatchGeneratedTextureArrayRoot = Join-Path $RepoRoot "Content\Generated\MaterialBatch\Medium\FloorBrick03_Probe"
$materialBatchParentMaterialAssetPath = Join-Path $RepoRoot "Content\Art\Material\EnvMaterial\Main\M_Env_Building_Batch.uasset"
$targetMaterialPath = Join-Path $RepoRoot "Content\Art\Material\EnvMaterial\Main\M_Env_Building.uasset"
$tierPlanMatch = Get-ChildItem -LiteralPath (Join-Path $RepoRoot "Docs") -Recurse -File -Filter "UE58_ArtPerformanceTieringAndBatching.md" -ErrorAction SilentlyContinue | Select-Object -First 1
$tierPlanPath = if ($tierPlanMatch) { $tierPlanMatch.FullName } else { "(not found)" }
$comprehensivePlanMatch = Get-ChildItem -LiteralPath (Join-Path $RepoRoot "Docs") -Recurse -File -Filter "UE58_*.md" -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -ne "UE58_ArtPerformanceTieringAndBatching.md" } |
    Select-Object -First 1
$comprehensivePlanPath = if ($comprehensivePlanMatch) { $comprehensivePlanMatch.FullName } else { "(not found)" }

$defaultEngineLines = @()
if (Test-Path -LiteralPath $defaultEnginePath) {
    $defaultEngineLines = Get-Content -LiteralPath $defaultEnginePath
}

$defaultGameText = ""
if (Test-Path -LiteralPath $defaultGamePath) {
    $defaultGameText = Get-Content -LiteralPath $defaultGamePath -Raw
}

$rendererSection = "/Script/Engine.RendererSettings"
$rendererKeys = @(
    "r.DynamicGlobalIlluminationMethod",
    "r.ReflectionMethod",
    "r.Shadow.Virtual.Enable",
    "r.Nanite.ProjectEnabled",
    "r.Nanite",
    "r.GenerateMeshDistanceFields",
    "r.AntiAliasingMethod",
    "r.VirtualTextures",
    "r.VirtualTexturedLightmaps",
    "r.ForwardShading"
)

$rendererRows = @()
foreach ($key in $rendererKeys) {
    $rendererRows += [pscustomobject]@{
        Key = $key
        Value = Get-ConfigValue -Lines $defaultEngineLines -Section $rendererSection -Key $key
    }
}

$mcpConfigured = $false
if (Test-Path -LiteralPath $defaultEditorSettingsPath) {
    $editorSettingsText = Get-Content -LiteralPath $defaultEditorSettingsPath -Raw
    $mcpConfigured = $editorSettingsText -match "ModelContextProtocolSettings" -and $editorSettingsText -match "ServerPortNumber=8765"
}

$deviceProfileLines = @()
if (Test-Path -LiteralPath $defaultDeviceProfilesPath) {
    $deviceProfileLines = Get-Content -LiteralPath $defaultDeviceProfilesPath
}

$devKitProfiles = @()
foreach ($line in $deviceProfileLines) {
    if ($line.Trim() -match '^\[(DevKit_[^\]]+) DeviceProfile\]$') {
        $devKitProfiles += $Matches[1]
    }
}

$scalabilityLines = @()
if (Test-Path -LiteralPath $defaultScalabilityPath) {
    $scalabilityLines = Get-Content -LiteralPath $defaultScalabilityPath
}

$scalabilitySections = @()
foreach ($line in $scalabilityLines) {
    if ($line.Trim() -match '^\[([A-Za-z]+Quality@[^\]]+)\]$') {
        $scalabilitySections += $Matches[1]
    }
}

$contentRoot = Join-Path $RepoRoot "Content"
$envMaterialRoot = Join-Path $RepoRoot "Content\Art\Material\EnvMaterial"
$artRoot = Join-Path $RepoRoot "Content\Art"

$counts = [ordered]@{
    TotalContentAssets = Count-Files -Path $contentRoot
    EnvMaterialAssets = Count-Files -Path $envMaterialRoot
    ArtAssets = Count-Files -Path $artRoot
    StaticMeshNameCandidates = Count-ByNamePattern -Path $contentRoot -Pattern "^(SM_|S_|Mesh_)"
    MaterialNameCandidates = Count-ByNamePattern -Path $contentRoot -Pattern "^(M_|MI_|MAT_)"
}

$settingsSaveText = if (Test-Path -LiteralPath $settingsSavePath) { Get-Content -LiteralPath $settingsSavePath -Raw } else { "" }
$performanceLibraryHeaderText = if (Test-Path -LiteralPath $performanceLibraryHeaderPath) { Get-Content -LiteralPath $performanceLibraryHeaderPath -Raw } else { "" }
$performanceLibraryCppText = if (Test-Path -LiteralPath $performanceLibraryCppPath) { Get-Content -LiteralPath $performanceLibraryCppPath -Raw } else { "" }
$performanceSettingsTestsText = if (Test-Path -LiteralPath $performanceSettingsTestsPath) { Get-Content -LiteralPath $performanceSettingsTestsPath -Raw } else { "" }
$gameInstanceCppText = if (Test-Path -LiteralPath $gameInstanceCppPath) { Get-Content -LiteralPath $gameInstanceCppPath -Raw } else { "" }
$graphicsSettingsWidgetHeaderText = if (Test-Path -LiteralPath $graphicsSettingsWidgetHeaderPath) { Get-Content -LiteralPath $graphicsSettingsWidgetHeaderPath -Raw } else { "" }
$graphicsSettingsWidgetCppText = if (Test-Path -LiteralPath $graphicsSettingsWidgetCppPath) { Get-Content -LiteralPath $graphicsSettingsWidgetCppPath -Raw } else { "" }
$graphicsSettingsWidgetSetupCommandletText = if (Test-Path -LiteralPath $graphicsSettingsWidgetSetupCommandletPath) { Get-Content -LiteralPath $graphicsSettingsWidgetSetupCommandletPath -Raw } else { "" }
$ue58MaterialMcpAuditReportText = if (Test-Path -LiteralPath $ue58MaterialMcpAuditReportPath) { Get-Content -LiteralPath $ue58MaterialMcpAuditReportPath -Raw } else { "" }
$ue58BatchVisualMcpAuditReportText = if (Test-Path -LiteralPath $ue58BatchVisualMcpAuditReportPath) { Get-Content -LiteralPath $ue58BatchVisualMcpAuditReportPath -Raw } else { "" }
$ue58SceneParityMcpAuditReportText = if (Test-Path -LiteralPath $ue58SceneParityMcpAuditReportPath) { Get-Content -LiteralPath $ue58SceneParityMcpAuditReportPath -Raw } else { "" }
$ue58RuntimeProfilingMcpSmokeReportText = if (Test-Path -LiteralPath $ue58RuntimeProfilingMcpSmokeReportPath) { Get-Content -LiteralPath $ue58RuntimeProfilingMcpSmokeReportPath -Raw } else { "" }
$ue58RuntimeProfilingCaptureScriptText = if (Test-Path -LiteralPath $ue58RuntimeProfilingCaptureScriptPath) { Get-Content -LiteralPath $ue58RuntimeProfilingCaptureScriptPath -Raw } else { "" }
$ue58RuntimeProfilingCaptureReportText = if (Test-Path -LiteralPath $ue58RuntimeProfilingCaptureReportPath) { Get-Content -LiteralPath $ue58RuntimeProfilingCaptureReportPath -Raw } else { "" }
$materialBatchAuditCommandletText = if (Test-Path -LiteralPath $materialBatchAuditCommandletPath) { Get-Content -LiteralPath $materialBatchAuditCommandletPath -Raw } else { "" }
$materialBatchBuildCommandletText = if (Test-Path -LiteralPath $materialBatchBuildCommandletPath) { Get-Content -LiteralPath $materialBatchBuildCommandletPath -Raw } else { "" }
$materialBatchBuildPlanText = if (Test-Path -LiteralPath $materialBatchBuildPlanPath) { Get-Content -LiteralPath $materialBatchBuildPlanPath -Raw } else { "" }
$materialBatchParentMaterialSetupCommandletText = if (Test-Path -LiteralPath $materialBatchParentMaterialSetupCommandletPath) { Get-Content -LiteralPath $materialBatchParentMaterialSetupCommandletPath -Raw } else { "" }
$materialBatchMaterialAuditCommandletText = if (Test-Path -LiteralPath $materialBatchMaterialAuditCommandletPath) { Get-Content -LiteralPath $materialBatchMaterialAuditCommandletPath -Raw } else { "" }
$ue58RuntimeProfilingPlanCommandletText = if (Test-Path -LiteralPath $ue58RuntimeProfilingPlanCommandletPath) { Get-Content -LiteralPath $ue58RuntimeProfilingPlanCommandletPath -Raw } else { "" }
$ue58RuntimeProfilingPlanText = if (Test-Path -LiteralPath $ue58RuntimeProfilingPlanPath) { Get-Content -LiteralPath $ue58RuntimeProfilingPlanPath -Raw } else { "" }
$ue58ScenePerformanceTestsText = if (Test-Path -LiteralPath $ue58ScenePerformanceTestsPath) { Get-Content -LiteralPath $ue58ScenePerformanceTestsPath -Raw } else { "" }
$materialBatchCandidateRulesText = if (Test-Path -LiteralPath $materialBatchCandidateRulesPath) { Get-Content -LiteralPath $materialBatchCandidateRulesPath -Raw } else { "" }
$materialBatchCandidateTestsText = if (Test-Path -LiteralPath $materialBatchCandidateTestsPath) { Get-Content -LiteralPath $materialBatchCandidateTestsPath -Raw } else { "" }
$materialBatchMappingDataAssetHeaderText = if (Test-Path -LiteralPath $materialBatchMappingDataAssetHeaderPath) { Get-Content -LiteralPath $materialBatchMappingDataAssetHeaderPath -Raw } else { "" }
$materialBatchAuditReportText = if (Test-Path -LiteralPath $materialBatchAuditReportPath) { Get-Content -LiteralPath $materialBatchAuditReportPath -Raw } else { "" }
$materialBatchBuildReportText = if (Test-Path -LiteralPath $materialBatchBuildReportPath) { Get-Content -LiteralPath $materialBatchBuildReportPath -Raw } else { "" }
$materialBatchBuildManifestText = if (Test-Path -LiteralPath $materialBatchBuildManifestPath) { Get-Content -LiteralPath $materialBatchBuildManifestPath -Raw } else { "" }
$materialBatchParentMaterialSetupReportText = if (Test-Path -LiteralPath $materialBatchParentMaterialSetupReportPath) { Get-Content -LiteralPath $materialBatchParentMaterialSetupReportPath -Raw } else { "" }
$materialBatchMaterialAuditReportText = if (Test-Path -LiteralPath $materialBatchMaterialAuditReportPath) { Get-Content -LiteralPath $materialBatchMaterialAuditReportPath -Raw } else { "" }
$ue58ScenePerformanceAuditReportText = if (Test-Path -LiteralPath $ue58ScenePerformanceAuditReportPath) { Get-Content -LiteralPath $ue58ScenePerformanceAuditReportPath -Raw } else { "" }
$ue58RuntimeProfilingPlanReportText = if (Test-Path -LiteralPath $ue58RuntimeProfilingPlanReportPath) { Get-Content -LiteralPath $ue58RuntimeProfilingPlanReportPath -Raw } else { "" }
$tierPlanText = if ($tierPlanMatch) { Get-Content -LiteralPath $tierPlanPath -Raw } else { "" }
$comprehensivePlanText = if ($comprehensivePlanMatch) { Get-Content -LiteralPath $comprehensivePlanPath -Raw } else { "" }
$graphicsSaveDetected = $settingsSaveText -match "FYogGraphicsSettings" -and $settingsSaveText -match "EYogPerformanceProfile"
$performanceLibraryDetected = $performanceLibraryHeaderText -match "ApplyPerformanceProfile" -and $performanceLibraryCppText -match "ApplyGraphicsSettings"
$customGraphicsSettingsDetected = $performanceLibraryHeaderText -match "MakeCustomGraphicsSettings" -and $performanceLibraryCppText -match "MakeCustomGraphicsSettings"
$frontendOptionsDetected = $gameInstanceCppText -match "PerformanceSettingsTitle" -and $gameInstanceCppText -match "ApplyPerformanceProfile"
$detailedFrontendOptionsDetected = $frontendOptionsDetected -and
    $gameInstanceCppText -match "PerformanceSettingsApplyCustom" -and
    $gameInstanceCppText -match "GraphicsResolutionScaleValue" -and
    $gameInstanceCppText -match "GraphicsFrameLimitValue" -and
    $gameInstanceCppText -match "GraphicsLumenLiteToggle" -and
    $gameInstanceCppText -match "GraphicsBatchProxyToggle"
$frontendPresetStateSyncDetected = $gameInstanceCppText -match "\.OnClicked_Lambda\(\[WeakThis, CurrentSettings, Profile\]" -and
    $gameInstanceCppText -match "MakeGraphicsSettingsForProfile\(Profile\)"
$performanceSettingsTestDetected = $performanceSettingsTestsText -match "DevKit.Performance.Settings.MakesClampedCustomSettings" -and
    $performanceSettingsTestsText -match "MakeCustomGraphicsSettings"
$graphicsSettingsWidgetBaseDetected = $graphicsSettingsWidgetHeaderText -match "UYogGraphicsSettingsWidgetBase" -and
    $graphicsSettingsWidgetCppText -match "GetRequiredDesignerWidgetNames" -and
    $graphicsSettingsWidgetCppText -match "ApplyPendingSettings"
$graphicsSettingsFrontendEntryDetected = $gameInstanceCppText -match "ShowGraphicsSettingsMenuWidget" -and
    $gameInstanceCppText -match "GraphicsSettingsMenuClass"
$graphicsSettingsWidgetSetupCommandletDetected = $graphicsSettingsWidgetSetupCommandletText -match "GraphicsSettingsWidgetSetupReport.md" -and
    $graphicsSettingsWidgetSetupCommandletText -match "/Game/UI/Frontend/WBP_GraphicsSettingsWidget"
$graphicsSettingsWidgetSetupTestDetected = $performanceSettingsTestsText -match "DevKit.Performance.Settings.GraphicsSettingsWidgetContract" -and
    $ue58ScenePerformanceTestsText -match "DevKitEditor.UI.GraphicsSettingsWidgetSetup.Contract"
$graphicsSettingsFocusContractDetected = $graphicsSettingsWidgetHeaderText -match "GetDefaultFocusWidgetName" -and
    $graphicsSettingsWidgetCppText -match "NativeGetDesiredFocusTarget" -and
    $graphicsSettingsWidgetCppText -match "GetDesiredInputConfig" -and
    $performanceSettingsTestsText -match "GetDefaultFocusWidgetName" -and
    $performanceSettingsTestsText -match "BtnApplyCustom"
$graphicsSettingsWidgetAssetDetected = Test-Path -LiteralPath $graphicsSettingsWidgetAssetPath
$graphicsSettingsWidgetSetupReportDetected = Test-Path -LiteralPath $graphicsSettingsWidgetSetupReportPath
$graphicsSettingsConfigDetected = $defaultGameText -match "GraphicsSettingsMenuClass=/Game/UI/Frontend/WBP_GraphicsSettingsWidget\.WBP_GraphicsSettingsWidget_C" -and
    $defaultGameText -match '\+DirectoriesToAlwaysCook=\(Path="/Game/UI/Frontend"\)'
$ue58MaterialMcpAuditReportDetected = Test-Path -LiteralPath $ue58MaterialMcpAuditReportPath
$ue58MaterialMcpAuditRequiredParamsDetected = $ue58MaterialMcpAuditReportText -match "Required batch parameters present: True" -and
    $ue58MaterialMcpAuditReportText -match "\| T_Array_A \| True \|" -and
    $ue58MaterialMcpAuditReportText -match "\| Tex_LightInfo \| True \|"
$ue58MaterialMcpAuditGraphDetected = $ue58MaterialMcpAuditReportText -match "MaterialAttributes connected: True" -and
    $ue58MaterialMcpAuditReportText -match "Expression count:" -and
    $ue58MaterialMcpAuditReportText -match "PerInstanceCustomData expression count:"
$ue58BatchVisualMcpAuditReportDetected = Test-Path -LiteralPath $ue58BatchVisualMcpAuditReportPath
$ue58BatchVisualMcpAuditCapturedDetected = $ue58BatchVisualMcpAuditReportText -match "Status: Captured" -and
    $ue58BatchVisualMcpAuditReportText -match "Source floor mesh" -and
    $ue58BatchVisualMcpAuditReportText -match "Generated batch proxy mesh" -and
    $ue58BatchVisualMcpAuditReportText -match "Generated batch material instance"
$ue58BatchVisualMcpAuditPngDetected = (Test-Path -LiteralPath (Join-Path $ue58BatchVisualMcpAuditRoot "source_floor_mesh.png")) -and
    (Test-Path -LiteralPath (Join-Path $ue58BatchVisualMcpAuditRoot "source_floor_material_instance.png")) -and
    (Test-Path -LiteralPath (Join-Path $ue58BatchVisualMcpAuditRoot "generated_batch_proxy_mesh.png")) -and
    (Test-Path -LiteralPath (Join-Path $ue58BatchVisualMcpAuditRoot "generated_batch_material_instance.png"))
$ue58SceneParityMcpAuditReportDetected = Test-Path -LiteralPath $ue58SceneParityMcpAuditReportPath
$ue58SceneParityMcpAuditPngDetected = (Test-Path -LiteralPath $ue58SceneParityMcpAuditPngPath) -and
    ((Get-Item -LiteralPath $ue58SceneParityMcpAuditPngPath -ErrorAction SilentlyContinue).Length -gt 512)
$ue58SceneParityMcpAuditReadyDetected = $ue58SceneParityMcpAuditReportText -match "(?m)^- Status: Captured\s*$" -and
    $ue58SceneParityMcpAuditReportText -match "(?m)^- Cleanup status: RemovedScratchActors\s*$" -and
    $ue58SceneParityMcpAuditReportText -match "(?m)^- Viewport bytes: [1-9][0-9]{3,}\s*$" -and
    $ue58SceneParityMcpAuditReportText -match "Scratch source asset:" -and
    $ue58SceneParityMcpAuditReportText -match "Scratch proxy asset:" -and
    $ue58SceneParityMcpAuditPngDetected
$ue58RuntimeProfilingMcpSmokeReportDetected = Test-Path -LiteralPath $ue58RuntimeProfilingMcpSmokeReportPath
$ue58RuntimeProfilingMcpSmokeReadyDetected = $ue58RuntimeProfilingMcpSmokeReportText -match "Status: Ready" -and
    $ue58RuntimeProfilingMcpSmokeReportText -match "sg\.GlobalIlluminationQuality.+True" -and
    $ue58RuntimeProfilingMcpSmokeReportText -match "r\.Lumen\.DiffuseIndirect\.Allow.+True" -and
    $ue58RuntimeProfilingMcpSmokeReportText -match "r\.MeshDrawCommands\.LogDynamicInstancingStats.+True"
$ue58RuntimeProfilingCaptureScriptDetected = $ue58RuntimeProfilingCaptureScriptText -match "UnrealEditor\.exe" -and
    $ue58RuntimeProfilingCaptureScriptText -match "-ExecCmds" -and
    $ue58RuntimeProfilingCaptureScriptText -match "Baseline_LumenOff_NoBatch" -and
    $ue58RuntimeProfilingCaptureScriptText -match "LumenLite_NoBatch" -and
    $ue58RuntimeProfilingCaptureScriptText -match "profilegpu"
$ue58RuntimeProfilingCaptureReportDetected = Test-Path -LiteralPath $ue58RuntimeProfilingCaptureReportPath
$ue58RuntimeProfilingCaptureReadyDetected = (
    ($ue58RuntimeProfilingCaptureReportText -match "(?m)^- Status: (Captured|Partial)\s*$" -and
        $ue58RuntimeProfilingCaptureReportText -match "\|\s*[1-9][0-9]*\s*\|") -or
    ($ue58RuntimeProfilingCaptureReportText -match "(?m)^- Status: ParsedLogCaptured\s*$" -and
        $ue58RuntimeProfilingCaptureReportText -match "Frame Time ms" -and
        $ue58RuntimeProfilingCaptureReportText -match "\|\s*[0-9]+(?:\.[0-9]+)?\s*\|\s*[0-9]+(?:\.[0-9]+)?\s*\|")
) -and
    $ue58RuntimeProfilingCaptureReportText -match "Baseline_LumenOff_NoBatch" -and
    $ue58RuntimeProfilingCaptureReportText -match "LumenLite_NoBatch"
$materialBatchAuditDetected = $materialBatchAuditCommandletText -match "MaterialBatchAuditReport.md" -and $materialBatchCandidateRulesText -match "ClassifyComponent"
$materialBatchBuildDetected = $materialBatchBuildCommandletText -match "MaterialBatchBuildReport.md" -and
    $materialBatchBuildPlanText -match "CreateDryRunPlan"
$materialBatchMappingDataAssetTypeDetected = $materialBatchMappingDataAssetHeaderText -match "UMaterialBatchMappingDataAsset" -and
    $materialBatchMappingDataAssetHeaderText -match "FMaterialBatchMappingGeometrySource" -and
    $materialBatchMappingDataAssetHeaderText -match "FMaterialBatchMappingPropertyRow"
$materialBatchBuildApplyMappingOnlyDetected = $materialBatchBuildCommandletText -match "ApplyMappingOnly" -and
    $materialBatchBuildCommandletText -match "SaveMappingDataAsset" -and
    $materialBatchBuildPlanText -match "PopulateMappingDataAsset"
$materialBatchBuildApplyTextureArraysOnlyDetected = $materialBatchBuildCommandletText -match "ApplyTextureArraysOnly" -and
    $materialBatchBuildCommandletText -match "SaveTextureArrayAssets" -and
    $materialBatchBuildPlanText -match "BuildTextureArrayPayloads"
$materialBatchBuildApplyPropertyTextureOnlyDetected = $materialBatchBuildCommandletText -match "ApplyPropertyTextureOnly" -and
    $materialBatchBuildCommandletText -match "SavePropertyTextureAsset" -and
    $materialBatchBuildPlanText -match "BuildPropertyTexturePayload"
$materialBatchBuildApplyProxyMeshOnlyDetected = $materialBatchBuildCommandletText -match "ApplyProxyMeshOnly" -and
    $materialBatchBuildCommandletText -match "SaveProxyMeshAsset" -and
    $materialBatchBuildPlanText -match "BuildProxyMeshPayload"
$materialBatchBuildApplyBatchMaterialOnlyDetected = $materialBatchBuildCommandletText -match "ApplyBatchMaterialOnly" -and
    $materialBatchBuildCommandletText -match "SaveBatchMaterialInstanceAsset" -and
    $materialBatchBuildPlanText -match "BuildBatchMaterialPayload"
$materialBatchMaterialAuditDetected = $materialBatchMaterialAuditCommandletText -match "MaterialBatchMaterialAuditReport.md" -and
    $materialBatchCandidateTestsText -match "DevKitEditor.MaterialBatch.MaterialAudit.LoadsTargetEnvBuildingMaterial"
$materialBatchAuditTestDetected = $materialBatchCandidateTestsText -match "DevKitEditor.MaterialBatch.CandidateRules.ClassifiesStaticMeshInputs"
$materialBatchBuildTestDetected = $materialBatchCandidateTestsText -match "DevKitEditor.MaterialBatch.BuildPlan.CreatesDryRunGeneratedAssetNames"
$materialBatchMappingDataAssetTestDetected = $materialBatchCandidateTestsText -match "DevKitEditor.MaterialBatch.BuildPlan.PopulatesMappingDataAsset"
$ue58RuntimeProfilingPlanDetected = $ue58RuntimeProfilingPlanCommandletText -match "UE58RuntimeProfilingPlanReport.md" -and
    $ue58RuntimeProfilingPlanText -match "BuildDefaultScenarios" -and
    $ue58RuntimeProfilingPlanText -match "BatchProxy_LumenLite"
$ue58RuntimeProfilingPlanTestDetected = $ue58ScenePerformanceTestsText -match "DevKitEditor.UE58Performance.RuntimeProfiling.BuildsScenarioMatrix"
$materialBatchAuditReportDetected = Test-Path -LiteralPath $materialBatchAuditReportPath
$materialBatchBuildReportDetected = Test-Path -LiteralPath $materialBatchBuildReportPath
$materialBatchBuildManifestDetected = Test-Path -LiteralPath $materialBatchBuildManifestPath
$materialBatchMaterialAuditReportDetected = Test-Path -LiteralPath $materialBatchMaterialAuditReportPath
$ue58ScenePerformanceAuditReportDetected = Test-Path -LiteralPath $ue58ScenePerformanceAuditReportPath
$ue58RuntimeProfilingPlanReportDetected = Test-Path -LiteralPath $ue58RuntimeProfilingPlanReportPath
$ue58ScenePerformanceAuditLoadedDetected = $ue58ScenePerformanceAuditReportText -match "- Loaded: Yes"
$ue58ScenePerformanceAuditStaticMeshDetected = $ue58ScenePerformanceAuditReportText -match "StaticMesh components:" -and
    $ue58ScenePerformanceAuditReportText -match "StaticMesh material slot upper-bound:"
$ue58ScenePerformanceAuditLightDetected = $ue58ScenePerformanceAuditReportText -match "Light components:" -and
    $ue58ScenePerformanceAuditReportText -match "Movable lights:"
$ue58RuntimeProfilingPlanMatrixDetected = $ue58RuntimeProfilingPlanReportText -match "## Scenario Matrix" -and
    $ue58RuntimeProfilingPlanReportText -match "Baseline_LumenOff_NoBatch" -and
    $ue58RuntimeProfilingPlanReportText -match "BatchProxy_LumenLite"
$ue58RuntimeProfilingPlanCaptureDetected = $ue58RuntimeProfilingPlanReportText -match "stat rhi" -and
    $ue58RuntimeProfilingPlanReportText -match "stat gpu" -and
    $ue58RuntimeProfilingPlanReportText -match "profilegpu"
$ue58RuntimeProfilingPlanNotMeasuredDetected = $ue58RuntimeProfilingPlanReportText -match "Evidence status: NotMeasured" -and
    $ue58RuntimeProfilingPlanReportText -match "Mesh draw calls \| NotMeasured"
$materialBatchMaterialAuditLoadedDetected = $materialBatchMaterialAuditReportText -match "- Loaded: Yes" -and
    $materialBatchMaterialAuditReportText -match "M_Env_Building"
$materialBatchMaterialAuditTextureParametersDetected = $materialBatchMaterialAuditReportText -match "## Texture Parameters" -and
    $materialBatchMaterialAuditReportText -match "T Unique Color Map" -and
    $materialBatchMaterialAuditReportText -match "T_Array_A"
$materialBatchMaterialAuditScalarParametersDetected = $materialBatchMaterialAuditReportText -match "## Scalar Parameters" -and
    $materialBatchMaterialAuditReportText -match "LightInfoCount"
$materialBatchMaterialAuditVectorParametersDetected = $materialBatchMaterialAuditReportText -match "## Vector Parameters" -and
    $materialBatchMaterialAuditReportText -match "Unique ColorTint"
$materialBatchMaterialAuditStaticSwitchDetected = $materialBatchMaterialAuditReportText -match "## Static Switch Parameters" -and
    $materialBatchMaterialAuditReportText -match "UseFakeLightInfo"
$materialBatchMaterialAuditTextureArrayDetected = $materialBatchMaterialAuditReportText -match "Texture2DArray"
$materialBatchMaterialAuditLightInfoDetected = $materialBatchMaterialAuditReportText -match "Tex_LightInfo" -and
    $materialBatchMaterialAuditReportText -match "LightInfoCount"
$materialBatchBuildEntriesDetected = $materialBatchBuildReportText -match "## Planned Batch Entries" -and
    $materialBatchBuildReportText -match "FirstBatchMaterialIndex"
$materialBatchBuildManifestSchemaDetected = $materialBatchBuildManifestText -match '"schema"\s*:\s*"DevKit\.MaterialBatchBuildPlan\.v1"' -and
    $materialBatchBuildManifestText -match '"entries"\s*:'
$materialBatchBuildManifestMaterialRowsDetected = $materialBatchBuildManifestSchemaDetected -and
    $materialBatchBuildManifestText -match '"materialRows"\s*:' -and
    $materialBatchBuildManifestText -match '"batchMaterialIndex"\s*:'
$materialBatchBuildManifestMaterialSourcesDetected = $materialBatchBuildManifestMaterialRowsDetected -and
    $materialBatchBuildManifestText -match '"materialSlotName"\s*:' -and
    $materialBatchBuildManifestText -match '"material"\s*:'
$materialBatchBuildManifestTextureChannelsDetected = $materialBatchBuildManifestMaterialRowsDetected -and
    $materialBatchBuildManifestText -match '"textureChannels"\s*:'
$materialBatchBuildManifestTextureChannelValuesDetected = $materialBatchBuildManifestTextureChannelsDetected -and
    $materialBatchBuildManifestText -match '"channel"\s*:' -and
    $materialBatchBuildManifestText -match '"parameter"\s*:' -and
    $materialBatchBuildManifestText -match '"texture"\s*:'
$materialBatchBuildReportTextureArraySlicesDetected = $materialBatchBuildReportText -match "## Planned Texture2DArray Slices" -and
    $materialBatchBuildReportText -match "SliceIndex"
$materialBatchBuildReportTextureArrayEligibilityDetected = $materialBatchBuildReportText -match "## Texture2DArray Build Eligibility" -and
    $materialBatchBuildReportText -match "Eligible" -and
    $materialBatchBuildReportText -match "Reason"
$materialBatchBuildManifestTextureArraysDetected = $materialBatchBuildManifestSchemaDetected -and
    $materialBatchBuildManifestText -match '"textureArrays"\s*:' -and
    $materialBatchBuildManifestText -match '"baseColor"\s*:' -and
    $materialBatchBuildManifestText -match '"normal"\s*:' -and
    $materialBatchBuildManifestText -match '"orm"\s*:'
$materialBatchBuildManifestTextureArrayValuesDetected = $materialBatchBuildManifestTextureArraysDetected -and
    $materialBatchBuildManifestText -match '"sliceIndex"\s*:'
$materialBatchBuildManifestTextureArrayEligibilityValuesDetected = $materialBatchBuildManifestTextureChannelsDetected -and
    $materialBatchBuildManifestText -match '"arrayBuildEligible"\s*:' -and
    $materialBatchBuildManifestText -match '"arrayBuildReason"\s*:'
$materialBatchBuildReportPropertyRowsDetected = $materialBatchBuildReportText -match "## Planned Property Texture Rows" -and
    $materialBatchBuildReportText -match "BaseColorSlice" -and
    $materialBatchBuildReportText -match "NormalSlice"
$materialBatchBuildManifestPropertyRowsDetected = $materialBatchBuildManifestSchemaDetected -and
    $materialBatchBuildManifestText -match '"propertyRows"\s*:' -and
    $materialBatchBuildManifestText -match '"baseColorSlice"\s*:' -and
    $materialBatchBuildManifestText -match '"normalSlice"\s*:' -and
    $materialBatchBuildManifestText -match '"ormSlice"\s*:'
$materialBatchBuildManifestPropertyTextureLayoutDetected = $materialBatchBuildManifestSchemaDetected -and
    $materialBatchBuildManifestText -match '"propertyTextureLayout"\s*:' -and
    $materialBatchBuildManifestText -match '"rowKey"\s*:\s*"batchMaterialIndex"' -and
    $materialBatchBuildManifestText -match '"columns"\s*:' -and
    $materialBatchBuildManifestText -match '"sourceField"\s*:\s*"baseColorSlice"'
$materialBatchBuildReportPropertyTextureLayoutDetected = $materialBatchBuildReportText -match "## Property Texture Layout" -and
    $materialBatchBuildReportText -match "BaseColorSlice" -and
    $materialBatchBuildReportText -match "batchMaterialIndex"
$materialBatchBuildParentContractSourceDetected = $materialBatchBuildPlanText -match "M_Env_Building_Batch" -and
    $materialBatchBuildPlanText -match "Batch Material Parent Contract" -and
    $materialBatchBuildPlanText -match "_PropTexture" -and
    $materialBatchBuildPlanText -match "TexCoord7\.x" -and
    $materialBatchCandidateTestsText -match "BuildsBatchMaterialPayload"
$materialBatchBuildReportParentContractDetected = $materialBatchBuildReportText -match "## Batch Material Parent Contract" -and
    $materialBatchBuildReportText -match "M_Env_Building_Batch" -and
    $materialBatchBuildReportText -match "_PropTexture"
$materialBatchBuildManifestParentContractDetected = $materialBatchBuildManifestSchemaDetected -and
    $materialBatchBuildManifestText -match '"batchMaterialContract"\s*:' -and
    $materialBatchBuildManifestText -match '"batchParentMaterial"\s*:\s*"/Game/Art/Material/EnvMaterial/Main/M_Env_Building_Batch\.M_Env_Building_Batch"' -and
    $materialBatchBuildManifestText -match '"propertyTextureParameter"\s*:\s*"_PropTexture"'
$materialBatchParentMaterialSetupCommandletDetected = $materialBatchParentMaterialSetupCommandletText -match "MaterialBatchParentMaterialSetupReport.md" -and
    $materialBatchParentMaterialSetupCommandletText -match "Texture2DArraySample" -and
    $materialBatchParentMaterialSetupCommandletText -match "TexCoord7"
$materialBatchParentMaterialAssetDetected = Test-Path -LiteralPath $materialBatchParentMaterialAssetPath
$materialBatchParentMaterialSetupReportDetected = $materialBatchParentMaterialSetupReportText -match "# Material Batch Parent Material Setup" -and
    $materialBatchParentMaterialSetupReportText -match "M_Env_Building_Batch" -and
    $materialBatchParentMaterialSetupReportText -match "TextureCoordinate index 7 \| Yes" -and
    $materialBatchParentMaterialSetupReportText -match 'Custom HLSL `Texture2DArraySample` \| Yes' -and
    $materialBatchParentMaterialSetupReportText -match 'Custom HLSL `_PropTexture` sample \| Yes'
$materialBatchBuildReportGeometryMergePlanDetected = $materialBatchBuildReportText -match "## Geometry Merge Plan" -and
    $materialBatchBuildReportText -match "TexCoord7.x" -and
    $materialBatchBuildReportText -match "Target proxy mesh"
$materialBatchBuildReportMaterialSlotRemapDetected = $materialBatchBuildReportText -match "## Material Slot Remap" -and
    $materialBatchBuildReportText -match "BatchMaterialIndex"
$materialBatchBuildReportMappingAssetSavedDetected = $materialBatchBuildReportText -match "## Mapping Data Asset" -and
    $materialBatchBuildReportText -match "Saved:" -and
    $materialBatchBuildReportText -match "DA_MaterialBatchMap_FloorBrick03_Probe"
$materialBatchGeneratedMappingAssetDetected = Test-Path -LiteralPath $materialBatchGeneratedMappingAssetPath
$materialBatchBuildReportTextureArraysSavedDetected = $materialBatchBuildReportText -match "## Texture2DArray Assets" -and
    $materialBatchBuildReportText -match "Saved BaseColor:" -and
    $materialBatchBuildReportText -match "T2DA_"
$materialBatchGeneratedTextureArraysCount = Count-ByNamePattern -Path $materialBatchGeneratedTextureArrayRoot -Pattern "^T2DA_"
$materialBatchGeneratedTextureArraysDetected = $materialBatchGeneratedTextureArraysCount -gt 0
$materialBatchBuildReportPropertyTextureSavedDetected = $materialBatchBuildReportText -match "## Property Texture Asset" -and
    $materialBatchBuildReportText -match "Saved:" -and
    $materialBatchBuildReportText -match "T_PropTexture_FloorBrick03_Probe"
$materialBatchGeneratedPropertyTextureDetected = Test-Path -LiteralPath $materialBatchGeneratedPropertyTexturePath
$materialBatchBuildReportProxyMeshSavedDetected = $materialBatchBuildReportText -match "## Proxy Mesh Asset" -and
    $materialBatchBuildReportText -match "Saved:" -and
    $materialBatchBuildReportText -match "SM_BatchProxy_FloorBrick03_Probe"
$materialBatchGeneratedProxyMeshDetected = Test-Path -LiteralPath $materialBatchGeneratedProxyMeshPath
$materialBatchBuildReportBatchMaterialSavedDetected = $materialBatchBuildReportText -match "## Batch Material Instance" -and
    $materialBatchBuildReportText -match "Saved:" -and
    $materialBatchBuildReportText -match "MI_Env_Batch_FloorBrick03_Probe"
$materialBatchBuildReportBatchMaterialBoundDetected = $materialBatchBuildReportText -match "## Batch Material Instance" -and
    $materialBatchBuildReportText -match 'Bound `T_Array_A`' -and
    $materialBatchBuildReportText -match "Bound proxy mesh slot 0"
$materialBatchGeneratedBatchMaterialDetected = Test-Path -LiteralPath $materialBatchGeneratedBatchMaterialPath
$materialBatchBuildManifestGeometryMergePlanDetected = $materialBatchBuildManifestSchemaDetected -and
    $materialBatchBuildManifestText -match '"geometryMergePlan"\s*:' -and
    $materialBatchBuildManifestText -match '"targetProxyMesh"\s*:' -and
    $materialBatchBuildManifestText -match '"materialIndexChannel"\s*:\s*"TexCoord7\.x"' -and
    $materialBatchBuildManifestText -match '"sources"\s*:'
$materialBatchBuildManifestMaterialSlotRemapDetected = $materialBatchBuildManifestGeometryMergePlanDetected -and
    $materialBatchBuildManifestText -match '"materialSlotRemap"\s*:' -and
    $materialBatchBuildManifestText -match '"sourceMaterialSlotIndex"\s*:' -and
    $materialBatchBuildManifestText -match '"batchMaterialIndex"\s*:'
$tierPlanDetected = $tierPlanText -match "Geometry Merge" -and $tierPlanText -match "Texture2DArray" -and $tierPlanText -match "Player Graphics Settings"
$comprehensivePlanDetected = $comprehensivePlanText -match "Texture2DArray" -and $comprehensivePlanText -match "VT/SVT" -and $comprehensivePlanText -match "Geometry Merge" -and $comprehensivePlanText -match "MaterialBatchBuild"

$materialBatchReportRows = @(
    [pscustomobject]@{ Metric = "Root"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "Root" },
    [pscustomobject]@{ Metric = "Map"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "Map" },
    [pscustomobject]@{ Metric = "StaticMesh assets found"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "StaticMesh assets found" },
    [pscustomobject]@{ Metric = "StaticMesh assets inspected"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "StaticMesh assets inspected" },
    [pscustomobject]@{ Metric = "StaticMesh components found"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "StaticMesh components found" },
    [pscustomobject]@{ Metric = "StaticMesh components inspected"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "StaticMesh components inspected" },
    [pscustomobject]@{ Metric = "Batch candidates"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "Batch candidates" },
    [pscustomobject]@{ Metric = "Rejected"; Value = Get-MarkdownBulletValue -Text $materialBatchAuditReportText -Label "Rejected" }
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.Value) }

$riskRows = @()
if ((Get-ConfigValue -Lines $defaultEngineLines -Section $rendererSection -Key "r.DynamicGlobalIlluminationMethod") -eq "0") {
    $riskRows += "Lumen is disabled in project defaults; Lumen Lite must be validated through an explicit profile or test cvar set."
}
if ((Get-ConfigValue -Lines $defaultEngineLines -Section $rendererSection -Key "r.Nanite.ProjectEnabled") -eq "False") {
    $riskRows += "Nanite is disabled in project defaults; handheld batching should not depend on Nanite until a separate experiment proves value."
}
if (-not (Test-Path -LiteralPath $defaultScalabilityPath)) {
    $riskRows += "Config/DefaultScalability.ini is missing; platform tiers do not yet have source-controlled scalability cvars."
}
if (-not (Test-Path -LiteralPath $defaultDeviceProfilesPath)) {
    $riskRows += "Config/DefaultDeviceProfiles.ini is missing; platform/device profile routing is not yet source-controlled."
}
elseif ($devKitProfiles.Count -lt 4) {
    $riskRows += "DefaultDeviceProfiles.ini exists, but fewer than four DevKit_* test profiles were detected."
}
if ((Test-Path -LiteralPath $defaultScalabilityPath) -and $scalabilitySections.Count -lt 20) {
    $riskRows += "DefaultScalability.ini exists, but the expected tier section coverage looks incomplete."
}
if (-not (Test-Path -LiteralPath $targetMaterialPath)) {
    $riskRows += "Target sample material M_Env_Building was not found at the expected path."
}
if (-not $tierPlanDetected) {
    $riskRows += "English UE58 tier plan was not detected or is missing key batching/settings sections."
}
if (-not $comprehensivePlanDetected) {
    $riskRows += "Chinese comprehensive art/performance plan was not detected or is missing key VT, batching, or art handoff sections."
}
if (-not $mcpConfigured) {
    $riskRows += "UE MCP project settings were not detected on port 8765."
}
if (-not $graphicsSaveDetected) {
    $riskRows += "Graphics performance settings were not detected in UYogSettingsSave."
}
if (-not $performanceLibraryDetected) {
    $riskRows += "Performance profile application library was not detected."
}
if (-not $customGraphicsSettingsDetected) {
    $riskRows += "Custom graphics settings builder was not detected in UYogPerformanceSettingsLibrary."
}
if (-not $frontendOptionsDetected) {
    $riskRows += "Frontend Options screen is not wired to performance profile application."
}
if (-not $detailedFrontendOptionsDetected) {
    $riskRows += "Frontend Options screen is missing detailed custom graphics controls."
}
if (-not $frontendPresetStateSyncDetected) {
    $riskRows += "Frontend preset buttons do not update the local custom graphics state shown on the same Options screen."
}
if (-not $performanceSettingsTestDetected) {
    $riskRows += "Performance settings automation test was not detected."
}
if (-not $graphicsSettingsWidgetBaseDetected) {
    $riskRows += "UYogGraphicsSettingsWidgetBase was not detected; player graphics settings UI lacks a reusable UMG base."
}
if (-not $graphicsSettingsFrontendEntryDetected) {
    $riskRows += "Frontend Options is not wired to the graphics settings UMG widget path."
}
if (-not $graphicsSettingsWidgetSetupCommandletDetected) {
    $riskRows += "GraphicsSettingsWidgetSetup commandlet was not detected."
}
if (-not $graphicsSettingsWidgetSetupTestDetected) {
    $riskRows += "Graphics settings widget automation contracts were not detected."
}
if (-not $graphicsSettingsFocusContractDetected) {
    $riskRows += "Graphics settings widget focus/input contract was not detected; controller navigation may not have a stable default target."
}
if (-not $graphicsSettingsWidgetAssetDetected) {
    $riskRows += "Generated WBP_GraphicsSettingsWidget asset was not found under Content/UI/Frontend."
}
if (-not $graphicsSettingsWidgetSetupReportDetected) {
    $riskRows += "GraphicsSettingsWidgetSetupReport.md was not found."
}
if (-not $graphicsSettingsConfigDetected) {
    $riskRows += "DefaultGame.ini is missing the graphics settings widget class or cook directory config."
}
if (-not $ue58MaterialMcpAuditReportDetected) {
    $riskRows += "UE58MaterialMcpAudit_LATEST.md was not found; run Invoke-UE58MaterialMcpAudit.ps1 while UE5.8 MCP is listening."
}
if ($ue58MaterialMcpAuditReportDetected -and -not $ue58MaterialMcpAuditRequiredParamsDetected) {
    $riskRows += "UE58MaterialMcpAudit_LATEST.md exists but does not prove the required batch material parameters are exposed."
}
if ($ue58MaterialMcpAuditReportDetected -and -not $ue58MaterialMcpAuditGraphDetected) {
    $riskRows += "UE58MaterialMcpAudit_LATEST.md exists but does not prove MaterialAttributes and expression graph evidence."
}
if (-not $materialBatchAuditDetected) {
    $riskRows += "MaterialBatchAudit commandlet and candidate rules were not detected."
}
if (-not $materialBatchBuildDetected) {
    $riskRows += "MaterialBatchBuild dry-run commandlet and plan builder were not detected."
}
if (-not $materialBatchMaterialAuditDetected) {
    $riskRows += "MaterialBatchMaterialAudit commandlet and target-material tests were not detected."
}
if (-not $materialBatchAuditTestDetected) {
    $riskRows += "Material batch candidate rule automation test was not detected."
}
if (-not $materialBatchBuildTestDetected) {
    $riskRows += "Material batch build plan automation test was not detected."
}
if (-not $materialBatchMappingDataAssetTestDetected) {
    $riskRows += "Material batch mapping data asset automation test was not detected."
}
if (-not $materialBatchAuditReportDetected) {
    $riskRows += "MaterialBatchAuditReport.md has not been generated yet."
}
if (-not $materialBatchBuildReportDetected) {
    $riskRows += "MaterialBatchBuildReport.md has not been generated yet."
}
if (-not $materialBatchMaterialAuditReportDetected) {
    $riskRows += "MaterialBatchMaterialAuditReport.md has not been generated yet."
}
elseif (-not $materialBatchMaterialAuditLoadedDetected) {
    $riskRows += "MaterialBatchMaterialAuditReport.md exists but does not show M_Env_Building loaded successfully."
}
elseif (-not ($materialBatchMaterialAuditTextureParametersDetected -and $materialBatchMaterialAuditScalarParametersDetected -and $materialBatchMaterialAuditVectorParametersDetected -and $materialBatchMaterialAuditStaticSwitchDetected)) {
    $riskRows += "MaterialBatchMaterialAuditReport.md exists but does not include the expected texture/scalar/vector/static switch evidence."
}
if ($materialBatchBuildReportDetected -and -not $materialBatchBuildEntriesDetected) {
    $riskRows += "MaterialBatchBuildReport.md does not include planned batch entries or BatchMaterialIndex allocation."
}
if (-not $materialBatchBuildManifestDetected) {
    $riskRows += "MaterialBatchBuildManifest.json has not been generated yet."
}
elseif (-not $materialBatchBuildManifestSchemaDetected) {
    $riskRows += "MaterialBatchBuildManifest.json exists but does not contain the expected schema or entries array."
}
elseif (-not $materialBatchBuildManifestMaterialRowsDetected) {
    $riskRows += "MaterialBatchBuildManifest.json exists but does not contain materialRows with batch material indices."
}
elseif (-not $materialBatchBuildManifestMaterialSourcesDetected) {
    $riskRows += "MaterialBatchBuildManifest.json materialRows do not include source material paths and material slot names."
}
elseif (-not $materialBatchBuildManifestTextureChannelsDetected) {
    $riskRows += "MaterialBatchBuildManifest.json materialRows do not include textureChannels arrays for automatic Texture2DArray planning."
}
elseif (-not $materialBatchBuildReportTextureArrayEligibilityDetected) {
    $riskRows += "MaterialBatchBuildReport.md does not include Texture2DArray build eligibility and rejection reasons."
}
elseif (-not $materialBatchBuildManifestTextureArraysDetected) {
    $riskRows += "MaterialBatchBuildManifest.json does not include textureArrays slice plans for automatic Texture2DArray generation."
}
elseif (-not $materialBatchBuildManifestPropertyRowsDetected) {
    $riskRows += "MaterialBatchBuildManifest.json does not include propertyRows with planned Texture2DArray slice indices."
}
elseif (-not $materialBatchBuildManifestPropertyTextureLayoutDetected) {
    $riskRows += "MaterialBatchBuildManifest.json does not include propertyTextureLayout for deterministic property texture generation."
}
elseif (-not $materialBatchBuildParentContractSourceDetected) {
    $riskRows += "MaterialBatchBuild does not source-control the final M_Env_Building_Batch parent material contract."
}
elseif (-not $materialBatchBuildReportParentContractDetected) {
    $riskRows += "MaterialBatchBuildReport.md does not include the final batch parent material contract."
}
elseif (-not $materialBatchBuildManifestParentContractDetected) {
    $riskRows += "MaterialBatchBuildManifest.json does not include the final batch parent material contract."
}
elseif (-not $materialBatchParentMaterialSetupCommandletDetected) {
    $riskRows += "MaterialBatchParentMaterialSetup commandlet was not detected; final batch parent material asset cannot be generated repeatably."
}
elseif (-not $materialBatchParentMaterialAssetDetected) {
    $riskRows += "M_Env_Building_Batch.uasset was not found; run MaterialBatchParentMaterialSetup -Apply before production batch material binding."
}
elseif (-not $materialBatchParentMaterialSetupReportDetected) {
    $riskRows += "MaterialBatchParentMaterialSetupReport.md does not prove TexCoord7.x, _PropTexture, and Texture2DArraySample evidence for M_Env_Building_Batch."
}
elseif (-not $materialBatchBuildManifestGeometryMergePlanDetected) {
    $riskRows += "MaterialBatchBuildManifest.json does not include geometryMergePlan for deterministic proxy mesh generation."
}
elseif (-not $materialBatchBuildManifestMaterialSlotRemapDetected) {
    $riskRows += "MaterialBatchBuildManifest.json geometryMergePlan does not include materialSlotRemap data for UV material index writing."
}
elseif (-not $materialBatchMappingDataAssetTypeDetected) {
    $riskRows += "UMaterialBatchMappingDataAsset was not detected; generated batch mapping cannot be consumed by runtime/editor tools."
}
elseif (-not $materialBatchBuildApplyMappingOnlyDetected) {
    $riskRows += "MaterialBatchBuild does not expose ApplyMappingOnly mapping asset generation."
}
elseif (-not $materialBatchBuildApplyTextureArraysOnlyDetected) {
    $riskRows += "MaterialBatchBuild does not expose ApplyTextureArraysOnly Texture2DArray generation."
}
elseif (-not $materialBatchGeneratedMappingAssetDetected) {
    $riskRows += "Generated mapping data asset was not found under Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe."
}
elseif (-not $materialBatchGeneratedTextureArraysDetected) {
    $riskRows += "Generated Texture2DArray assets were not found under Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe; run ApplyTextureArraysOnly on a real eligible art cluster."
}
elseif (-not $materialBatchBuildApplyPropertyTextureOnlyDetected) {
    $riskRows += "MaterialBatchBuild does not expose ApplyPropertyTextureOnly property texture generation."
}
elseif (-not $materialBatchGeneratedPropertyTextureDetected) {
    $riskRows += "Generated property texture asset was not found under Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe."
}
elseif (-not $materialBatchBuildApplyProxyMeshOnlyDetected) {
    $riskRows += "MaterialBatchBuild does not expose ApplyProxyMeshOnly proxy mesh generation."
}
elseif (-not $materialBatchGeneratedProxyMeshDetected) {
    $riskRows += "Generated proxy mesh asset was not found under Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe."
}
elseif (-not $materialBatchBuildApplyBatchMaterialOnlyDetected) {
    $riskRows += "MaterialBatchBuild does not expose ApplyBatchMaterialOnly batch material instance generation."
}
elseif (-not $materialBatchGeneratedBatchMaterialDetected) {
    $riskRows += "Generated batch material instance was not found under Content/Generated/MaterialBatch/Medium/FloorBrick03_Probe."
}
elseif (-not $materialBatchBuildReportBatchMaterialSavedDetected) {
    $riskRows += "MaterialBatchBuildReport.md does not record the saved batch material instance path."
}
elseif (-not $materialBatchBuildReportBatchMaterialBoundDetected) {
    $riskRows += "MaterialBatchBuildReport.md does not record Texture2DArray parameter bindings and proxy mesh material assignment."
}
elseif (-not ($ue58BatchVisualMcpAuditCapturedDetected -and $ue58BatchVisualMcpAuditPngDetected)) {
    $riskRows += "UE58 batch visual MCP audit does not yet prove source and generated batch assets can render non-empty thumbnail captures."
}
elseif (-not $ue58SceneParityMcpAuditReadyDetected) {
    $riskRows += "UE58 scene parity MCP audit is missing or incomplete; run Invoke-UE58SceneParityMcpAudit.ps1 to capture source/proxy meshes side by side in a target level context."
}
elseif (-not $ue58ScenePerformanceAuditReportDetected) {
    $riskRows += "UE58ScenePerformanceAuditReport.md was not found; run UE58ScenePerformanceAudit on a representative map."
}
elseif (-not $ue58ScenePerformanceAuditLoadedDetected) {
    $riskRows += "UE58ScenePerformanceAuditReport.md does not show a loaded representative map."
}
elseif (-not $ue58RuntimeProfilingPlanDetected) {
    $riskRows += "UE58RuntimeProfilingPlan commandlet was not detected; runtime profiling scenarios are not yet source-controlled."
}
elseif (-not $ue58RuntimeProfilingPlanTestDetected) {
    $riskRows += "UE58RuntimeProfilingPlan automation test was not detected."
}
elseif (-not $ue58RuntimeProfilingPlanReportDetected) {
    $riskRows += "UE58RuntimeProfilingPlanReport.md was not found; generate the runtime profiling scenario checklist before measurement."
}
elseif (-not $ue58RuntimeProfilingPlanMatrixDetected) {
    $riskRows += "UE58RuntimeProfilingPlanReport.md does not include the expected baseline, Lumen Lite, and batch proxy scenario matrix."
}
elseif (-not $ue58RuntimeProfilingPlanCaptureDetected) {
    $riskRows += "UE58RuntimeProfilingPlanReport.md does not include the required stat/profilegpu capture commands."
}
elseif (-not $ue58RuntimeProfilingMcpSmokeReadyDetected) {
    $riskRows += "UE58 runtime profiling MCP smoke is missing or not ready; run Invoke-UE58RuntimeProfilingMcpSmoke.ps1 while the UE5.8 MCP server is listening."
}
elseif (-not $ue58RuntimeProfilingCaptureScriptDetected) {
    $riskRows += "UE58 runtime profiling capture automation is missing; add the Codex script that launches UE5.8 with scenario CVars and profilegpu ExecCmds."
}
elseif (-not $ue58RuntimeProfilingCaptureReadyDetected) {
    if ($ue58RuntimeProfilingCaptureReportText -match "(?m)^- Status: LogCaptured\s*$") {
        $riskRows += "UE58 runtime profiling capture produced baseline/Lumen logs, but no ProfileGPU artifact was found; capture needs a follow-up path that emits a GPU profile file or parseable frame metrics."
    }
    else {
        $riskRows += "UE58 runtime profiling capture is prepared but not measured; run Invoke-UE58RuntimeProfilingCapture.ps1 -Run for baseline and Lumen Lite evidence."
    }
}

if ($riskRows.Count -eq 0) {
    $riskRows += "No static configuration risk was detected by this lightweight audit. Asset-level and GPU-level profiling still required."
}

$rendererTable = @("| Key | Value |", "| --- | --- |")
foreach ($row in $rendererRows) {
    $value = if ([string]::IsNullOrWhiteSpace($row.Value)) { "(not set)" } else { $row.Value }
    $rendererTable += "| $($row.Key) | $value |"
}

$countTable = @("| Item | Count |", "| --- | ---: |")
foreach ($entry in $counts.GetEnumerator()) {
    $countTable += "| $($entry.Key) | $($entry.Value) |"
}

$riskList = $riskRows | ForEach-Object { "- $_" }

$materialBatchReportTable = @("| Metric | Value |", "| --- | --- |")
if ($materialBatchReportRows.Count -gt 0) {
    foreach ($row in $materialBatchReportRows) {
        $materialBatchReportTable += "| $($row.Metric) | $($row.Value) |"
    }
}
else {
    $materialBatchReportTable += "| Report summary | (not available) |"
}

$materialBatchBuildReportRows = @(
    [pscustomobject]@{ Metric = "Root"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Root" },
    [pscustomobject]@{ Metric = "Map"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Map" },
    [pscustomobject]@{ Metric = "DataLayer"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "DataLayer" },
    [pscustomobject]@{ Metric = "Cluster"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Cluster" },
    [pscustomobject]@{ Metric = "Tier"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Tier" },
    [pscustomobject]@{ Metric = "OutputRoot"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "OutputRoot" },
    [pscustomobject]@{ Metric = "Mode"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Mode" },
    [pscustomobject]@{ Metric = "Source kind"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Source kind" },
    [pscustomobject]@{ Metric = "Source found"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Source found" },
    [pscustomobject]@{ Metric = "Source inspected"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Source inspected" },
    [pscustomobject]@{ Metric = "Batch candidates"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Batch candidates" },
    [pscustomobject]@{ Metric = "Rejected"; Value = Get-MarkdownBulletValue -Text $materialBatchBuildReportText -Label "Rejected" },
    [pscustomobject]@{ Metric = "Planned entries"; Value = if ($materialBatchBuildEntriesDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON manifest"; Value = if ($materialBatchBuildManifestDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON manifest schema"; Value = if ($materialBatchBuildManifestSchemaDetected) { "DevKit.MaterialBatchBuildPlan.v1" } else { "" } },
    [pscustomobject]@{ Metric = "JSON material rows"; Value = if ($materialBatchBuildManifestMaterialRowsDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON material sources"; Value = if ($materialBatchBuildManifestMaterialSourcesDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON texture channel arrays"; Value = if ($materialBatchBuildManifestTextureChannelsDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON texture channel values"; Value = if ($materialBatchBuildManifestTextureChannelValuesDetected) { "present" } else { "not found in current sample" } },
    [pscustomobject]@{ Metric = "Report texture array eligibility"; Value = if ($materialBatchBuildReportTextureArrayEligibilityDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON texture array eligibility values"; Value = if ($materialBatchBuildManifestTextureArrayEligibilityValuesDetected) { "present" } else { "not found in current sample" } },
    [pscustomobject]@{ Metric = "Report texture array slices"; Value = if ($materialBatchBuildReportTextureArraySlicesDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON texture arrays"; Value = if ($materialBatchBuildManifestTextureArraysDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Report property texture rows"; Value = if ($materialBatchBuildReportPropertyRowsDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON property rows"; Value = if ($materialBatchBuildManifestPropertyRowsDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Report property texture layout"; Value = if ($materialBatchBuildReportPropertyTextureLayoutDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON property texture layout"; Value = if ($materialBatchBuildManifestPropertyTextureLayoutDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Report geometry merge plan"; Value = if ($materialBatchBuildReportGeometryMergePlanDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Report material slot remap"; Value = if ($materialBatchBuildReportMaterialSlotRemapDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON geometry merge plan"; Value = if ($materialBatchBuildManifestGeometryMergePlanDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "JSON material slot remap"; Value = if ($materialBatchBuildManifestMaterialSlotRemapDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Mapping data asset type"; Value = if ($materialBatchMappingDataAssetTypeDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "ApplyMappingOnly commandlet path"; Value = if ($materialBatchBuildApplyMappingOnlyDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Generated mapping asset"; Value = if ($materialBatchGeneratedMappingAssetDetected) { $materialBatchGeneratedMappingAssetPath } else { "" } },
    [pscustomobject]@{ Metric = "Report mapping asset saved"; Value = if ($materialBatchBuildReportMappingAssetSavedDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "ApplyTextureArraysOnly commandlet path"; Value = if ($materialBatchBuildApplyTextureArraysOnlyDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Generated Texture2DArray asset count"; Value = if ($materialBatchGeneratedTextureArraysDetected) { $materialBatchGeneratedTextureArraysCount } else { "" } },
    [pscustomobject]@{ Metric = "Report Texture2DArray assets saved"; Value = if ($materialBatchBuildReportTextureArraysSavedDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "ApplyPropertyTextureOnly commandlet path"; Value = if ($materialBatchBuildApplyPropertyTextureOnlyDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Generated property texture"; Value = if ($materialBatchGeneratedPropertyTextureDetected) { $materialBatchGeneratedPropertyTexturePath } else { "" } },
    [pscustomobject]@{ Metric = "Report property texture saved"; Value = if ($materialBatchBuildReportPropertyTextureSavedDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "ApplyProxyMeshOnly commandlet path"; Value = if ($materialBatchBuildApplyProxyMeshOnlyDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Generated proxy mesh"; Value = if ($materialBatchGeneratedProxyMeshDetected) { $materialBatchGeneratedProxyMeshPath } else { "" } },
    [pscustomobject]@{ Metric = "Report proxy mesh saved"; Value = if ($materialBatchBuildReportProxyMeshSavedDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "ApplyBatchMaterialOnly commandlet path"; Value = if ($materialBatchBuildApplyBatchMaterialOnlyDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Generated batch material instance"; Value = if ($materialBatchGeneratedBatchMaterialDetected) { $materialBatchGeneratedBatchMaterialPath } else { "" } },
    [pscustomobject]@{ Metric = "Report batch material saved"; Value = if ($materialBatchBuildReportBatchMaterialSavedDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Report batch material bindings"; Value = if ($materialBatchBuildReportBatchMaterialBoundDetected) { "present" } else { "" } }
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.Value) }

$materialBatchBuildReportTable = @("| Metric | Value |", "| --- | --- |")
if ($materialBatchBuildReportRows.Count -gt 0) {
    foreach ($row in $materialBatchBuildReportRows) {
        $materialBatchBuildReportTable += "| $($row.Metric) | $($row.Value) |"
    }
}
else {
    $materialBatchBuildReportTable += "| Report summary | (not available) |"
}

$materialBatchMaterialAuditRows = @(
    [pscustomobject]@{ Metric = "Material"; Value = Get-MarkdownBulletValue -Text $materialBatchMaterialAuditReportText -Label "Material" },
    [pscustomobject]@{ Metric = "Loaded"; Value = Get-MarkdownBulletValue -Text $materialBatchMaterialAuditReportText -Label "Loaded" },
    [pscustomobject]@{ Metric = "Class"; Value = Get-MarkdownBulletValue -Text $materialBatchMaterialAuditReportText -Label "Class" },
    [pscustomobject]@{ Metric = "BlendMode"; Value = Get-MarkdownBulletValue -Text $materialBatchMaterialAuditReportText -Label "BlendMode" },
    [pscustomobject]@{ Metric = "ShadingModel"; Value = Get-MarkdownBulletValue -Text $materialBatchMaterialAuditReportText -Label "ShadingModel" },
    [pscustomobject]@{ Metric = "Masked"; Value = Get-MarkdownBulletValue -Text $materialBatchMaterialAuditReportText -Label "Masked" },
    [pscustomobject]@{ Metric = "Texture parameters"; Value = if ($materialBatchMaterialAuditTextureParametersDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Scalar parameters"; Value = if ($materialBatchMaterialAuditScalarParametersDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Vector parameters"; Value = if ($materialBatchMaterialAuditVectorParametersDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Static switches"; Value = if ($materialBatchMaterialAuditStaticSwitchDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Texture2DArray parameters"; Value = if ($materialBatchMaterialAuditTextureArrayDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Material light info"; Value = if ($materialBatchMaterialAuditLightInfoDetected) { "present" } else { "" } }
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.Value) }

$materialBatchMaterialAuditTable = @("| Metric | Value |", "| --- | --- |")
if ($materialBatchMaterialAuditRows.Count -gt 0) {
    foreach ($row in $materialBatchMaterialAuditRows) {
        $materialBatchMaterialAuditTable += "| $($row.Metric) | $($row.Value) |"
    }
}
else {
    $materialBatchMaterialAuditTable += "| Report summary | (not available) |"
}

$ue58ScenePerformanceAuditRows = @(
    [pscustomobject]@{ Metric = "Map"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Map" },
    [pscustomobject]@{ Metric = "Loaded"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Loaded" },
    [pscustomobject]@{ Metric = "Levels"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Levels" },
    [pscustomobject]@{ Metric = "Actors"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Actors" },
    [pscustomobject]@{ Metric = "StaticMesh components"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "StaticMesh components" },
    [pscustomobject]@{ Metric = "StaticMesh components with mesh"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "StaticMesh components with mesh" },
    [pscustomobject]@{ Metric = "StaticMesh material slot upper-bound"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "StaticMesh material slot upper-bound" },
    [pscustomobject]@{ Metric = "Movable StaticMesh components"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Movable StaticMesh components" },
    [pscustomobject]@{ Metric = "Light components"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Light components" },
    [pscustomobject]@{ Metric = "Movable lights"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Movable lights" },
    [pscustomobject]@{ Metric = "Shadow-casting lights"; Value = Get-MarkdownBulletValue -Text $ue58ScenePerformanceAuditReportText -Label "Shadow-casting lights" }
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.Value) }

$ue58ScenePerformanceAuditTable = @("| Metric | Value |", "| --- | --- |")
if ($ue58ScenePerformanceAuditRows.Count -gt 0) {
    foreach ($row in $ue58ScenePerformanceAuditRows) {
        $ue58ScenePerformanceAuditTable += "| $($row.Metric) | $($row.Value) |"
    }
}
else {
    $ue58ScenePerformanceAuditTable += "| Report summary | (not available) |"
}

$ue58RuntimeProfilingPlanRows = @(
    [pscustomobject]@{ Metric = "Map"; Value = Get-MarkdownBulletValue -Text $ue58RuntimeProfilingPlanReportText -Label "Map" },
    [pscustomobject]@{ Metric = "Cluster"; Value = Get-MarkdownBulletValue -Text $ue58RuntimeProfilingPlanReportText -Label "Cluster" },
    [pscustomobject]@{ Metric = "Camera label"; Value = Get-MarkdownBulletValue -Text $ue58RuntimeProfilingPlanReportText -Label "Camera label" },
    [pscustomobject]@{ Metric = "Evidence status"; Value = Get-MarkdownBulletValue -Text $ue58RuntimeProfilingPlanReportText -Label "Evidence status" },
    [pscustomobject]@{ Metric = "Scenario matrix"; Value = if ($ue58RuntimeProfilingPlanMatrixDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "Capture commands"; Value = if ($ue58RuntimeProfilingPlanCaptureDetected) { "present" } else { "" } },
    [pscustomobject]@{ Metric = "NotMeasured guard"; Value = if ($ue58RuntimeProfilingPlanNotMeasuredDetected) { "present" } else { "" } }
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.Value) }

$ue58RuntimeProfilingPlanTable = @("| Metric | Value |", "| --- | --- |")
if ($ue58RuntimeProfilingPlanRows.Count -gt 0) {
    foreach ($row in $ue58RuntimeProfilingPlanRows) {
        $ue58RuntimeProfilingPlanTable += "| $($row.Metric) | $($row.Value) |"
    }
}
else {
    $ue58RuntimeProfilingPlanTable += "| Report summary | (not available) |"
}

$lines = @(
    "# UE5.8 Performance and Art Production Static Audit",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Tier plan: $tierPlanPath",
    "- Comprehensive plan: $comprehensivePlanPath",
    "- Target sample material exists: $(Test-Path -LiteralPath $targetMaterialPath)",
    "- MCP project settings detected: $mcpConfigured",
    "",
    "## Renderer Defaults",
    ""
) + $rendererTable + @(
    "",
    "## Source-Controlled Tier Files",
    "",
    "| File | Exists |",
    "| --- | --- |",
    "| Config/DefaultScalability.ini | $(Test-Path -LiteralPath $defaultScalabilityPath) |",
    "| Config/DefaultDeviceProfiles.ini | $(Test-Path -LiteralPath $defaultDeviceProfilesPath) |",
    "| UE58 tier plan | $tierPlanDetected |",
    "| UE58 comprehensive art/performance plan | $comprehensivePlanDetected |",
    "| UYogSettingsSave graphics fields | $graphicsSaveDetected |",
    "| UYogPerformanceSettingsLibrary | $performanceLibraryDetected |",
    "| Custom graphics settings builder | $customGraphicsSettingsDetected |",
    "| Frontend Options performance UI | $frontendOptionsDetected |",
    "| Frontend Options detailed custom UI | $detailedFrontendOptionsDetected |",
    "| Frontend preset state sync | $frontendPresetStateSyncDetected |",
    "| Performance settings automation test | $performanceSettingsTestDetected |",
    "| Graphics settings UMG base | $graphicsSettingsWidgetBaseDetected |",
    "| Graphics settings frontend entry | $graphicsSettingsFrontendEntryDetected |",
    "| Graphics settings setup commandlet | $graphicsSettingsWidgetSetupCommandletDetected |",
    "| Graphics settings widget automation contracts | $graphicsSettingsWidgetSetupTestDetected |",
    "| Graphics settings focus contract | $graphicsSettingsFocusContractDetected |",
    "| Generated graphics settings widget asset | $graphicsSettingsWidgetAssetDetected |",
    "| Graphics settings setup report | $graphicsSettingsWidgetSetupReportDetected |",
    "| Graphics settings config/cook path | $graphicsSettingsConfigDetected |",
    "| UE58 Material MCP audit report | $ue58MaterialMcpAuditReportDetected |",
    "| UE58 Material MCP required batch params | $ue58MaterialMcpAuditRequiredParamsDetected |",
    "| UE58 Material MCP graph evidence | $ue58MaterialMcpAuditGraphDetected |",
    "| UE58 Batch Visual MCP audit report | $ue58BatchVisualMcpAuditReportDetected |",
    "| UE58 Batch Visual MCP captures | $ue58BatchVisualMcpAuditCapturedDetected |",
    "| UE58 Batch Visual MCP PNG files | $ue58BatchVisualMcpAuditPngDetected |",
    "| UE58 Scene Parity MCP audit report | $ue58SceneParityMcpAuditReportDetected |",
    "| UE58 Scene Parity MCP PNG file | $ue58SceneParityMcpAuditPngDetected |",
    "| UE58 Scene Parity MCP ready | $ue58SceneParityMcpAuditReadyDetected |",
    "| MaterialBatchAudit commandlet | $materialBatchAuditDetected |",
    "| MaterialBatchBuild dry-run commandlet | $materialBatchBuildDetected |",
    "| MaterialBatchMaterialAudit commandlet | $materialBatchMaterialAuditDetected |",
    "| Material batch candidate test | $materialBatchAuditTestDetected |",
    "| Material batch build plan test | $materialBatchBuildTestDetected |",
    "| Material batch mapping data asset test | $materialBatchMappingDataAssetTestDetected |",
    "| Material batch mapping data asset type | $materialBatchMappingDataAssetTypeDetected |",
    "| MaterialBatchBuild ApplyMappingOnly | $materialBatchBuildApplyMappingOnlyDetected |",
    "| Generated mapping data asset | $materialBatchGeneratedMappingAssetDetected |",
    "| MaterialBatchBuild ApplyTextureArraysOnly | $materialBatchBuildApplyTextureArraysOnlyDetected |",
    "| Generated Texture2DArray assets | $materialBatchGeneratedTextureArraysDetected |",
    "| MaterialBatchBuild ApplyPropertyTextureOnly | $materialBatchBuildApplyPropertyTextureOnlyDetected |",
    "| Generated property texture asset | $materialBatchGeneratedPropertyTextureDetected |",
    "| MaterialBatchBuild ApplyProxyMeshOnly | $materialBatchBuildApplyProxyMeshOnlyDetected |",
    "| Generated proxy mesh asset | $materialBatchGeneratedProxyMeshDetected |",
    "| MaterialBatchBuild ApplyBatchMaterialOnly | $materialBatchBuildApplyBatchMaterialOnlyDetected |",
    "| Generated batch material instance | $materialBatchGeneratedBatchMaterialDetected |",
    "| MaterialBatchAudit report | $materialBatchAuditReportDetected |",
    "| MaterialBatchBuild report | $materialBatchBuildReportDetected |",
    "| MaterialBatchMaterialAudit report | $materialBatchMaterialAuditReportDetected |",
    "| MaterialBatchMaterialAudit target loaded | $materialBatchMaterialAuditLoadedDetected |",
    "| MaterialBatchMaterialAudit texture params | $materialBatchMaterialAuditTextureParametersDetected |",
    "| MaterialBatchMaterialAudit scalar params | $materialBatchMaterialAuditScalarParametersDetected |",
    "| MaterialBatchMaterialAudit vector params | $materialBatchMaterialAuditVectorParametersDetected |",
    "| MaterialBatchMaterialAudit static switches | $materialBatchMaterialAuditStaticSwitchDetected |",
    "| MaterialBatchMaterialAudit Texture2DArray evidence | $materialBatchMaterialAuditTextureArrayDetected |",
    "| MaterialBatchMaterialAudit light info evidence | $materialBatchMaterialAuditLightInfoDetected |",
    "| MaterialBatchBuild planned entries | $materialBatchBuildEntriesDetected |",
    "| MaterialBatchBuild JSON manifest | $materialBatchBuildManifestSchemaDetected |",
    "| MaterialBatchBuild material rows | $materialBatchBuildManifestMaterialRowsDetected |",
    "| MaterialBatchBuild material sources | $materialBatchBuildManifestMaterialSourcesDetected |",
    "| MaterialBatchBuild texture channel arrays | $materialBatchBuildManifestTextureChannelsDetected |",
    "| MaterialBatchBuild texture channel values | $materialBatchBuildManifestTextureChannelValuesDetected |",
    "| MaterialBatchBuild report texture array eligibility | $materialBatchBuildReportTextureArrayEligibilityDetected |",
    "| MaterialBatchBuild JSON texture array eligibility values | $materialBatchBuildManifestTextureArrayEligibilityValuesDetected |",
    "| MaterialBatchBuild report texture array slices | $materialBatchBuildReportTextureArraySlicesDetected |",
    "| MaterialBatchBuild JSON texture arrays | $materialBatchBuildManifestTextureArraysDetected |",
    "| MaterialBatchBuild JSON texture array slice values | $materialBatchBuildManifestTextureArrayValuesDetected |",
    "| MaterialBatchBuild report property rows | $materialBatchBuildReportPropertyRowsDetected |",
    "| MaterialBatchBuild JSON property rows | $materialBatchBuildManifestPropertyRowsDetected |",
    "| MaterialBatchBuild report property texture layout | $materialBatchBuildReportPropertyTextureLayoutDetected |",
    "| MaterialBatchBuild JSON property texture layout | $materialBatchBuildManifestPropertyTextureLayoutDetected |",
    "| MaterialBatchBuild parent material contract source | $materialBatchBuildParentContractSourceDetected |",
    "| MaterialBatchBuild report parent material contract | $materialBatchBuildReportParentContractDetected |",
    "| MaterialBatchBuild JSON parent material contract | $materialBatchBuildManifestParentContractDetected |",
    "| MaterialBatchParentMaterialSetup commandlet | $materialBatchParentMaterialSetupCommandletDetected |",
    "| M_Env_Building_Batch asset | $materialBatchParentMaterialAssetDetected |",
    "| MaterialBatchParentMaterialSetup report evidence | $materialBatchParentMaterialSetupReportDetected |",
    "| MaterialBatchBuild report geometry merge plan | $materialBatchBuildReportGeometryMergePlanDetected |",
    "| MaterialBatchBuild report material slot remap | $materialBatchBuildReportMaterialSlotRemapDetected |",
    "| MaterialBatchBuild JSON geometry merge plan | $materialBatchBuildManifestGeometryMergePlanDetected |",
    "| MaterialBatchBuild JSON material slot remap | $materialBatchBuildManifestMaterialSlotRemapDetected |",
    "| MaterialBatchBuild report mapping asset saved | $materialBatchBuildReportMappingAssetSavedDetected |",
    "| MaterialBatchBuild report Texture2DArray assets saved | $materialBatchBuildReportTextureArraysSavedDetected |",
    "| MaterialBatchBuild report property texture saved | $materialBatchBuildReportPropertyTextureSavedDetected |",
    "| MaterialBatchBuild report proxy mesh saved | $materialBatchBuildReportProxyMeshSavedDetected |",
    "| MaterialBatchBuild report batch material saved | $materialBatchBuildReportBatchMaterialSavedDetected |",
    "| MaterialBatchBuild report batch material bindings | $materialBatchBuildReportBatchMaterialBoundDetected |",
    "| UE58ScenePerformanceAudit report | $ue58ScenePerformanceAuditReportDetected |",
    "| UE58ScenePerformanceAudit loaded map | $ue58ScenePerformanceAuditLoadedDetected |",
    "| UE58ScenePerformanceAudit StaticMesh evidence | $ue58ScenePerformanceAuditStaticMeshDetected |",
    "| UE58ScenePerformanceAudit light evidence | $ue58ScenePerformanceAuditLightDetected |",
    "| UE58RuntimeProfilingPlan commandlet | $ue58RuntimeProfilingPlanDetected |",
    "| UE58RuntimeProfilingPlan automation test | $ue58RuntimeProfilingPlanTestDetected |",
    "| UE58RuntimeProfilingPlan report | $ue58RuntimeProfilingPlanReportDetected |",
    "| UE58RuntimeProfilingPlan scenario matrix | $ue58RuntimeProfilingPlanMatrixDetected |",
    "| UE58RuntimeProfilingPlan capture commands | $ue58RuntimeProfilingPlanCaptureDetected |",
    "| UE58RuntimeProfiling MCP smoke report | $ue58RuntimeProfilingMcpSmokeReportDetected |",
    "| UE58RuntimeProfiling MCP smoke ready | $ue58RuntimeProfilingMcpSmokeReadyDetected |",
    "| UE58RuntimeProfiling capture script | $ue58RuntimeProfilingCaptureScriptDetected |",
    "| UE58RuntimeProfiling capture report | $ue58RuntimeProfilingCaptureReportDetected |",
    "| UE58RuntimeProfiling capture ready | $ue58RuntimeProfilingCaptureReadyDetected |",
    "",
    "## DevKit Device Profiles",
    ""
)

if ($devKitProfiles.Count -gt 0) {
    $lines += ($devKitProfiles | Sort-Object | ForEach-Object { "- $_" })
}
else {
    $lines += "- (none detected)"
}

$requiredNextEvidence = @()
if (-not ($ue58MaterialMcpAuditRequiredParamsDetected -and $ue58MaterialMcpAuditGraphDetected)) {
    $requiredNextEvidence += "- Use UE MCP MaterialTools when the local MCP server is reachable to cross-check `/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building` expression graph connections; the local `MaterialBatchMaterialAudit` commandlet already captures parameter, texture, scalar, vector, and static-switch evidence."
}
if (-not $ue58RuntimeProfilingCaptureReadyDetected) {
    if ($ue58RuntimeProfilingCaptureReportText -match "(?m)^- Status: LogCaptured\s*$") {
        $requiredNextEvidence += "- Runtime profiling automation now launches baseline and Lumen Lite scenarios and invokes the stat/profilegpu commands, but UE did not emit a ProfileGPU artifact. Add a follow-up capture mode that writes a GPU profile artifact or parseable frame metrics before using the data for handheld decisions."
    }
    else {
        $requiredNextEvidence += "- Use `Invoke-UE58RuntimeProfilingCapture.ps1 -Run` to launch UE5.8 runtime scenarios and collect `stat rhi`, `stat scenerendering`, `stat gpu`, and `profilegpu` logs/artifacts. The profiling plan and MCP smoke prove readiness only, not measured draw calls or GPU pass costs."
    }
}
if ($materialBatchBuildParentContractSourceDetected) {
    if (-not ($materialBatchParentMaterialAssetDetected -and $materialBatchParentMaterialSetupReportDetected)) {
        $requiredNextEvidence += "- Create or validate the actual `M_Env_Building_Batch` material asset against the source-controlled parent contract: read `TexCoord7.x`, sample `_PropTexture`, and use the row data as Texture2DArray slice indices."
    }
    elseif (-not ($ue58BatchVisualMcpAuditCapturedDetected -and $ue58BatchVisualMcpAuditPngDetected)) {
        $requiredNextEvidence += "- Run `Invoke-UE58BatchVisualMcpAudit.ps1` with the UE5.8 MCP server available to capture source and generated batch asset thumbnails before production replacement."
    }
    elseif (-not $ue58SceneParityMcpAuditReadyDetected) {
        $requiredNextEvidence += "- Run `Invoke-UE58SceneParityMcpAudit.ps1` with the UE5.8 MCP server available to capture source/proxy meshes side by side in a target level lighting context."
    }
    else {
        $requiredNextEvidence += "- Scene parity evidence is available; review the source/proxy side-by-side PNG before enabling production map replacement."
    }
}
else {
    $requiredNextEvidence += "- Create or validate the final `M_Env_Building_Batch` parent material contract before production use; the current generated MI binds Texture2DArray parameters but does not yet read `TexCoord7.x` plus the property texture row data."
}
if (-not $graphicsSettingsFocusContractDetected) {
    $requiredNextEvidence += "- Add and test a stable controller focus target for `WBP_GraphicsSettingsWidget`; keep all profile/cvar logic routed through UYogPerformanceSettingsLibrary."
}
$requiredNextEvidence += "- Compile before upload or push."

$lines += @(
    "",
    "## Scalability Section Count",
    "",
    "- Detected quality sections: $($scalabilitySections.Count)",
    "",
    "## Content Counts",
    ""
) + $countTable + @(
    "",
    "## Material Batch Audit Latest Report",
    ""
) + $materialBatchReportTable + @(
    "",
    "## Material Batch Build Latest Report",
    ""
) + $materialBatchBuildReportTable + @(
    "",
    "## Target Material Audit Latest Report",
    ""
) + $materialBatchMaterialAuditTable + @(
    "",
    "## UE5.8 Scene Performance Audit Latest Report",
    ""
) + $ue58ScenePerformanceAuditTable + @(
    "",
    "## UE5.8 Runtime Profiling Plan Latest Report",
    ""
) + $ue58RuntimeProfilingPlanTable + @(
    "",
    "## Static Risks and Gaps",
    ""
) + $riskList + @(
    "",
    "## Required Next Evidence",
    ""
) + $requiredNextEvidence

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote audit report: $reportPath"
Write-Output "Updated latest audit: $latestPath"
