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
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\EnvBatchTagTools"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$nativeHeaderPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SEnvBatchTaggerWidget.h"
$nativeCppPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SEnvBatchTaggerWidget.cpp"
$modelComplianceHeaderPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SModelAssetComplianceWidget.h"
$modelComplianceCppPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SModelAssetComplianceWidget.cpp"
$textureHeaderPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SMaterialTextureRulesWidget.h"
$textureCppPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SMaterialTextureRulesWidget.cpp"
$batchHeaderPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SMaterialBatchToolsWidget.h"
$batchCppPath = Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SMaterialBatchToolsWidget.cpp"
$devKitEditorCppPath = Join-Path $RepoRoot "Source\DevKitEditor\DevKitEditor.cpp"
$devKitEditorBuildCsPath = Join-Path $RepoRoot "Source\DevKitEditor\DevKitEditor.Build.cs"
$pythonToolPath = Join-Path $RepoRoot "Source\DevKitEditor\MaterialBatch\EnvBatchTagTool.py"
$authoringDataPath = Join-Path $RepoRoot "Source\DevKit\Public\System\YogPerformanceAuthoringData.h"

function Read-TextOrEmpty {
    param([string]$Path)
    if (Test-Path -LiteralPath $Path) {
        return Get-Content -LiteralPath $Path -Raw -Encoding UTF8
    }
    return ""
}

$nativeHeaderText = Read-TextOrEmpty $nativeHeaderPath
$nativeCppText = Read-TextOrEmpty $nativeCppPath
$modelComplianceHeaderText = Read-TextOrEmpty $modelComplianceHeaderPath
$modelComplianceCppText = Read-TextOrEmpty $modelComplianceCppPath
$textureHeaderText = Read-TextOrEmpty $textureHeaderPath
$textureCppText = Read-TextOrEmpty $textureCppPath
$batchHeaderText = Read-TextOrEmpty $batchHeaderPath
$batchCppText = Read-TextOrEmpty $batchCppPath
$devKitEditorText = Read-TextOrEmpty $devKitEditorCppPath
$devKitEditorBuildCsText = Read-TextOrEmpty $devKitEditorBuildCsPath
$pythonToolText = Read-TextOrEmpty $pythonToolPath
$authoringDataText = Read-TextOrEmpty $authoringDataPath

$requiredTags = @(
    "EnvBatch.Source.",
    "EnvBatch.Proxy.",
    "EnvBatch.Baked.Ground.Mid",
    "EnvBatch.Baked.Ground.Low",
    "EnvBatch.Baked.Wall.Mid",
    "EnvBatch.Baked.Wall.Low",
    "EnvBatch.Exclude"
)

$checks = @()
function Add-Check {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Evidence
    )

    $script:checks += [pscustomobject]@{
        Name = $Name
        Passed = $Passed
        Evidence = $Evidence
    }
}

Add-Check "Native tagger source exists" (Test-Path -LiteralPath $nativeCppPath) $nativeCppPath
Add-Check "Native tagger header exists" (Test-Path -LiteralPath $nativeHeaderPath) $nativeHeaderPath
Add-Check "Model asset compliance widget exists" ((Test-Path -LiteralPath $modelComplianceHeaderPath) -and (Test-Path -LiteralPath $modelComplianceCppPath)) "$modelComplianceHeaderPath ; $modelComplianceCppPath"
Add-Check "Texture rules widget exists" ((Test-Path -LiteralPath $textureHeaderPath) -and (Test-Path -LiteralPath $textureCppPath)) "$textureHeaderPath ; $textureCppPath"
Add-Check "Material batch tools widget exists" ((Test-Path -LiteralPath $batchHeaderPath) -and (Test-Path -LiteralPath $batchCppPath)) "$batchHeaderPath ; $batchCppPath"
Add-Check "DevKit editor menu source exists" (Test-Path -LiteralPath $devKitEditorCppPath) $devKitEditorCppPath
Add-Check "DevKit editor Build.cs source exists" (Test-Path -LiteralPath $devKitEditorBuildCsPath) $devKitEditorBuildCsPath
Add-Check "Python fallback source exists" (Test-Path -LiteralPath $pythonToolPath) $pythonToolPath
Add-Check "Performance authoring data types exist" (Test-Path -LiteralPath $authoringDataPath) $authoringDataPath

