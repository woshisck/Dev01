#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "Story/Flow/Nodes/SNode_SetNextRoomPlan.h"
#include "Story/StoryNextRoomPlanTypes.h"
#include "System/YogGameInstanceBase.h"

namespace StoryNextRoomPlanTests
{
FLootOption MakeGoldOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Gold;
	Option.Amount = Amount;
	Option.DisplayName = FText::Format(
		NSLOCTEXT("StoryNextRoomPlanTests", "GoldRewardFormat", "Gold x{0}"),
		FText::AsNumber(Amount));
	return Option;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryNextRoomPlanGameInstanceConsumedOnceTest,
	"DevKit.StoryNextRoomPlan.GameInstanceConsumedOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryNextRoomPlanGameInstanceConsumedOnceTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	URoomDataAsset* Room = NewObject<URoomDataAsset>(GI);
	TestNotNull(TEXT("Game instance exists"), GI);
	TestNotNull(TEXT("Room override exists"), Room);
	if (!GI || !Room)
	{
		return false;
	}

	FStoryNextRoomPlan Plan;
	Plan.bForceSinglePortal = true;
	Plan.PortalIndex = 2;
	Plan.RoomDataOverride = Room;
	Plan.bOverrideRewardOptions = true;
	Plan.RewardOptionsOverride = { StoryNextRoomPlanTests::MakeGoldOption(50) };

	GI->SetPendingStoryNextRoomPlan(Plan);

	FStoryNextRoomPlan PreviewPlan;
	TestTrue(TEXT("Pending next-room plan can be previewed"),
		GI->GetPendingStoryNextRoomPlan(PreviewPlan));
	TestTrue(TEXT("Preview plan forces a single portal"), PreviewPlan.bForceSinglePortal);
	TestEqual(TEXT("Preview plan portal index"), PreviewPlan.PortalIndex, 2);
	TestEqual(TEXT("Preview plan room override"), PreviewPlan.RoomDataOverride.Get(), Room);
	TestTrue(TEXT("Preview plan has reward override"), PreviewPlan.bOverrideRewardOptions);
	TestEqual(TEXT("Preview reward option count"), PreviewPlan.RewardOptionsOverride.Num(), 1);

	FStoryNextRoomPlan ConsumedPlan;
	TestTrue(TEXT("First consume returns pending next-room plan"),
		GI->ConsumePendingStoryNextRoomPlan(ConsumedPlan));
	TestEqual(TEXT("Consumed plan portal index"), ConsumedPlan.PortalIndex, 2);
	TestEqual(TEXT("Consumed plan room override"), ConsumedPlan.RoomDataOverride.Get(), Room);

	FStoryNextRoomPlan SecondConsume;
	TestFalse(TEXT("Pending next-room plan is one-shot"),
		GI->ConsumePendingStoryNextRoomPlan(SecondConsume));

	TArray<FLootOption> PendingRewardOptions;
	TestTrue(TEXT("Setting a next-room plan also seeds reward preview/room override data"),
		GI->GetPendingRoomRewardOptionsOverride(PendingRewardOptions));
	TestEqual(TEXT("Seeded reward count"), PendingRewardOptions.Num(), 1);
	if (PendingRewardOptions.Num() == 1)
	{
		TestEqual(TEXT("Seeded reward type"), PendingRewardOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Seeded reward amount"), PendingRewardOptions[0].Amount, 50);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryNextRoomPlanClearRunStateClearsPlanTest,
	"DevKit.StoryNextRoomPlan.ClearRunStateClearsPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryNextRoomPlanClearRunStateClearsPlanTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	TestNotNull(TEXT("Game instance exists"), GI);
	if (!GI)
	{
		return false;
	}

	FStoryNextRoomPlan Plan;
	Plan.bForceSinglePortal = true;
	Plan.PortalIndex = 1;
	GI->SetPendingStoryNextRoomPlan(Plan);
	GI->ClearRunState();

	FStoryNextRoomPlan RemainingPlan;
	TestFalse(TEXT("ClearRunState clears the pending next-room plan"),
		GI->GetPendingStoryNextRoomPlan(RemainingPlan));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryFlowSetNextRoomPlanStoresPendingPlanTest,
	"DevKit.StoryNextRoomPlan.StoryFlowNodeStoresPendingPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryFlowSetNextRoomPlanStoresPendingPlanTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	USNode_SetNextRoomPlan* Node = NewObject<USNode_SetNextRoomPlan>();
	TestNotNull(TEXT("Game instance exists"), GI);
	TestNotNull(TEXT("StoryFlow next-room plan node exists"), Node);
	if (!GI || !Node)
	{
		return false;
	}

	Node->bClearPlan = false;
	Node->Plan.bForceSinglePortal = true;
	Node->Plan.PortalIndex = 0;
	Node->Plan.bOverrideRewardOptions = true;
	Node->Plan.RewardOptionsOverride = { StoryNextRoomPlanTests::MakeGoldOption(50) };

	TestTrue(TEXT("StoryFlow node writes a pending next-room plan"),
		Node->ApplyNextRoomPlan(GI));

	FStoryNextRoomPlan StoredPlan;
	TestTrue(TEXT("Pending next-room plan exists after node application"),
		GI->GetPendingStoryNextRoomPlan(StoredPlan));
	TestTrue(TEXT("Stored plan forces a single portal"), StoredPlan.bForceSinglePortal);
	TestEqual(TEXT("Stored portal index"), StoredPlan.PortalIndex, 0);
	TestTrue(TEXT("Stored plan has reward override"), StoredPlan.bOverrideRewardOptions);
	TestEqual(TEXT("Stored reward option count"), StoredPlan.RewardOptionsOverride.Num(), 1);

	return true;
}

#endif
