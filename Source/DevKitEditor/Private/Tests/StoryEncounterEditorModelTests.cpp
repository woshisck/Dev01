#include "Misc/AutomationTest.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Tools/StoryEncounter/StoryEncounterEditorModel.h"

namespace
{
bool HasMessageContaining(const TArray<FStoryEncounterWorkbenchMessage>& Messages, const FString& Needle)
{
	for (const FStoryEncounterWorkbenchMessage& Message : Messages)
	{
		if (Message.Message.Contains(Needle))
		{
			return true;
		}
	}
	return false;
}

UStoryEncounterMap* MakeEncounterMap(FName EncounterId, FName NodeId)
{
	UStoryEncounterMap* Map = NewObject<UStoryEncounterMap>();
	Map->EncounterId = EncounterId;

	FStoryEncounterNode Node;
	Node.NodeId = NodeId;
	Node.DisplayName = FText::FromString(TEXT("Weapon Echo"));
	FStoryEncounterAction HintAction;
	HintAction.Kind = EStoryEncounterActionKind::WeakHint;
	HintAction.Body = FText::FromString(TEXT("Try attacking the memory echo."));
	Node.Actions.Add(HintAction);
	Map->Nodes.Add(Node);
	return Map;
}

UStoryProductionBoardDA* MakeProductionBoard(FName RequirementId, FName EncounterId, FName NodeId)
{
	UStoryProductionBoardDA* Board = NewObject<UStoryProductionBoardDA>();
	FStoryProductionRow Row;
	Row.RequirementId = RequirementId;
	Row.FlowId = FName(TEXT("MemoryTutorial"));
	Row.PointName = FText::FromString(TEXT("See Weapon Echo"));
	Row.EncounterId = EncounterId;
	Row.NodeId = NodeId;
	Row.Status = EStoryProductionStatus::InEncounterMap;
	Board->Rows.Add(Row);
	return Board;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelValidatesLinkedBoardTest,
	"DevKit.StoryEncounterEditor.ValidatesLinkedProductionBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelValidatesLinkedBoardTest::RunTest(const FString& Parameters)
{
	UStoryEncounterMap* Map = MakeEncounterMap(TEXT("EM_MemoryTutorial_PreRun"), TEXT("weapon_echo"));
	UStoryProductionBoardDA* Board = MakeProductionBoard(TEXT("REQ_001"), Map->EncounterId, TEXT("weapon_echo"));

	const TArray<FStoryEncounterWorkbenchMessage> Messages = FStoryEncounterEditorModel::Validate({ Board }, { Map });

	for (const FStoryEncounterWorkbenchMessage& Message : Messages)
	{
		TestNotEqual(TEXT("No errors expected for a valid linked board row"),
			Message.Severity,
			EStoryEncounterWorkbenchMessageSeverity::Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelFindsMissingMapTest,
	"DevKit.StoryEncounterEditor.FindsMissingEncounterMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelFindsMissingMapTest::RunTest(const FString& Parameters)
{
	UStoryProductionBoardDA* Board = MakeProductionBoard(TEXT("REQ_001"), TEXT("MissingMap"), TEXT("weapon_echo"));

	const TArray<FStoryEncounterWorkbenchMessage> Messages = FStoryEncounterEditorModel::Validate({ Board }, {});

	TestTrue(TEXT("Reports missing encounter map"), HasMessageContaining(Messages, TEXT("MissingMap")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelFindsMissingNodeTest,
	"DevKit.StoryEncounterEditor.FindsMissingNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelFindsMissingNodeTest::RunTest(const FString& Parameters)
{
	UStoryEncounterMap* Map = MakeEncounterMap(TEXT("EM_MemoryTutorial_PreRun"), TEXT("weapon_echo"));
	UStoryProductionBoardDA* Board = MakeProductionBoard(TEXT("REQ_001"), Map->EncounterId, TEXT("missing_node"));

	const TArray<FStoryEncounterWorkbenchMessage> Messages = FStoryEncounterEditorModel::Validate({ Board }, { Map });

	TestTrue(TEXT("Reports missing node"), HasMessageContaining(Messages, TEXT("missing_node")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelFindsDuplicateRequirementTest,
	"DevKit.StoryEncounterEditor.FindsDuplicateRequirementId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelFindsDuplicateRequirementTest::RunTest(const FString& Parameters)
{
	UStoryEncounterMap* Map = MakeEncounterMap(TEXT("EM_MemoryTutorial_PreRun"), TEXT("weapon_echo"));
	UStoryProductionBoardDA* Board = MakeProductionBoard(TEXT("REQ_001"), Map->EncounterId, TEXT("weapon_echo"));
	const FStoryProductionRow DuplicateRow = Board->Rows[0];
	Board->Rows.Add(DuplicateRow);

	const TArray<FStoryEncounterWorkbenchMessage> Messages = FStoryEncounterEditorModel::Validate({ Board }, { Map });

	TestTrue(TEXT("Reports duplicate requirement id"), HasMessageContaining(Messages, TEXT("REQ_001")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelListsEmptyBoardsTest,
	"DevKit.StoryEncounterEditor.ListsEmptyProductionBoardAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelListsEmptyBoardsTest::RunTest(const FString& Parameters)
{
	UStoryProductionBoardDA* Board = NewObject<UStoryProductionBoardDA>();

	const TArray<FStoryEncounterBoardItem> Items = FStoryEncounterEditorModel::BuildBoardItems({ Board });

	TestEqual(TEXT("Empty board asset is still visible"), Items.Num(), 1);
	TestTrue(TEXT("Board pointer is kept"), Items[0].Board.Get() == Board);
	TestEqual(TEXT("Row count is zero"), Items[0].RowCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelInitializesNewEncounterMapTest,
	"DevKit.StoryEncounterEditor.InitializesNewEncounterMapWithEntryNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelInitializesNewEncounterMapTest::RunTest(const FString& Parameters)
{
	UStoryEncounterMap* Map = NewObject<UStoryEncounterMap>();

	FStoryEncounterEditorModel::InitializeNewEncounterMap(Map, TEXT("EM_TestEncounter"));

	TestEqual(TEXT("Encounter id is set"), Map->EncounterId, FName(TEXT("EM_TestEncounter")));
	TestEqual(TEXT("Creates one entry node"), Map->Nodes.Num(), 1);
	TestEqual(TEXT("Entry node id"), Map->Nodes[0].NodeId, FName(TEXT("entry")));
	TestEqual(TEXT("Entry node starts with weak hint"), Map->Nodes[0].Actions.Num(), 1);
	TestEqual(TEXT("Weak hint action"), Map->Nodes[0].Actions[0].Kind, EStoryEncounterActionKind::WeakHint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelInitializesNewEncounterPointTest,
	"DevKit.StoryEncounterEditor.InitializesNewEncounterPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelInitializesNewEncounterPointTest::RunTest(const FString& Parameters)
{
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>();

	FStoryEncounterEditorModel::InitializeNewEncounterPoint(Point, TEXT("EM_TestEncounter"), TEXT("entry"));

	TestEqual(TEXT("Encounter id is set"), Point->EncounterId, FName(TEXT("EM_TestEncounter")));
	TestEqual(TEXT("Node id is set"), Point->NodeId, FName(TEXT("entry")));
	TestEqual(TEXT("Point starts with weak hint"), Point->Actions.Num(), 1);
	TestEqual(TEXT("Weak hint action"), Point->Actions[0].Kind, EStoryEncounterActionKind::WeakHint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterEditorModelInitializesNewEncounterGraphTest,
	"DevKit.StoryEncounterEditor.InitializesNewEncounterGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterEditorModelInitializesNewEncounterGraphTest::RunTest(const FString& Parameters)
{
	UStoryEncounterGraph* Graph = NewObject<UStoryEncounterGraph>();

	FStoryEncounterEditorModel::InitializeNewEncounterGraph(Graph, TEXT("EM_TestEncounter"));

	TestEqual(TEXT("Encounter id is set"), Graph->EncounterId, FName(TEXT("EM_TestEncounter")));
	TestTrue(TEXT("Display name is set"), !Graph->DisplayName.IsEmpty());
	TestEqual(TEXT("Graph node type is story node"), Graph->NodeType.Get(), UStoryEncounterGraphNode::StaticClass());
	return true;
}