foreach ($tag in $requiredTags) {
    $pattern = [regex]::Escape($tag)
    Add-Check "Native tagger exposes $tag" ($nativeCppText -match $pattern) "SEnvBatchTaggerWidget.cpp contains $tag."
    Add-Check "Python fallback exposes $tag" ($pythonToolText -match $pattern) "EnvBatchTagTool.py contains $tag."
}

Add-Check "Native tagger stays tag-only" `
    (($nativeHeaderText -notmatch "WriteModelPerformanceConfigAsset") -and
        ($nativeCppText -notmatch "WriteModelPerformanceConfigAsset") -and
        ($nativeHeaderText -notmatch "WriteDefaultTextureNamingConventionAsset") -and
        ($nativeCppText -notmatch "WriteDefaultTextureNamingConventionAsset")) `
    "SEnvBatchTaggerWidget no longer owns model config or texture naming authoring."

Add-Check "Editor exposes expanded Chinese performance launcher" `
    (($devKitEditorText -match "DevKitPerformanceToolsLauncher") -and
        ($devKitEditorText -match "DevKitModelAssetCompliance") -and
        ($devKitEditorText -match "DevKitMaterialTextureRules")) `
    "DevKitEditor.cpp registers the launcher plus model asset compliance and texture rules tabs."

Add-Check "Material batch command window is not exposed to art shortcut/menu" `
    (($devKitEditorText -notmatch "LauncherOpenMaterialBatchTools") -and
        ($devKitEditorText -notmatch "OpenMaterialBatchToolsLabel") -and
        ($devKitEditorText -match "DevKitMaterialBatchTools")) `
    "MaterialBatchTools tab remains registered for formal packaging/TA paths, but the art-facing launcher and Tools menu no longer expose its command buttons."

Add-Check "Editor exposes restored white quick-run arrow" `
    (($devKitEditorText -match "DevKitPlayFromMainMenu") -and
        ($devKitEditorText -match "Icons\.Play") -and
        ($devKitEditorText -match "ResolveEntryMenuMapPackagePath") -and
        ($devKitEditorText -match "RequestPlaySession")) `
    "DevKitEditor.cpp registers the quick-run toolbar entry and starts PIE from the entry menu map without GlobalMapOverride."

Add-Check "Editor exposes art asset tools outside the performance launcher" `
    (($devKitEditorText -match "DevKitArtAssetTools") -and
        ($devKitEditorText -match "OpenDevKitModelAssetCompliance") -and
        ($devKitEditorText -match "OpenDevKitMaterialTextureRules") -and
        ($devKitEditorText -match "OpenModelAssetCompliance") -and
        ($devKitEditorText -match "OpenMaterialTextureRules") -and
        ($devKitEditorText -notmatch "DevKitModelPerformanceConfig") -and
        ($devKitEditorText -notmatch "OpenModelPerformanceConfig") -and
        ($devKitEditorText -notmatch "SModelPerformanceConfigWidget") -and
        ($devKitEditorText -notmatch "LauncherOpenModelAssetCompliance") -and
        ($devKitEditorText -notmatch "LauncherOpenModelPerformanceConfig") -and
        ($devKitEditorText -notmatch "LauncherOpenTextureRules")) `
    "DevKitEditor.cpp exposes model compliance and texture rules from the editor toolbar / DevKit art asset section; the deleted model config window is not exposed anywhere."

Add-Check "Native tagger explains tag workflow" `
    (($nativeCppText -match "Actor Tag") -and
        ($nativeCppText -match "EnvBatch Actor Tag") -and
        ($nativeCppText -match "StaticMeshComponent") -and
        ($nativeCppText -match "Level Instance")) `
    "SEnvBatchTaggerWidget describes selected Actor, Blueprint, and Level Instance tag usage."

