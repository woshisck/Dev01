#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetGraphSchema_GameplayAbilityComboGraph.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "GenericGraphAssetEditor/EdGraph_GenericGraph.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComboGraphEditorSchemaAddsDodgeRootPresetTest,
	"DevKit.ComboGraphEditor.SchemaAddsDodgeRootPreset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComboGraphEditorSchemaAddsDodgeRootPresetTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* ComboGraph = NewObject<UGameplayAbilityComboGraph>();
	UEdGraph_GenericGraph* EditorGraph = NewObject<UEdGraph_GenericGraph>(ComboGraph);
	EditorGraph->Schema = UAssetGraphSchema_GameplayAbilityComboGraph::StaticClass();
	ComboGraph->EdGraph = EditorGraph;

	FGraphContextMenuBuilder ContextMenuBuilder(EditorGraph);
	GetDefault<UAssetGraphSchema_GameplayAbilityComboGraph>()->GetGraphContextActions(ContextMenuBuilder);

	TSharedPtr<FEdGraphSchemaAction> DodgeRootAction;
	for (int32 ActionIndex = 0; ActionIndex < ContextMenuBuilder.GetNumActions(); ++ActionIndex)
	{
		FGraphActionListBuilderBase::ActionGroup& ActionGroup = ContextMenuBuilder.GetAction(ActionIndex);
		for (const TSharedPtr<FEdGraphSchemaAction>& Action : ActionGroup.Actions)
		{
			if (Action.IsValid() && Action->GetMenuDescription().ToString() == TEXT("Dodge Action Root Node"))
			{
				DodgeRootAction = Action;
				break;
			}
		}
	}

	TestTrue(TEXT("Context menu exposes a Dodge Action Root Node preset"), DodgeRootAction.IsValid());
	if (!DodgeRootAction.IsValid())
	{
		return false;
	}

	UEdGraphNode* CreatedEditorNode = DodgeRootAction->PerformAction(EditorGraph, nullptr, FVector2D::ZeroVector, false);
	UEdNode_GenericGraphNode* CreatedGenericNode = Cast<UEdNode_GenericGraphNode>(CreatedEditorNode);
	UGameplayAbilityComboGraphNode* CreatedComboNode = CreatedGenericNode ? Cast<UGameplayAbilityComboGraphNode>(CreatedGenericNode->GenericGraphNode) : nullptr;

	TestNotNull(TEXT("Preset creates a combo graph editor node"), CreatedGenericNode);
	TestNotNull(TEXT("Preset creates a gameplay ability combo node"), CreatedComboNode);
	if (!CreatedComboNode)
	{
		return false;
	}

	TestEqual(TEXT("Preset node id"), CreatedComboNode->NodeId, FName(TEXT("DodgeActionRoot")));
	TestEqual(TEXT("Preset root input"), CreatedComboNode->RootInputAction, EYogComboGraphInputAction::Dash);
	TestNull(TEXT("Dodge preset does not require a montage"), CreatedComboNode->Montage.Get());
	TestFalse(TEXT("Dodge preset does not use attack combo windows"), CreatedComboNode->bUseNodeComboWindow);
	TestEqual(TEXT("Dodge preset total frames"), CreatedComboNode->TotalFrames, 1);
	TestEqual(TEXT("Dodge preset dash save mode"), CreatedComboNode->DashSaveMode, EYogComboGraphDashSaveMode::PreserveIfSourceAllows);
	TestTrue(TEXT("Dodge preset keeps pending link context"), CreatedComboNode->bSavePendingLinkContext);
	TestTrue(TEXT("Dodge preset clears combat tags on dash end"), CreatedComboNode->bClearCombatTagsOnDashEnd);
	TestTrue(TEXT("Dodge preset breaks combo on dash cancel"), CreatedComboNode->bBreakComboOnDashCancel);

	EditorGraph->RebuildGenericGraph();
	TestNotNull(TEXT("Rebuilt graph finds Dodge root through Dash input"), ComboGraph->FindRootComboNode(EYogComboGraphInputAction::Dash));

	return true;
}

#endif
