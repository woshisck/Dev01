#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetGraphSchema_GameplayAbilityComboGraph.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "EdNode_ComboGraphRoot.h"
#include "GenericGraphAssetEditor/EdGraph_GenericGraph.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphEdge.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComboGraphEditorSchemaOmitsDodgeRootPresetTest,
	"DevKit.ComboGraphEditor.SchemaOmitsDodgeRootPreset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComboGraphEditorSchemaOmitsDodgeRootPresetTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* ComboGraph = NewObject<UGameplayAbilityComboGraph>();
	UEdGraph_GenericGraph* EditorGraph = NewObject<UEdGraph_GenericGraph>(ComboGraph);
	EditorGraph->Schema = UAssetGraphSchema_GameplayAbilityComboGraph::StaticClass();
	ComboGraph->EdGraph = EditorGraph;

	FGraphContextMenuBuilder ContextMenuBuilder(EditorGraph);
	GetDefault<UAssetGraphSchema_GameplayAbilityComboGraph>()->GetGraphContextActions(ContextMenuBuilder);

	bool bFoundDodgeRootAction = false;
	for (int32 ActionIndex = 0; ActionIndex < ContextMenuBuilder.GetNumActions(); ++ActionIndex)
	{
		FGraphActionListBuilderBase::ActionGroup& ActionGroup = ContextMenuBuilder.GetAction(ActionIndex);
		for (const TSharedPtr<FEdGraphSchemaAction>& Action : ActionGroup.Actions)
		{
			if (Action.IsValid() && Action->GetMenuDescription().ToString() == TEXT("Dodge Action Root Node"))
			{
				bFoundDodgeRootAction = true;
				break;
			}
		}
	}

	TestFalse(TEXT("Context menu omits Dodge/Dash root preset"), bFoundDodgeRootAction);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComboGraphEditorSchemaRootConnectionUsesVisualEdgeTest,
	"DevKit.ComboGraphEditor.RootConnectionUsesVisualEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComboGraphEditorSchemaRootConnectionUsesVisualEdgeTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* ComboGraph = NewObject<UGameplayAbilityComboGraph>();
	UEdGraph_GenericGraph* EditorGraph = NewObject<UEdGraph_GenericGraph>(ComboGraph);
	EditorGraph->Schema = UAssetGraphSchema_GameplayAbilityComboGraph::StaticClass();
	ComboGraph->EdGraph = EditorGraph;

	const UAssetGraphSchema_GameplayAbilityComboGraph* Schema = GetDefault<UAssetGraphSchema_GameplayAbilityComboGraph>();
	Schema->CreateDefaultNodesForGraph(*EditorGraph);

	UEdNode_ComboGraphRoot* RootEdNode = nullptr;
	for (UEdGraphNode* Node : EditorGraph->Nodes)
	{
		RootEdNode = Cast<UEdNode_ComboGraphRoot>(Node);
		if (RootEdNode)
		{
			break;
		}
	}
	TestNotNull(TEXT("Schema creates visual root node"), RootEdNode);
	if (!RootEdNode)
	{
		return false;
	}

	UEdNode_GenericGraphNode* ComboEdNode = NewObject<UEdNode_GenericGraphNode>(EditorGraph);
	UGameplayAbilityComboGraphNode* ComboNode = NewObject<UGameplayAbilityComboGraphNode>(ComboEdNode, ComboGraph->NodeType);
	ComboNode->Graph = ComboGraph;
	ComboNode->NodeId = TEXT("ComboRoot");
	ComboEdNode->GenericGraphNode = ComboNode;
	EditorGraph->AddNode(ComboEdNode, true, false);
	ComboEdNode->CreateNewGuid();
	ComboEdNode->PostPlacedNewNode();
	ComboEdNode->AllocateDefaultPins();
	ComboEdNode->NodePosX = 100;

	TestTrue(TEXT("Root connection succeeds"), Schema->TryCreateConnection(RootEdNode->GetOutputPin(), ComboEdNode->GetInputPin()));

	UEdNode_GenericGraphEdge* VisualEdge = nullptr;
	for (UEdGraphNode* Node : EditorGraph->Nodes)
	{
		if (UEdNode_GenericGraphEdge* EdgeNode = Cast<UEdNode_GenericGraphEdge>(Node))
		{
			VisualEdge = EdgeNode;
			break;
		}
	}
	TestNotNull(TEXT("Root connection creates a visual edge node"), VisualEdge);
	TestTrue(TEXT("Root output points at the visual edge"), RootEdNode->GetOutputPin()->LinkedTo.Num() == 1 && VisualEdge && RootEdNode->GetOutputPin()->LinkedTo[0]->GetOwningNode() == VisualEdge);
	TestTrue(TEXT("Visual edge points at combo root node"),
		VisualEdge && VisualEdge->Pins.Num() > 1 && VisualEdge->Pins[1]->LinkedTo.Num() == 1 && VisualEdge->Pins[1]->LinkedTo[0]->GetOwningNode() == ComboEdNode);
	TestFalse(TEXT("Root visual edge does not show combo input label"), VisualEdge && VisualEdge->GenericGraphEdge && VisualEdge->GenericGraphEdge->bShouldDrawTitle);
	TestFalse(TEXT("Duplicate root connection is rejected"), Schema->TryCreateConnection(RootEdNode->GetOutputPin(), ComboEdNode->GetInputPin()));
	TestFalse(TEXT("Connections into Root are rejected"), Schema->TryCreateConnection(ComboEdNode->GetOutputPin(), RootEdNode->GetInputPin()));

	EditorGraph->RebuildGenericGraph();

	TestTrue(TEXT("Combo root remains runtime root"), ComboGraph->RootNodes.Contains(ComboNode));
	TestEqual(TEXT("Combo root has no runtime parent"), ComboNode->ParentNodes.Num(), 0);
	TestEqual(TEXT("Root visual edge is not a runtime edge"), EditorGraph->EdgeMap.Num(), 0);

	return true;
}

#endif
