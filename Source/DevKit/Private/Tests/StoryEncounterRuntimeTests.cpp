#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Story/Encounter/StoryEncounterMap.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"
#include "Story/Encounter/StoryProductionBoard.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterMapFindNodeTest,
	"DevKit.StoryEncounter.MapFindsNodeById",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterMapFindNodeTest::RunTest(const FString& Parameters)
{
	UStoryEncounterMap* Map = NewObject<UStoryEncounterMap>();

	FStoryEncounterNode Node;
	Node.NodeId = TEXT("weapon_echo");
	Node.DisplayName = FText::FromString(TEXT("武器残影"));
	Map->Nodes.Add(Node);

	const FStoryEncounterNode* Found = Map->FindNode(TEXT("weapon_echo"));
	TestNotNull(TEXT("FindNode returns configured node"), Found);
	if (Found)
	{
		TestEqual(TEXT("Found node id matches"), Found->NodeId, FName(TEXT("weapon_echo")));
	}

	TestNull(TEXT("FindNode returns null for missing node"), Map->FindNode(TEXT("missing")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryProductionBoardFindRowTest,
	"DevKit.StoryEncounter.ProductionBoardFindsRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryProductionBoardFindRowTest::RunTest(const FString& Parameters)
{
	UStoryProductionBoardDA* Board = NewObject<UStoryProductionBoardDA>();

	FStoryProductionRow Row;
	Row.RequirementId = TEXT("TUT_MEM_002");
	Row.FlowId = TEXT("MemoryTutorial");
	Row.EncounterId = TEXT("EM_MemoryTutorial_PreRun");
	Row.NodeId = TEXT("weapon_echo");
	Row.Status = EStoryProductionStatus::Designed;
	Board->Rows.Add(Row);

	const FStoryProductionRow* Found = Board->FindRow(TEXT("TUT_MEM_002"));
	TestNotNull(TEXT("FindRow returns configured row"), Found);
	if (Found)
	{
		TestEqual(TEXT("Node id matches"), Found->NodeId, FName(TEXT("weapon_echo")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterConvertsRecordProgressTest,
	"DevKit.StoryEncounter.ConvertsRecordProgressAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterConvertsRecordProgressTest::RunTest(const FString& Parameters)
{
	FStoryEncounterAction EncounterAction;
	EncounterAction.Kind = EStoryEncounterActionKind::RecordProgress;
	EncounterAction.ProgressKey = TEXT("weapon_echo_seen");
	EncounterAction.ProgressLabel = FText::FromString(TEXT("已看过武器残影"));

	FStoryAction StoryAction;
	const bool bConverted = UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(
		TEXT("EM_MemoryTutorial_PreRun"),
		EncounterAction,
		StoryAction);

	TestTrue(TEXT("Action converts"), bConverted);
	TestEqual(TEXT("Converts to SetFlag"), StoryAction.Type, EStoryActionType::SetFlag);
	TestTrue(TEXT("Generated flag is valid"), StoryAction.FlagTag.IsValid());
	TestEqual(TEXT("Generated tag name is stable"),
		UStoryEncounterRuntimeSubsystem::MakeProgressTagName(TEXT("EM_MemoryTutorial_PreRun"), TEXT("weapon_echo_seen")),
		FString(TEXT("Story.Encounter.Progress.EM_MemoryTutorial_PreRun.weapon_echo_seen")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterConvertsWeakHintTest,
	"DevKit.StoryEncounter.ConvertsWeakHintToInfoHint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterConvertsWeakHintTest::RunTest(const FString& Parameters)
{
	FStoryEncounterAction EncounterAction;
	EncounterAction.Kind = EStoryEncounterActionKind::WeakHint;
	EncounterAction.Title = FText::FromString(TEXT("移动"));
	EncounterAction.Body = FText::FromString(TEXT("穿过记忆残影。"));

	FStoryAction StoryAction;
	const bool bConverted = UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(
		TEXT("EM_MemoryTutorial_PreRun"),
		EncounterAction,
		StoryAction);

	TestTrue(TEXT("Action converts"), bConverted);
	TestEqual(TEXT("Converts to ShowInfoHint"), StoryAction.Type, EStoryActionType::ShowInfoHint);
	TestTrue(TEXT("Hint body is retained"), StoryAction.HintText.EqualTo(EncounterAction.Body));
	return true;
}

#endif
