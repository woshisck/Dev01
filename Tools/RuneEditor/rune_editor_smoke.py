from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read_text(path: str) -> str:
    target = ROOT / path
    require(target.exists(), f"Missing required file: {path}")
    return target.read_text(encoding="utf-8", errors="replace")


def require_all(text: str, markers: list[str], scope: str) -> None:
    for marker in markers:
        require(marker in text, f"{scope} missing marker: {marker}")


def require_none(text: str, markers: list[str], scope: str) -> None:
    for marker in markers:
        require(marker not in text, f"{scope} should not contain marker: {marker}")


def main() -> int:
    authoring_h = read_text("Source/DevKitEditor/Private/RuneEditor/RuneEditorAuthoring.h")
    authoring_cpp = read_text("Source/DevKitEditor/Private/RuneEditor/RuneEditorAuthoring.cpp")
    validation_h = read_text("Source/DevKitEditor/Private/RuneEditor/RuneEditorValidation.h")
    validation_cpp = read_text("Source/DevKitEditor/Private/RuneEditor/RuneEditorValidation.cpp")
    widget_h = read_text("Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h")
    widget_cpp = read_text("Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp")
    data_editor_library_cpp = read_text("Source/DevKitEditor/Private/Tools/DataEditorLibrary.cpp")
    rune_data_asset_h = read_text("Source/DevKit/Public/Data/RuneDataAsset.h")
    gameplay_tags_ini = read_text("Config/DefaultGameplayTags.ini")
    module_cpp = read_text("Source/DevKitEditor/DevKitEditor.cpp")
    build_cs = read_text("Source/DevKitEditor/DevKitEditor.Build.cs")
    flow_schema_cpp = read_text("Plugins/FlowGraph-2.1-5.4/Source/FlowEditor/Private/Graph/FlowGraphSchema.cpp")
    flow_schema_actions_cpp = read_text("Plugins/FlowGraph-2.1-5.4/Source/FlowEditor/Private/Graph/FlowGraphSchema_Actions.cpp")
    yog_flow_asset_h = read_text("Source/DevKit/Public/BuffFlow/YogRuneFlowAsset.h")
    yog_flow_asset_cpp = read_text("Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp")
    yog_nodes_h = read_text("Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h")
    yog_nodes_cpp = read_text("Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp")
    preview_html = read_text("Docs/Prototypes/RuneEditorPreview/index.html")

    require_all(
        yog_flow_asset_h + yog_flow_asset_cpp,
        [
            "UYogRuneFlowAsset",
            "Yog Rune Flow Graph",
            "AllowedNodeClasses",
            "UFlowNode_Start::StaticClass",
            "UYogFlowNode_",
        ],
        "Yog Rune Flow asset",
    )

    require_all(
        yog_nodes_h + yog_nodes_cpp,
        [
            "UYogFlowNode_SkillPass",
            "UYogFlowNode_EffectDamage",
            "UYogFlowNode_EffectHeal",
            "UYogFlowNode_EffectCost",
            "UYogFlowNode_EffectAttributeModify",
            "UYogFlowNode_TaskSearchTarget",
            "UYogFlowNode_TaskEndSkill",
            "UYogFlowNode_TaskPlayAnimation",
            "UYogFlowNode_ConditionAttributeCompare",
            "UYogFlowNode_PresentationPlayVFX",
            "UYogFlowNode_TriggerDamageDealt",
            "UYogFlowNode_TriggerDamageReceived",
            "UYogFlowNode_TriggerCritHit",
            "UYogFlowNode_TriggerKill",
            "UYogFlowNode_TriggerDash",
            "UYogFlowNode_TriggerGameplayEvent",
            "UYogFlowNode_ConditionHasTag",
            "UYogFlowNode_ConditionProbability",
            "UYogFlowNode_ConditionDoOnce",
            "UYogFlowNode_ConditionCheckDistance",
            "UYogFlowNode_EffectApplyState",
            "UYogFlowNode_EffectApplyProfile",
            "UYogFlowNode_EffectApplyInRadius",
            "UYogFlowNode_EffectAreaDamage",
            "UYogFlowNode_EffectAddTag",
            "UYogFlowNode_EffectRemoveTag",
            "UYogFlowNode_EffectGrantAbility",
            "UYogFlowNode_SpawnProjectileProfile",
            "UYogFlowNode_SpawnAreaProfile",
            "UYogFlowNode_SpawnGroundPath",
            "UYogFlowNode_SpawnRangedProjectiles",
            "UYogFlowNode_SpawnSlashWave",
            "UYogFlowNode_PresentationCueOnActor",
            "UYogFlowNode_PresentationCueAtLocation",
            "UYogFlowNode_PresentationVFXProfile",
            "UYogFlowNode_PresentationFlipbook",
            "UYogFlowNode_LifecycleDelay",
            "UYogFlowNode_LifecycleFinishBuff",
            'TEXT("技能")',
            'TEXT("效果节点|瞬时效果")',
            'TEXT("效果节点|持续效果")',
            'TEXT("任务节点")',
            'TEXT("条件节点")',
            'TEXT("表现节点")',
        ],
        "Yog Flow nodes",
    )

    require_all(
        yog_flow_asset_cpp,
        [
            "UYogFlowNode_TriggerDamageDealt::StaticClass",
            "UYogFlowNode_EffectApplyState::StaticClass",
            "UYogFlowNode_SpawnProjectileProfile::StaticClass",
            "UYogFlowNode_LifecycleFinishBuff::StaticClass",
        ],
        "Yog Rune Flow allowed nodes",
    )

    require_all(
        authoring_h + authoring_cpp,
        [
            "/Game/YogRuneEditor",
            "/Game/YogRuneEditor/Runes",
            "/Game/YogRuneEditor/Flows",
            "UYogRuneFlowAsset::StaticClass",
            "DuplicateAsset",
            "RenameAssets",
            "DeleteObjects",
            "SyncBrowserToAssets",
        ],
        "Rune authoring/resource management",
    )

    require_all(
        validation_h + validation_cpp,
        [
            "ValidateRuneGraph",
            "入口节点",
            "流程节点",
            "Yog 符文流程",
        ],
        "Generic validation",
    )
    require_none(validation_h + validation_cpp, ["No rune selected.", "Entry node", "Flow nodes"], "Generic validation")
    require_none(validation_h + validation_cpp, ["ValidateRune512", "Backpack shape", "512"], "Generic validation")

    require_all(
        widget_h + widget_cpp,
        [
            "BuildResourceManagerPanel",
            "BuildRuneResourceListPanel",
            "BuildBottomDiagnosticsPanel",
            "BuildCenterTabButton",
            "BuildValueTablePanel",
            "BuildTuningRow",
            "OnCenterTabSelected",
            "OnAddTuningRowClicked",
            "OnDeleteTuningRowClicked",
            "OnTuningSourceClicked",
            "RefreshTuningRows",
            "ECenterPanelTab",
            "ActiveCenterTab",
            "TuningRows",
            "数值表",
            "流程图",
            "具体值",
            "公式",
            "MMC",
            "上下文",
            "BuildNodeLibraryPanel",
            "BuildValidationPanel",
            "BuildRunLogPanel",
            "BuildSelectedNodePanel",
            "BuildBottomTabButton",
            "BuildNodeLibraryFilterButton",
            "BuildResourceFilterButton",
            "BuildLibraryCategoryToggle",
            "ENodeLibraryFilter",
            "EResourceFilter",
            "ActiveNodeLibraryFilter",
            "ActiveResourceFilter",
            "SWrapBox",
            "ZoomToFit(false)",
            ".Value(0.84f)",
            ".Value(0.16f)",
            "基础节点",
            "敌人",
            "关卡",
            "终结技",
            "连携卡牌",
            "OnAddNodeFromLibrary",
            "符文资源",
            "节点库",
            "校验",
            "运行日志",
            "选中节点",
            "SListView<FRuneRowPtr>",
            "SWidgetSwitcher",
            "OnRuneSelectionChanged",
            "OnCopyAssetClicked",
            "OnPasteAssetClicked",
            "OnRenameAssetClicked",
            "OnDeleteAssetClicked",
            "OnLocateAssetClicked",
            "SScrollBox",
            "SSplitter",
            "Yog 符文流程",
            "ValidateRuneGraph",
            "GetResourceFilterTag",
            "GetRuneLibraryCategoryText",
            "OnLibraryCategoryToggled",
            "Rune.Library.Base",
            "Rune.Library.Enemy",
            "Rune.Library.Level",
            "Rune.Library.Finisher",
            "Rune.Library.ComboCard",
        ],
        "Rune Editor UI",
    )
    require_none(
        widget_h + widget_cpp,
        [
            "512 MVP",
            "512 Validation",
            "512 Node Catalog",
            "Backpack Shape Compatibility",
            "GetShapeCompatibilityText",
            "RuneEditorNodeCatalog",
            "Asset Browser",
            "FAssetPickerConfig",
            "BuildResourceAssetPicker",
            "OnShouldFilterResourceAsset",
            "RefreshResourceAssetPicker",
            "Graph Diagnostics",
        ],
        "Rune Editor UI",
    )

    require_all(
        data_editor_library_cpp,
        [
            "UAssetRegistryHelpers::FindAssetNativeClass",
            "UDataAsset::StaticClass",
            "NativeClass->IsChildOf(T::StaticClass())",
        ],
        "Data asset collection",
    )

    require_all(
        rune_data_asset_h + read_text("Source/DevKit/Private/Data/RuneDataAsset.cpp"),
        [
            "ERuneTuningValueSource",
            "Literal",
            "Formula",
            "MMC",
            "Context",
            "URuneValueCalculation",
            "EvaluateRuneTuningFormula",
            "FBasicMathExpressionEvaluator",
            "MagnitudeCalculationClass",
            "ContextKey",
            "FormulaExpression",
            "UnitText",
            "Description",
            "LibraryTags",
            "GetLibraryTags",
            'Categories = "Rune.Library"',
        ],
        "Rune data asset library categories",
    )

    require_all(
        gameplay_tags_ini,
        [
            'Tag="Rune.Library.Base"',
            'Tag="Rune.Library.Enemy"',
            'Tag="Rune.Library.Level"',
            'Tag="Rune.Library.Finisher"',
            'Tag="Rune.Library.ComboCard"',
        ],
        "Gameplay tags",
    )

    require("DevKitRuneEditor" in module_cpp, "DevKitEditor module should register the Rune Editor tab")
    require("OpenRuneEditorTab" in module_cpp, "DevKitEditor module should expose the Rune Editor menu action")
    require("512 Rune Editor" not in module_cpp, "Rune Editor menu tooltip should be generic")
    require('"GraphEditor"' in build_cs, "DevKitEditor module should depend on GraphEditor for embedded graph editing")
    require('"ContentBrowser"' in build_cs, "DevKitEditor module should depend on ContentBrowser for resource management")
    require("FlowGraphEditor.IsValid()" in flow_schema_cpp, "FlowGraph schema should guard embedded editors without a FlowAssetEditor toolkit")
    require("return FlowGraphEditor.IsValid() ? FlowGraphEditor->GetNumberOfSelectedNodes() : 0" in flow_schema_cpp, "FlowGraph schema should safely report zero selected nodes for embedded editors")
    require("FlowGraphEditor->PasteNodesHere(Location)" in flow_schema_actions_cpp and "FlowGraphEditor.IsValid()" in flow_schema_actions_cpp, "FlowGraph paste action should guard missing FlowGraph editor")
    require_all(preview_html, ["Yog Rune Flow", "Resource Manager", "Graph Diagnostics"], "Preview")
    require_none(preview_html, ["512 MVP", "512 Validation", "Backpack Shape"], "Preview")

    print("Rune Editor smoke: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"Rune Editor smoke: FAIL - {exc}", file=sys.stderr)
        raise SystemExit(1)