Add-Check "Standalone model config widget was removed" `
    ((-not (Test-Path -LiteralPath (Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SModelPerformanceConfigWidget.h"))) -and
        (-not (Test-Path -LiteralPath (Join-Path $RepoRoot "Source\DevKitEditor\Private\Tools\SModelPerformanceConfigWidget.cpp"))) -and
        ($devKitEditorText -notmatch "SModelPerformanceConfigWidget") -and
        ($devKitEditorText -notmatch "DevKitModelPerformanceConfig") -and
        ($modelComplianceCppText -notmatch "DevKitModelPerformanceConfig")) `
    "The separate model tier config window is removed; model settings are opened through the StaticMesh asset editor from model compliance."

Add-Check "Model asset compliance widget scans art StaticMeshes" `
    (($modelComplianceHeaderText -match "SModelAssetComplianceWidget") -and
        ($modelComplianceCppText -match "/Game/Art") -and
        ($modelComplianceCppText -match "UStaticMesh") -and
        ($modelComplianceCppText -match "FAssetThumbnail") -and
        ($modelComplianceCppText -match "SModelAssetComplianceViewport") -and
        ($modelComplianceCppText -match "FAdvancedPreviewScene")) `
    "SModelAssetComplianceWidget scans /Game/Art StaticMesh assets, shows thumbnails, and previews the selected mesh."

Add-Check "Model asset compliance directory preview uses expandable folder tree" `
    (($modelComplianceHeaderText -match "FModelAssetComplianceTreeNode") -and
        ($modelComplianceHeaderText -match "STreeView") -and
        ($modelComplianceCppText -match "RebuildTreeNodes") -and
        ($modelComplianceCppText -match "GenerateTreeRow") -and
        ($modelComplianceCppText -match "GetTreeChildren") -and
        ($modelComplianceCppText -match "MakeRelativeFolderPath") -and
        ($modelComplianceCppText -match "SetItemExpansion") -and
        ($modelComplianceCppText -notmatch "SListView<FComplianceItemPtr>") -and
        ($modelComplianceCppText -notmatch "GenerateAssetRow")) `
    "The model directory preview is grouped by /Game/Art folders in an expandable STreeView instead of a flat asset list."

Add-Check "Model asset compliance refreshes status while preserving tree expansion and selected position" `
    (($modelComplianceHeaderText -match "CaptureExpandedFolderPaths") -and
        ($modelComplianceHeaderText -match "RestoreExpandedFolderPaths") -and
        ($modelComplianceHeaderText -match "BuildAssetTreeContextMenu") -and
        ($modelComplianceHeaderText -match "OpenSelectedModelSettings") -and
        ($modelComplianceCppText -match "CaptureExpandedFolderPaths") -and
        ($modelComplianceCppText -match "RestoreExpandedFolderPaths") -and
        ($modelComplianceCppText -match "void SModelAssetComplianceWidget::OnStatusSettingChanged[\s\S]*RebuildFilteredItems") -and
        ($modelComplianceCppText -match "void SModelAssetComplianceWidget::OnAutoStatusChanged[\s\S]*RebuildFilteredItems") -and
        ($modelComplianceCppText -match "RequestScrollIntoView") -and
        ($modelComplianceCppText -notmatch "RefreshCurrentTreeRows") -and
        ($modelComplianceCppText -match "BuildAssetTreeContextMenu") -and
        ($modelComplianceCppText -match "ExecuteOpenSelectedAsset") -and
        ($modelComplianceCppText -match "ExecuteShowSelectedAssetInContentBrowser") -and
        ($modelComplianceCppText -match "OpenSelectedModelSettings") -and
        ($modelComplianceCppText -match "OpenSelectedModelSettings[\s\S]*ExecuteOpenSelectedAsset") -and
        ($modelComplianceCppText -match "SyncBrowserToObjects") -and
        ($modelComplianceCppText -match "OpenEditorForAsset") -and
        ($modelComplianceCppText -notmatch "DevKitModelPerformanceConfig")) `
    "Changing the selected model status rebuilds the left tree so status text updates, then restores expanded folders and scrolls the current model back into view; selected models can be opened or located from the compliance UI."

