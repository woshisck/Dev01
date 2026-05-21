#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphEdge.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterGraphDefaultsTest,
	"DevKit.StoryEncounter.GraphDefaultsToStoryEncounterTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterGraphDefaultsTest::RunTest(const FString& Parameters)
{
	UStoryEncounterGraph* Graph = NewObject<UStoryEncounterGraph>();

	TestEqual(TEXT("Graph uses story encounter node type"),
		Graph->NodeType.Get(),
		UStoryEncounterGraphNode::StaticClass());
	TestEqual(TEXT("Graph uses story encounter edge type"),
		Graph->EdgeType.Get(),
		UStoryEncounterGraphEdge::StaticClass());
#if WITH_EDITORONLY_DATA
	TestTrue(TEXT("Story graph nodes can be renamed"), Graph->bCanRenameNode);
	TestFalse(TEXT("Story graph should not be cyclical by default"), Graph->bCanBeCyclical);
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterPointConvertsToNodeTest,
	"DevKit.StoryEncounter.PointConvertsToEncounterNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterPointConvertsToNodeTest::RunTest(const FString& Parameters)
{
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>();
	Point->EncounterId = TEXT("EM_MemoryTutorial_PreRun");
	Point->NodeId = TEXT("weapon_echo");
	Point->DisplayName = FText::FromString(TEXT("武器残影"));
	Point->Kind = EStoryEncounterNodeKind::Object;
	Point->PlayerFacingEvent = FText::FromString(TEXT("玩家看见武器残影。"));

	FStoryEncounterAction HintAction;
	HintAction.Kind = EStoryEncounterActionKind::WeakHint;
	HintAction.Title = FText::FromString(TEXT("残影"));
	HintAction.Body = FText::FromString(TEXT("靠近残影，观察它的动作。"));
	Point->Actions.Add(HintAction);

	const FStoryEncounterNode Node = Point->ToEncounterNode();

	TestEqual(TEXT("Node id is copied"), Node.NodeId, FName(TEXT("weapon_echo")));
	TestTrue(TEXT("Display name is copied"), Node.DisplayName.EqualTo(Point->DisplayName));
	TestEqual(TEXT("Kind is copied"), Node.Kind, EStoryEncounterNodeKind::Object);
	TestTrue(TEXT("Player event is copied"), Node.PlayerFacingEvent.EqualTo(Point->PlayerFacingEvent));
	TestEqual(TEXT("Actions are copied"), Node.Actions.Num(), 1);
	TestEqual(TEXT("Weak hint action survives conversion"), Node.Actions[0].Kind, EStoryEncounterActionKind::WeakHint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterGraphNodeReadsBoundPointTest,
	"DevKit.StoryEncounter.GraphNodeReadsBoundPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterGraphNodeReadsBoundPointTest::RunTest(const FString& Parameters)
{
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>();
	Point->EncounterId = TEXT("EM_MemoryTutorial_PreRun");
	Point->NodeId = TEXT("weapon_echo");
	Point->DisplayName = FText::FromString(TEXT("武器残影"));

	UStoryEncounterGraphNode* GraphNode = NewObject<UStoryEncounterGraphNode>();
	GraphNode->Point = Point;

	TestEqual(TEXT("Graph node reports point encounter id"),
		GraphNode->GetEncounterId(),
		FName(TEXT("EM_MemoryTutorial_PreRun")));
	TestEqual(TEXT("Graph node reports point node id"),
		GraphNode->GetStoryNodeId(),
		FName(TEXT("weapon_echo")));
#if WITH_EDITOR
	TestTrue(TEXT("Graph node title comes from bound point"),
		GraphNode->GetNodeTitle().EqualTo(Point->DisplayName));
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterRuntimeRejectsNullPointTest,
	"DevKit.StoryEncounter.RuntimeRejectsNullEncounterPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterRuntimeRejectsNullPointTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEncounterRuntimeSubsystem* Runtime = NewObject<UStoryEncounterRuntimeSubsystem>(GameInstance);

	TestFalse(TEXT("Null story point should not trigger"),
		Runtime->TriggerEncounterPoint(nullptr, nullptr));
	return true;
}

#endif
