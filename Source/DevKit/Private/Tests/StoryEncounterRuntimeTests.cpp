#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "FlowAsset.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "Story/Flow/Nodes/SNode_ActivateTutorialSpawner.h"
#include "Story/Flow/Nodes/SNode_ShowHint.h"
#include "Story/Flow/Nodes/SNode_SpawnRewardPickup.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "LevelFlow/Nodes/LENode_GetStoryContext.h"
#include "LevelFlow/Nodes/LENode_SpawnRewardPickup.h"
#include "Map/RewardPickup.h"
#include "Character/TrainingDummyCharacter.h"
#include "Story/Encounter/StoryEncounterMap.h"
#include "Story/Encounter/StoryEncounterDeathListener.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryFlowProxy.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"
#include "Story/Encounter/StoryProductionBoard.h"
#include "Types/FlowDataPinResults.h"
#include "UI/DialogContentDA.h"

namespace StoryEncounterRuntimeTests
{
AStoryFlowProxy* FindProxyForSource(UWorld* World, const AActor* SourceActor)
{
	if (!World || !SourceActor)
	{
		return nullptr;
	}

	for (TActorIterator<AStoryFlowProxy> It(World); It; ++It)
	{
		if (It->GetContextSourceActor() == SourceActor)
		{
			return *It;
		}
	}

	return nullptr;
}

void DestroyStoryFlowProxies(UWorld* World)
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AStoryFlowProxy> It(World); It; ++It)
	{
		It->Destroy();
	}
}

void DestroyRewardPickups(UWorld* World)
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<ARewardPickup> It(World); It; ++It)
	{
		It->Destroy();
	}
}