Add-Check "Model asset compliance preview module dependency exists" `
    (($devKitEditorBuildCsText -match "AdvancedPreviewScene") -and
        ($modelComplianceCppText -match "FAdvancedPreviewScene") -and
        ($modelComplianceCppText -match "SEditorViewport")) `
    "DevKitEditor.Build.cs includes AdvancedPreviewScene for the StaticMesh preview viewport."

Add-Check "Model asset compliance uses requested multi-select category filters" `
    (($modelComplianceHeaderText -match "EnabledDirectoryCategoryFilters") -and
        ($modelComplianceCppText -match "BuildDirectoryCategoryFilterButtons") -and
        ($modelComplianceCppText -match "BuildModelCategoryButtons") -and
        ($modelComplianceCppText -match "OnDirectoryCategoryFilterChanged") -and
        ($modelComplianceCppText -match "OnModelCategoryChanged") -and
        ($modelComplianceCppText -match "ToggleButtonCheckbox") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::Interactive") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::Character") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::Weapon") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::Prop") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::SceneProp") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::SceneBuilding") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::SceneGround") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::SceneOther") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::Decal") -and
        ($modelComplianceCppText -match "EModelAssetComplianceCategory::OtherAsset") -and
        ($modelComplianceCppText -notmatch "SComboBox") -and
        ($modelComplianceCppText -notmatch "FilterStatic") -and
        ($modelComplianceCppText -notmatch "FilterWall")) `
    "SModelAssetComplianceWidget uses the requested category buttons for directory filtering and current-model category definition; old static/wall/ground dropdown filters are removed."

Add-Check "Model asset compliance category definition drives directory filter metadata only" `
    (($modelComplianceCppText -match "EnabledDirectoryCategoryFilters\.Contains") -and
        ($modelComplianceCppText -match "OnDirectoryCategoryFilterChanged[\s\S]*RebuildFilteredItems") -and
        ($modelComplianceCppText -match "OnModelCategoryChanged[\s\S]*SelectedItem->Category = Category") -and
        ($modelComplianceCppText -match "OnModelCategoryChanged[\s\S]*RebuildFilteredItems") -and
        ($modelComplianceCppText -match "CategoryFilterOnly") -and
        ($modelComplianceCppText -notmatch "BuildCategoryFilterButtons") -and
        ($modelComplianceCppText -notmatch "EnabledCategoryFilters") -and
        ($modelComplianceCppText -notmatch "EnabledModelInfoCategories") -and
        ($modelComplianceCppText -notmatch "Status = EModelAssetComplianceStatus::Excluded")) `
    "Right-side buttons define the selected model category used by left directory filtering, without participating in blocked/excluded compliance logic."

Add-Check "Model asset compliance supports manual status switching" `
    (($modelComplianceHeaderText -match "bHasStatusOverride") -and
        ($modelComplianceHeaderText -match "StatusOverride") -and
        ($modelComplianceHeaderText -match "GetDisplayStatus") -and
        ($modelComplianceCppText -match "BuildStatusSettingButtons") -and
        ($modelComplianceCppText -match "BuildAutoStatusButton") -and
        ($modelComplianceCppText -match "OnStatusSettingChanged") -and
        ($modelComplianceCppText -match "OnAutoStatusChanged") -and
        ($modelComplianceCppText -match "GetDisplayStatus") -and
        ($modelComplianceCppText -match "StatusToString\(DisplayStatus\)") -and
        ($modelComplianceCppText -match "ManualStatusOverrideInfo") -and
        ($modelComplianceCppText -match "EModelAssetComplianceStatus::Pass") -and
        ($modelComplianceCppText -match "EModelAssetComplianceStatus::Warning") -and
        ($modelComplianceCppText -match "EModelAssetComplianceStatus::Blocked") -and
        ($modelComplianceCppText -match "EModelAssetComplianceStatus::Excluded")) `
    "Right-side status buttons support automatic status plus manual pass/warning/blocked/excluded display overrides without writing assets."

Add-Check "Model asset compliance uses rich log and rich settings text" `
    (($modelComplianceCppText -match "SRichTextBlock") -and
        ($modelComplianceCppText -match "DecoratorStyleSet") -and
        ($modelComplianceCppText -match "GetPreviewLogRichText") -and
        ($modelComplianceCppText -match "GetSelectedDetailsRichText") -and
        ($modelComplianceCppText -match "GetSelectedMessagesRichText") -and
        ($modelComplianceCppText -match "SOverlay")) `
    "The selected-asset log is inside the preview overlay, and compliance/settings text uses colored SRichTextBlock content."

Add-Check "Model asset compliance enforces current tier mapping" `
    (($modelComplianceCppText -match "Epic=LOD0") -and
        ($modelComplianceCppText -match "High=LOD0") -and
        ($modelComplianceCppText -match "Mid=LOD1") -and
        ($modelComplianceCppText -match "Low=LOD1") -and
        ($modelComplianceCppText -match "LODCount < 2") -and
        ($authoringDataText -notmatch "UYogModelPerformanceConfig")) `
    "Current model tier contract is checked in model compliance as Epic/High=LOD0 and Mid/Low=LOD1; the removed model config data asset no longer owns this rule."

Add-Check "Model asset compliance warnings are art-configurable" `
    (($modelComplianceCppText -match "bIgnoreMaterialSlotWarning") -and
        ($modelComplianceCppText -match "bIgnoreCollisionWarning") -and
        ($modelComplianceCppText -match "GetStaticMaterials") -and
        ($modelComplianceCppText -match "GetBodySetup") -and
        ($modelComplianceCppText -match "GetNumTriangles") -and
        ($modelComplianceCppText -match "TriangleDisplayOnly")) `
    "Material slots and collision are warnings that can be ignored; triangle counts are displayed but not treated as warnings."

Add-Check "Model asset compliance supports all requested category definitions" `
    (($modelComplianceCppText -match "Character") -and
        ($modelComplianceCppText -match "Interactive") -and
        ($modelComplianceCppText -match "Weapon") -and
        ($modelComplianceCppText -match "Prop") -and
        ($modelComplianceCppText -match "Decal") -and
        ($modelComplianceCppText -match "OtherAsset") -and
        ($modelComplianceCppText -match "SceneProp") -and
        ($modelComplianceCppText -match "SceneBuilding") -and
        ($modelComplianceCppText -match "SceneGround") -and
        ($modelComplianceCppText -match "SceneOther") -and
        ($modelComplianceCppText -notmatch "UIOrWeapon")) `
    "The selected model can be defined as any requested category; that category is used by the left directory filter."

Add-Check "Texture rules widget writes and checks naming convention" `
    (($textureHeaderText -match "CheckSelectedTextureNaming") -and
        ($textureCppText -match "DA_MaterialTextureNamingRules_Default") -and
        ($textureCppText -match "GetSelectedAssets") -and
        ($textureCppText -match "AcceptedSuffixes") -and
        ($textureCppText -match "SRGB") -and
        ($authoringDataText -match "UYogMaterialTextureNamingConvention")) `
    "SMaterialTextureRulesWidget writes rules and checks selected Texture2D suffix/sRGB."

Add-Check "Material batch widget drives tag-based commandlet flow" `
    (($batchHeaderText -match "RunDryRunCommandlet") -and
        ($batchCppText -match "MaterialBatchBuild") -and
        ($batchCppText -match "RequireTag") -and
        ($batchCppText -match "TextureBackend=VTAtlas") -and
        ($batchCppText -match "ApplyVTAtlasOnly") -and
        ($batchCppText -match "ApplyProxyMeshOnly")) `
    "SMaterialBatchToolsWidget runs dry-run and copies reviewed partial apply commands."