AActor* SpawnTestActorWithTransform(UWorld* World, const FTransform& Transform)
{
	AActor* Actor = World ? World->SpawnActor<AActor>() : nullptr;
	if (!Actor)
	{
		return nullptr;
	}

	USceneComponent* RootComponent = NewObject<USceneComponent>(Actor, TEXT("TestRoot"));
	Actor->SetRootComponent(RootComponent);
	RootComponent->RegisterComponent();
	Actor->SetActorTransform(Transform);
	return Actor;
}
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterPreservesDottedProgressKeyTest,
	"DevKit.StoryEncounter.PreservesDottedProgressKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterPreservesDottedProgressKeyTest::RunTest(const FString& Parameters)
{
	FStoryEncounterAction EncounterAction;
	EncounterAction.Kind = EStoryEncounterActionKind::RecordProgress;
	EncounterAction.ProgressKey = TEXT("first_run.weapon_picked");

	FStoryAction StoryAction;
	const bool bConverted = UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(
		TEXT("EM_FirstRun_Tutorial"),
		EncounterAction,
		StoryAction);

	TestTrue(TEXT("Dotted progress key converts"), bConverted);
	TestTrue(TEXT("Generated dotted flag is valid"), StoryAction.FlagTag.IsValid());
	TestEqual(TEXT("Generated dotted tag name is stable"),
		UStoryEncounterRuntimeSubsystem::MakeProgressTagName(TEXT("EM_FirstRun_Tutorial"), TEXT("first_run.weapon_picked")),
		FString(TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.weapon_picked")));
	return true;
}

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
	TestTrue(TEXT("Weak hint title is editor-only"), StoryAction.HintTitle.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterConvertsTutorialAreaHintTest,
	"DevKit.StoryEncounter.ConvertsTutorialAreaHintToPersistentInfoHint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterConvertsTutorialAreaHintTest::RunTest(const FString& Parameters)
{
	FStoryEncounterAction EncounterAction;
	EncounterAction.Kind = EStoryEncounterActionKind::TutorialAreaHint;
	EncounterAction.Body = FText::FromString(TEXT("<input action=\"Dash\"/>"));

	FStoryAction StoryAction;
	const bool bConverted = UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(
		TEXT("EM_FirstRun_Tutorial"),
		EncounterAction,
		StoryAction);

	TestTrue(TEXT("Action converts"), bConverted);
	TestEqual(TEXT("Converts to ShowInfoHint"), StoryAction.Type, EStoryActionType::ShowInfoHint);
	TestEqual(TEXT("Area hint does not auto-close"), StoryAction.HintDuration, 0.f);
	TestTrue(TEXT("Area hint body is retained"), StoryAction.HintText.EqualTo(EncounterAction.Body));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterConvertsTutorialPopupTest,
	"DevKit.StoryEncounter.ConvertsTutorialPopupAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterConvertsTutorialPopupTest::RunTest(const FString& Parameters)
{
	FStoryEncounterAction EncounterAction;
	EncounterAction.Kind = EStoryEncounterActionKind::TutorialPopup;
	EncounterAction.TutorialEventId = TEXT("tutorial_weapon_pickup");
	EncounterAction.bPauseGame = false;
	FTutorialPage InlinePage;
	InlinePage.Title = FText::FromString(TEXT("Inline"));
	InlinePage.Body = FText::FromString(TEXT("Inline body"));
	EncounterAction.TutorialPages.Add(InlinePage);

	FStoryAction StoryAction;
	const bool bConverted = UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(
		TEXT("EM_FirstRun_Tutorial"),
		EncounterAction,
		StoryAction);

	TestTrue(TEXT("Action converts"), bConverted);
	TestEqual(TEXT("Converts to ShowTutorialPopup"), StoryAction.Type, EStoryActionType::ShowTutorialPopup);
	TestEqual(TEXT("Tutorial event id is retained"), StoryAction.TutorialEventId, FName(TEXT("tutorial_weapon_pickup")));
	TestEqual(TEXT("Inline tutorial pages are retained"), StoryAction.TutorialPages.Num(), 1);
	TestFalse(TEXT("Pause flag is retained"), StoryAction.bPauseGame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterConvertsPlayLevelFlowStopExistingTest,
	"DevKit.StoryEncounter.ConvertsPlayLevelFlowStopExistingFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterConvertsPlayLevelFlowStopExistingTest::RunTest(const FString& Parameters)
{
	FStoryEncounterAction EncounterAction;
	EncounterAction.Kind = EStoryEncounterActionKind::PlayLevelFlow;
	EncounterAction.LevelFlow = NewObject<ULevelFlowAsset>();
	EncounterAction.bStopExistingStoryFlow = false;

	FStoryAction StoryAction;
	const bool bConverted = UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(
		TEXT("EM_MemoryTutorial_PreRun"),
		EncounterAction,
		StoryAction);

	TestTrue(TEXT("Action converts"), bConverted);
	TestEqual(TEXT("Converts to PlayLevelFlow"), StoryAction.Type, EStoryActionType::PlayLevelFlow);
	TestEqual(TEXT("Level flow is retained"), StoryAction.LevelFlow.Get(), EncounterAction.LevelFlow.Get());
	TestFalse(TEXT("Stop-existing flag is retained"), StoryAction.bStopExistingStoryFlow);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryFlowProxyStoresContextSnapshotTest,
	"DevKit.StoryEncounter.FlowProxyStoresContextSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryFlowProxyStoresContextSnapshotTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* SourceActor = World->SpawnActor<AActor>();
	AStoryFlowProxy* Proxy = World->SpawnActor<AStoryFlowProxy>();
	TestNotNull(TEXT("Source actor spawned"), SourceActor);
	TestNotNull(TEXT("Proxy spawned"), Proxy);
	if (!SourceActor || !Proxy)
	{
		return false;
	}

	const FTransform Snapshot(FRotator(0.f, 45.f, 0.f), FVector(10.f, 20.f, 30.f), FVector(1.f, 1.f, 1.f));
	Proxy->ContextSourceActor = SourceActor;
	Proxy->ContextTransform = Snapshot;
	Proxy->ContextPlayerController = nullptr;

	TestEqual(TEXT("Proxy returns source actor"), Proxy->GetContextSourceActor(), SourceActor);
	TestTrue(TEXT("Proxy returns transform snapshot"), Proxy->GetContextTransform().Equals(Snapshot));
	TestNull(TEXT("Proxy returns optional player controller"), Proxy->GetContextPlayerController());

	Proxy->Destroy();
	SourceActor->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLENodeGetStoryContextSuppliesFallbackDataTest,
	"DevKit.StoryEncounter.GetStoryContextSuppliesFallbackData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLENodeGetStoryContextSuppliesFallbackDataTest::RunTest(const FString& Parameters)
{
	ULENode_GetStoryContext* Node = NewObject<ULENode_GetStoryContext>();
	TestNotNull(TEXT("Node constructs"), Node);
	if (!Node)
	{
		return false;
	}

	TestTrue(TEXT("Node can supply data pins"),
		IFlowDataPinValueSupplierInterface::Execute_CanSupplyDataPinValues(Node));

	const FFlowDataPinResult_Transform TransformResult =
		IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsTransform(Node, TEXT("ContextTransform"));
	TestEqual(TEXT("Known transform pin resolves"), TransformResult.Result, EFlowDataPinResolveResult::Success);
	TestTrue(TEXT("Missing proxy falls back to identity transform"), TransformResult.Value.Equals(FTransform::Identity));

	const FFlowDataPinResult_Object SourceActorResult =
		IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsObject(Node, TEXT("SourceActor"));
	TestEqual(TEXT("Known source actor pin resolves"), SourceActorResult.Result, EFlowDataPinResolveResult::Success);
	TestNull(TEXT("Missing proxy has null source actor"), SourceActorResult.Value.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterNodeEventFlowRunsViaProxyTest,
	"DevKit.StoryEncounter.NodeEventFlowRunsViaProxy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterNodeEventFlowRunsViaProxyTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	StoryEncounterRuntimeTests::DestroyStoryFlowProxies(World);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEncounterRuntimeSubsystem* Runtime = NewObject<UStoryEncounterRuntimeSubsystem>(GameInstance);
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>();
	Point->EncounterId = TEXT("EM_Test");
	Point->NodeId = TEXT("node_event_flow");
	Point->NodeEventFlow = NewObject<UStoryFlowAsset>(Point);

	const FTransform Snapshot(FRotator(0.f, 90.f, 0.f), FVector(100.f, 200.f, 300.f), FVector::OneVector);
	AActor* SourceActor = StoryEncounterRuntimeTests::SpawnTestActorWithTransform(World, Snapshot);
	TestNotNull(TEXT("Source actor spawned"), SourceActor);
	if (!SourceActor)
	{
		return false;
	}

	TestTrue(TEXT("Point triggers"), Runtime->TriggerEncounterPoint(Point, SourceActor));

	AStoryFlowProxy* Proxy = StoryEncounterRuntimeTests::FindProxyForSource(World, SourceActor);
	TestNotNull(TEXT("NodeEventFlow creates a proxy"), Proxy);
	if (Proxy)
	{
		TestEqual(TEXT("Proxy stores source actor"), Proxy->GetContextSourceActor(), SourceActor);
		TestTrue(TEXT("Proxy stores trigger transform snapshot"), Proxy->GetContextTransform().Equals(Snapshot));
		Proxy->Destroy();
	}

	SourceActor->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLENodeSpawnRewardPickupUsesStoryContextTransformTest,
	"DevKit.StoryEncounter.SpawnRewardPickupUsesStoryContextTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterOncePointSkipsSecondRewardDropTest,
	"DevKit.StoryEncounter.OncePointSkipsSecondRewardDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLENodeSpawnRewardPickupUsesStoryContextTransformTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	StoryEncounterRuntimeTests::DestroyRewardPickups(World);

	ULENode_SpawnRewardPickup* Node = NewObject<ULENode_SpawnRewardPickup>();
	TestNotNull(TEXT("Spawn reward node constructs"), Node);
	if (!Node)
	{
		return false;
	}

	FLootOption Option;
	Option.LootType = ELootType::Rune;
	Option.DisplayName = FText::FromString(TEXT("Heavy Card"));
	Node->RewardPickupClass = ARewardPickup::StaticClass();
	Node->RewardLootOptions.Add(Option);
	Node->RewardSpawnOffset = FVector(120.f, 0.f, 20.f);
	Node->RewardPickupCount = 1;
	Node->bAllowPickupOutsideArrangement = true;

	const FTransform ContextTransform(FRotator::ZeroRotator, FVector(10.f, 20.f, 30.f), FVector::OneVector);
	TestTrue(TEXT("Reward pickup spawns from supplied context transform"),
		Node->SpawnRewardPickupAtContext(World, ContextTransform));

	ARewardPickup* FoundPickup = nullptr;
	for (TActorIterator<ARewardPickup> It(World); It; ++It)
	{
		FoundPickup = *It;
		break;
	}

	TestNotNull(TEXT("Reward pickup spawned"), FoundPickup);
	if (FoundPickup)
	{
		TestTrue(TEXT("Pickup uses context transform plus configured offset"),
			FoundPickup->GetActorLocation().Equals(FVector(130.f, 20.f, 50.f), KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Pickup allows tutorial collection outside arrangement"),
			FoundPickup->bAllowPickupOutsideArrangement);
		TestTrue(TEXT("Pickup receives fixed loot options"), FoundPickup->bUseFixedLootOptions);
		TestEqual(TEXT("Fixed loot option count"), FoundPickup->FixedLootOptions.Num(), 1);
		FoundPickup->Destroy();
	}

	return true;
}

bool FStoryEncounterOncePointSkipsSecondRewardDropTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	StoryEncounterRuntimeTests::DestroyRewardPickups(World);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEncounterRuntimeSubsystem* Runtime = NewObject<UStoryEncounterRuntimeSubsystem>(GameInstance);
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>();
	Point->EncounterId = TEXT("EM_Test");
	Point->NodeId = TEXT("dummy_death_drop_once");
	Point->Kind = EStoryEncounterNodeKind::Death;
	Point->FirePolicy = EStoryEncounterFirePolicy::Once;

	FStoryEncounterAction DropAction;
	DropAction.Kind = EStoryEncounterActionKind::SpawnRewardPickup;
	DropAction.RewardPickupClass = ARewardPickup::StaticClass();
	DropAction.RewardPickupCount = 1;
	DropAction.bSpawnRewardOnTargetDeath = false;
	FLootOption LootOption;
	LootOption.LootType = ELootType::Rune;
	LootOption.DisplayName = FText::FromString(TEXT("Heavy Card"));
	DropAction.RewardLootOptions.Add(LootOption);
	Point->Actions.Add(DropAction);

	AActor* SourceActor = StoryEncounterRuntimeTests::SpawnTestActorWithTransform(World, FTransform::Identity);
	TestNotNull(TEXT("Source actor spawned"), SourceActor);
	if (!SourceActor)
	{
		return false;
	}

	TestTrue(TEXT("First death trigger spawns reward"), Runtime->TriggerEncounterPoint(Point, SourceActor));
	TestFalse(TEXT("Second death trigger is blocked by Once policy"), Runtime->TriggerEncounterPoint(Point, SourceActor));

	int32 PickupCount = 0;
	for (TActorIterator<ARewardPickup> It(World); It; ++It)
	{
		++PickupCount;
	}

	TestEqual(TEXT("Once death point drops only one reward pickup"), PickupCount, 1);

	StoryEncounterRuntimeTests::DestroyRewardPickups(World);
	SourceActor->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterDeathListenerRunsNodeEventFlowTest,
	"DevKit.StoryEncounter.DeathListenerRunsNodeEventFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterDeathListenerRunsNodeEventFlowTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone(TEXT("StoryEncounterDeathListenerTest"));
	UWorld* World = GameInstance->GetWorld();
	ON_SCOPE_EXIT
	{
		GameInstance->Shutdown();
		if (World)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	TestNotNull(TEXT("Standalone test world exists"), World);
	if (!World)
	{
		return false;
	}

	StoryEncounterRuntimeTests::DestroyStoryFlowProxies(World);

	UStoryEncounterRuntimeSubsystem* Runtime = GameInstance->GetSubsystem<UStoryEncounterRuntimeSubsystem>();
	TestNotNull(TEXT("Story runtime subsystem is available from game instance"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	ATrainingDummyCharacter* Dummy = World->SpawnActor<ATrainingDummyCharacter>(
		ATrainingDummyCharacter::StaticClass(),
		FVector(300.f, 400.f, 500.f),
		FRotator::ZeroRotator);
	AStoryEncounterDeathListener* Listener = World->SpawnActor<AStoryEncounterDeathListener>();
	TestNotNull(TEXT("Dummy spawned"), Dummy);
	TestNotNull(TEXT("Death listener spawned"), Listener);
	if (!Dummy || !Listener)
	{
		return false;
	}
	Dummy->Tags.Add(TEXT("StoryTest.DeathListenerDummy"));

	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>(Listener);
	Point->EncounterId = TEXT("EM_Test");
	Point->NodeId = TEXT("dummy_death_drop");
	Point->NodeEventFlow = NewObject<UStoryFlowAsset>(Point);

	Listener->EncounterPoint = Point;
	Listener->TargetActorTag = TEXT("StoryTest.DeathListenerDummy");
	TestEqual(TEXT("Death listener binds matching dummy"), Listener->BindMatchingTargets(), 1);
	TestTrue(TEXT("Death listener delegate is bound to dummy"), Listener->IsBoundToTarget(Dummy));

	Dummy->FinishDying();
	TestTrue(TEXT("Death listener receives dummy death"), Listener->HasTriggered());

	AStoryFlowProxy* Proxy = StoryEncounterRuntimeTests::FindProxyForSource(World, Dummy);
	TestNotNull(TEXT("Death listener runs NodeEventFlow through proxy"), Proxy);
	if (Proxy)
	{
		TestTrue(TEXT("Proxy source is dead dummy"), Proxy->GetContextSourceActor() == Dummy);
		TestTrue(TEXT("Proxy snapshots dummy death transform"),
			Proxy->GetContextTransform().GetLocation().Equals(Dummy->GetActorLocation(), KINDA_SMALL_NUMBER));
		Proxy->Destroy();
	}

	Listener->Destroy();
	Dummy->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterPlayLevelFlowNonTriggerRunsViaProxyTest,
	"DevKit.StoryEncounter.PlayLevelFlowNonTriggerRunsViaProxy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterWeaponPickupTutorialPointsAtDummyTest,
	"DevKit.StoryEncounter.FirstRunWeaponPickupTutorialPointsAtDummy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterWeaponPickupTutorialPointsAtDummyTest::RunTest(const FString& Parameters)
{
	const UDialogContentDA* Tutorial = LoadObject<UDialogContentDA>(
		nullptr,
		TEXT("/Game/Docs/UI/Tutorial/DA_Tutorial_WeaponPickup.DA_Tutorial_WeaponPickup"));

	TestNotNull(TEXT("Weapon pickup tutorial asset loads"), Tutorial);
	if (!Tutorial)
	{
		return false;
	}

	TestTrue(TEXT("Weapon pickup tutorial has an extra dummy-combat page"), Tutorial->Pages.Num() >= 3);
	if (Tutorial->Pages.Num() < 3)
	{
		return false;
	}

	const FString ThirdPageBody = Tutorial->Pages[2].Body.ToString();
	TestTrue(TEXT("Third page asks the player to attack the training dummy"),
		ThirdPageBody.Contains(TEXT("木头人")) || ThirdPageBody.Contains(TEXT("dummy"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Third page mentions attacking"),
		ThirdPageBody.Contains(TEXT("攻击")) || ThirdPageBody.Contains(TEXT("attack"), ESearchCase::IgnoreCase));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterDummyDeathFlowDropsHeavyAndHintsPickupTest,
	"DevKit.StoryEncounter.DummyDeathFlowDropsHeavyAndHintsPickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEncounterTutorialDummySpawnBindsKillPointTest,
	"DevKit.StoryEncounter.TutorialDummySpawnBindsKillPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEncounterDummyDeathFlowDropsHeavyAndHintsPickupTest::RunTest(const FString& Parameters)
{
	const UStoryFlowAsset* Flow = LoadObject<UStoryFlowAsset>(
		nullptr,
		TEXT("/Game/Story/Flows/Tutorial/FA_DummyDeath_DropHeavyCard.FA_DummyDeath_DropHeavyCard"));

	TestNotNull(TEXT("Dummy death Story FA loads"), Flow);
	if (!Flow)
	{
		return false;
	}

	bool bDropsHeavy = false;
	bool bDropsKnockback = false;
	bool bHasPickupHint = false;
	for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
	{
		if (const USNode_SpawnRewardPickup* SpawnNode = Cast<USNode_SpawnRewardPickup>(Pair.Value))
		{
			for (const FLootOption& Option : SpawnNode->RewardLootOptions)
			{
				const FString RunePath = GetPathNameSafe(Option.RuneAsset);
				bDropsHeavy |= RunePath.Contains(TEXT("DA_Rune512_Heavy"));
				bDropsKnockback |= RunePath.Contains(TEXT("DA_Rune512_Knockback"));
			}
		}

		if (const USNode_ShowHint* HintNode = Cast<USNode_ShowHint>(Pair.Value))
		{
			const FString HintBody = HintNode->HintText.ToString();
			bHasPickupHint |= HintBody.Contains(TEXT("拾取")) || HintBody.Contains(TEXT("pick"), ESearchCase::IgnoreCase);
		}
	}

	TestTrue(TEXT("Dummy death flow drops Heavy card"), bDropsHeavy);
	TestFalse(TEXT("Dummy death flow must not drop Knockback card"), bDropsKnockback);
	TestTrue(TEXT("Dummy death flow shows pickup weak hint after the drop"), bHasPickupHint);
	return true;
}

bool FStoryEncounterTutorialDummySpawnBindsKillPointTest::RunTest(const FString& Parameters)
{
	const UStoryFlowAsset* ActivateFlow = LoadObject<UStoryFlowAsset>(
		nullptr,
		TEXT("/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner.FA_ActivateTutorialDummySpawner"));
	const UStoryEncounterPointDA* KillPoint = LoadObject<UStoryEncounterPointDA>(
		nullptr,
		TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo.EP_FirstRun_TrainingDummyCombo"));

	TestNotNull(TEXT("Tutorial dummy activation Story FA loads"), ActivateFlow);
	TestNotNull(TEXT("Training dummy kill point loads"), KillPoint);
	if (!ActivateFlow || !KillPoint)
	{
		return false;
	}

	const USNode_ActivateTutorialSpawner* ActivateNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : ActivateFlow->GetNodes())
	{
		if (const USNode_ActivateTutorialSpawner* Candidate = Cast<USNode_ActivateTutorialSpawner>(Pair.Value))
		{
			ActivateNode = Candidate;
			break;
		}
	}

	TestNotNull(TEXT("Tutorial dummy activation flow has ActivateTutorialSpawner node"), ActivateNode);
	if (!ActivateNode)
	{
		return false;
	}

	TestTrue(TEXT("ActivateTutorialSpawner binds the dummy kill point"),
		ActivateNode->OnKillEncounterPoint.Get() == KillPoint);
	TestTrue(TEXT("ActivateTutorialSpawner uses BP_Enemy_DummyTraining"),
		GetPathNameSafe(ActivateNode->EnemyClassOverride.Get()).Contains(TEXT("BP_Enemy_DummyTraining")));

	TestEqual(TEXT("Training dummy kill point runs on death"),
		KillPoint->Kind,
		EStoryEncounterNodeKind::Death);
	TestEqual(TEXT("Training dummy kill point has no progress gate"),
		KillPoint->Condition.Kind,
		EStoryEncounterConditionKind::None);
	TestNotNull(TEXT("Training dummy kill point runs the dummy death Story FA"), KillPoint->NodeEventFlow.Get());
	if (KillPoint->NodeEventFlow)
	{
		TestTrue(TEXT("Training dummy kill point uses Heavy card drop flow"),
			GetPathNameSafe(KillPoint->NodeEventFlow.Get()).Contains(TEXT("FA_DummyDeath_DropHeavyCard")));
	}

	const bool bHasInlineDelayedRewardDrop = KillPoint->Actions.ContainsByPredicate(
		[](const FStoryEncounterAction& Action)
		{
			return Action.Kind == EStoryEncounterActionKind::SpawnRewardPickup;
		});
	TestFalse(TEXT("Training dummy kill point does not keep the old inline delayed reward drop"),
		bHasInlineDelayedRewardDrop);
	return true;
}

bool FStoryEncounterPlayLevelFlowNonTriggerRunsViaProxyTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	StoryEncounterRuntimeTests::DestroyStoryFlowProxies(World);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEncounterRuntimeSubsystem* Runtime = NewObject<UStoryEncounterRuntimeSubsystem>(GameInstance);
	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>();
	Point->EncounterId = TEXT("EM_Test");
	Point->NodeId = TEXT("play_level_flow");

	FStoryEncounterAction Action;
	Action.Kind = EStoryEncounterActionKind::PlayLevelFlow;
	Action.LevelFlow = NewObject<ULevelFlowAsset>(Point);
	Point->Actions.Add(Action);

	AActor* SourceActor = StoryEncounterRuntimeTests::SpawnTestActorWithTransform(World, FTransform::Identity);
	TestNotNull(TEXT("Source actor spawned"), SourceActor);
	if (!SourceActor)
	{
		return false;
	}

	TestTrue(TEXT("Point triggers"), Runtime->TriggerEncounterPoint(Point, SourceActor));

	AStoryFlowProxy* Proxy = StoryEncounterRuntimeTests::FindProxyForSource(World, SourceActor);
	TestNotNull(TEXT("PlayLevelFlow from a non-trigger source creates a proxy"), Proxy);
	if (Proxy)
	{
		TestEqual(TEXT("Proxy stores source actor"), Proxy->GetContextSourceActor(), SourceActor);
		Proxy->Destroy();
	}

	SourceActor->Destroy();
	return true;
}

#endif