Add-Check "Python fallback keeps mutual exclusion" `
    (($pythonToolText -match "MUTUALLY_EXCLUSIVE_PREFIXES") -and ($pythonToolText -match "_clear_mutually_exclusive")) `
    "EnvBatchTagTool.py removes old Source/Proxy/Baked tags before applying a new mutually-exclusive tag."

Add-Check "Python fallback uses dynamic proxy group" `
    (($pythonToolText -match "EnvBatch\.Proxy\.\{_group_name\(\)\}\.Mid") -and ($pythonToolText -match "EnvBatch\.Proxy\.\{_group_name\(\)\}\.Low")) `
    "EnvBatchTagTool.py builds Proxy tags from the current group name."

Add-Check "Python fallback exposes Source/Proxy asset readiness summary" `
    (($pythonToolText -match "_asset_readiness_summary") -and
        ($pythonToolText -match "print_asset_readiness") -and
        ($pythonToolText -match "MissingSourceAssetReference") -and
        ($pythonToolText -match "SourceLOD0") -and
        ($pythonToolText -match "ProxyLOD1")) `
    "EnvBatchTagTool.py shows and logs Source/Proxy asset readiness for selected actors."

$pythonCompilePassed = $false
$pythonCompileEvidence = "Not run"
if (Test-Path -LiteralPath $pythonToolPath) {
    $tempPyc = Join-Path ([System.IO.Path]::GetTempPath()) ("EnvBatchTagTool_{0}.pyc" -f ([guid]::NewGuid().ToString("N")))
    try {
        $compileOutput = & python -c "import py_compile, sys; py_compile.compile(sys.argv[1], cfile=sys.argv[2], doraise=True)" $pythonToolPath $tempPyc 2>&1
        $pythonCompilePassed = ($LASTEXITCODE -eq 0)
        $pythonCompileEvidence = if ($pythonCompilePassed) { "Python py_compile passed without writing __pycache__." } else { ($compileOutput -join " ; ") }
    }
    catch {
        $pythonCompilePassed = $false
        $pythonCompileEvidence = $_.Exception.Message
    }
    finally {
        if (Test-Path -LiteralPath $tempPyc) {
            Remove-Item -LiteralPath $tempPyc -Force
        }
    }
}
Add-Check "Python fallback syntax compiles" $pythonCompilePassed $pythonCompileEvidence

$failedChecks = @($checks | Where-Object { -not $_.Passed })
$status = if ($failedChecks.Count -eq 0) { "Passed" } else { "Failed" }
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58EnvBatchTagTools_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$lines = @(
    "# UE5.8 EnvBatch And Performance Tool Contract",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Checks: $($checks.Count)",
    "- Failed checks: $($failedChecks.Count)",
    "- Native tagger: $nativeCppPath",
    "- Model config widget: removed",
    "- Texture rules widget: $textureCppPath",
    "- Material batch widget: $batchCppPath",
    "- Python fallback: $pythonToolPath",
    "",
    "## Required Tags",
    ""
)

foreach ($tag in $requiredTags) {
    $lines += ("- ``{0}``" -f $tag)
}

$lines += @(
    "",
    "## Checks",
    "",
    "| Check | Passed | Evidence |",
    "| --- | --- | --- |"
)

foreach ($check in $checks) {
    $lines += "| $($check.Name) | $($check.Passed) | $($check.Evidence) |"
}

if ($failedChecks.Count -gt 0) {
    $lines += @(
        "",
        "## Failed Checks",
        ""
    )
    foreach ($check in $failedChecks) {
        $lines += "- $($check.Name)"
    }
}

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote EnvBatch/performance tool report: $reportPath"
Write-Output "Updated EnvBatch/performance tool latest: $latestPath"
Write-Output "Status: $status"

if ($status -ne "Passed") {
    exit 1
}
