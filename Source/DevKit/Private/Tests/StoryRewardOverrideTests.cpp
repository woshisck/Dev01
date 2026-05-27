#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "Engine/World.h"
#include "GameModes/YogGameMode.h"
#include "Map/Portal.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Flow/Nodes/SNode_SetRoomRewardOverride.h"
#include "System/YogGameInstanceBase.h"

namespace StoryRewardOverrideTests
{
FLootOption MakeGoldOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Gold;
	Option.Amount = Amount;
	Option.DisplayName = FText::Format(
		NSLOCTEXT("StoryRewardOverrideTests", "GoldRewardFormat", "Gold x{0}"),
		FText::AsNumber(Amount));
	return Option;
}

FLootOption MakeMaterialOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Material;
	Option.Amount = Amount;
	Option.DisplayName = FText::Format(
		NSLOCTEXT("StoryRewardOverrideTests", "MaterialRewardFormat", "Material x{0}"),
		FText::AsNumber(Amount));
	return Option;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameInstancePendingRoomRewardOverrideConsumedOnceTest,
	"DevKit.StoryRewardOverride.GameInstancePendingOverrideConsumedOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameInstancePendingRoomRewardOverrideConsumedOnceTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	TestNotNull(TEXT("Game instance exists"), GI);
	if (!GI)
	{
		return false;
	}

	GI->SetPendingRoomRewardOptionsOverride({ StoryRewardOverrideTests::MakeGoldOption(50) });

	TArray<FLootOption> ConsumedOptions;
	TestTrue(TEXT("First consume returns pending reward override"),
		GI->ConsumePendingRoomRewardOptionsOverride(ConsumedOptions));
	TestEqual(TEXT("Consumed option count"), ConsumedOptions.Num(), 1);
	if (ConsumedOptions.Num() == 1)
	{
		TestEqual(TEXT("Consumed loot type is gold"), ConsumedOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Consumed gold amount"), ConsumedOptions[0].Amount, 50);
	}

	TArray<FLootOption> SecondConsume;
	TestFalse(TEXT("Pending reward override is one-shot"),
		GI->ConsumePendingRoomRewardOptionsOverride(SecondConsume));
	TestEqual(TEXT("Second consume returns no options"), SecondConsume.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameInstancePendingRoomRewardOverrideCanBePreviewedTest,
	"DevKit.StoryRewardOverride.GameInstancePendingOverrideCanBePreviewed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameInstancePendingRoomRewardOverrideCanBePreviewedTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	TestNotNull(TEXT("Game instance exists"), GI);
	if (!GI)
	{
		return false;
	}

	GI->SetPendingRoomRewardOptionsOverride({ StoryRewardOverrideTests::MakeGoldOption(50) });

	TArray<FLootOption> PreviewOptions;
	TestTrue(TEXT("Pending reward override can be read for preview"),
		GI->GetPendingRoomRewardOptionsOverride(PreviewOptions));
	TestEqual(TEXT("Preview option count"), PreviewOptions.Num(), 1);
	if (PreviewOptions.Num() == 1)
	{
		TestEqual(TEXT("Preview loot type is gold"), PreviewOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Preview gold amount"), PreviewOptions[0].Amount, 50);
	}

	TArray<FLootOption> ConsumedOptions;
	TestTrue(TEXT("Preview read does not consume pending reward override"),
		GI->ConsumePendingRoomRewardOptionsOverride(ConsumedOptions));
	TestEqual(TEXT("Consumed option count after preview"), ConsumedOptions.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameInstanceClearRunStateClearsPendingRoomRewardOverrideTest,
	"DevKit.StoryRewardOverride.ClearRunStateClearsPendingOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameInstanceClearRunStateClearsPendingRoomRewardOverrideTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	TestNotNull(TEXT("Game instance exists"), GI);
	if (!GI)
	{
		return false;
	}

	GI->SetPendingRoomRewardOptionsOverride({ StoryRewardOverrideTests::MakeGoldOption(50) });
	GI->ClearRunState();

	TArray<FLootOption> ConsumedOptions;
	TestFalse(TEXT("ClearRunState clears pending reward override"),
		GI->ConsumePendingRoomRewardOptionsOverride(ConsumedOptions));
	TestEqual(TEXT("No options remain after ClearRunState"), ConsumedOptions.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameModeAppliesPendingRoomRewardOverrideTest,
	"DevKit.StoryRewardOverride.GameModeAppliesPendingOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameModeAppliesPendingRoomRewardOverrideTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	AYogGameMode* GM = World->SpawnActor<AYogGameMode>();
	TestNotNull(TEXT("Game mode actor spawned"), GM);
	if (!GM)
	{
		return false;
	}

	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	GI->SetPendingRoomRewardOptionsOverride({ StoryRewardOverrideTests::MakeGoldOption(50) });

	TestTrue(TEXT("GameMode consumes pending override from GameInstance"),
		GM->ApplyPendingRoomRewardOptionsOverride(GI));
	TestTrue(TEXT("GameMode now has a room reward override"), GM->HasRoomRewardOptionsOverride());

	const TArray<FLootOption>& AppliedOptions = GM->GetRoomRewardOptionsOverride();
	TestEqual(TEXT("Applied option count"), AppliedOptions.Num(), 1);
	if (AppliedOptions.Num() == 1)
	{
		TestEqual(TEXT("Applied loot type is gold"), AppliedOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Applied gold amount"), AppliedOptions[0].Amount, 50);
	}

	TArray<FLootOption> RemainingOptions;
	TestFalse(TEXT("GameMode application consumes the pending data"),
		GI->ConsumePendingRoomRewardOptionsOverride(RemainingOptions));

	GM->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameModeKeepsPendingRoomRewardOverrideForHubRoomTest,
	"DevKit.StoryRewardOverride.GameModeKeepsPendingOverrideForHubRoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameModeKeepsPendingRoomRewardOverrideForHubRoomTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	AYogGameMode* GM = World->SpawnActor<AYogGameMode>();
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	URoomDataAsset* HubRoom = NewObject<URoomDataAsset>();
	TestNotNull(TEXT("Game mode actor spawned"), GM);
	TestNotNull(TEXT("Game instance exists"), GI);
	TestNotNull(TEXT("Hub room exists"), HubRoom);
	if (!GM || !GI || !HubRoom)
	{
		return false;
	}

	HubRoom->bIsHubRoom = true;
	GI->SetPendingRoomRewardOptionsOverride({ StoryRewardOverrideTests::MakeGoldOption(50) });

	TestFalse(TEXT("Hub room does not consume the next-room reward override"),
		GM->ApplyPendingRoomRewardOptionsOverrideForRoom(GI, HubRoom));
	TestFalse(TEXT("Hub room does not receive a current-room reward override"),
		GM->HasRoomRewardOptionsOverride());

	TArray<FLootOption> RemainingOptions;
	TestTrue(TEXT("Pending reward override remains available for portal preview and next room"),
		GI->ConsumePendingRoomRewardOptionsOverride(RemainingOptions));
	TestEqual(TEXT("Remaining option count"), RemainingOptions.Num(), 1);
	if (RemainingOptions.Num() == 1)
	{
		TestEqual(TEXT("Remaining pending reward is gold"), RemainingOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Remaining pending gold amount"), RemainingOptions[0].Amount, 50);
	}

	GM->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalRewardPreviewPrefersPendingOverrideTest,
	"DevKit.StoryRewardOverride.PortalPreviewPrefersPendingOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalRewardPreviewPrefersPendingOverrideTest::RunTest(const FString& Parameters)
{
	URoomDataAsset* Room = NewObject<URoomDataAsset>();
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	TestNotNull(TEXT("Room data exists"), Room);
	TestNotNull(TEXT("Game instance exists"), GI);
	if (!Room || !GI)
	{
		return false;
	}

	Room->bUseFixedRewardOptions = true;
	Room->FixedRewardOptions = { StoryRewardOverrideTests::MakeMaterialOption(1) };
	GI->SetPendingRoomRewardOptionsOverride({ StoryRewardOverrideTests::MakeGoldOption(50) });

	const TArray<FLootOption> PreviewOptions = APortal::BuildRewardPreviewOptionsForRoom(Room, GI);
	TestEqual(TEXT("Portal preview option count"), PreviewOptions.Num(), 1);
	if (PreviewOptions.Num() == 1)
	{
		TestEqual(TEXT("Portal preview uses pending gold override"), PreviewOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Portal preview gold amount"), PreviewOptions[0].Amount, 50);
	}

	TArray<FLootOption> ConsumedOptions;
	TestTrue(TEXT("Portal preview does not consume pending reward override"),
		GI->ConsumePendingRoomRewardOptionsOverride(ConsumedOptions));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryFlowSetRoomRewardOverrideNextRoomStoresPendingTest,
	"DevKit.StoryRewardOverride.StoryFlowNextRoomStoresPendingOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryFlowSetRoomRewardOverrideNextRoomStoresPendingTest::RunTest(const FString& Parameters)
{
	UYogGameInstanceBase* GI = NewObject<UYogGameInstanceBase>();
	USNode_SetRoomRewardOverride* Node = NewObject<USNode_SetRoomRewardOverride>();
	TestNotNull(TEXT("StoryFlow reward override node exists"), Node);
	if (!GI || !Node)
	{
		return false;
	}

	Node->OverrideTarget = EStoryRewardOverrideTarget::NextRoom;
	Node->bClearOverride = false;
	Node->LootOptions = { StoryRewardOverrideTests::MakeGoldOption(50) };

	TestTrue(TEXT("NextRoom node writes pending reward override"),
		Node->ApplyRewardOverride(nullptr, GI));

	TArray<FLootOption> ConsumedOptions;
	TestTrue(TEXT("Pending override is available after node application"),
		GI->ConsumePendingRoomRewardOptionsOverride(ConsumedOptions));
	TestEqual(TEXT("Pending option count"), ConsumedOptions.Num(), 1);
	if (ConsumedOptions.Num() == 1)
	{
		TestEqual(TEXT("Pending loot type is gold"), ConsumedOptions[0].LootType, ELootType::Gold);
		TestEqual(TEXT("Pending gold amount"), ConsumedOptions[0].Amount, 50);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunHubPortalSetsNextRoomGoldRewardTest,
	"DevKit.StoryRewardOverride.FirstRunHubPortalSetsNextRoomGoldReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunHubPortalSetsNextRoomGoldRewardTest::RunTest(const FString& Parameters)
{
	UStoryEncounterPointDA* Point = Cast<UStoryEncounterPointDA>(StaticLoadObject(
		UStoryEncounterPointDA::StaticClass(),
		nullptr,
		TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_HubPortalRewardPreview.EP_FirstRun_HubPortalRewardPreview")));
	TestNotNull(TEXT("First-run hub portal encounter point exists"), Point);
	if (!Point)
	{
		return false;
	}

	const FStoryEncounterAction* RewardAction = Point->Actions.FindByPredicate(
		[](const FStoryEncounterAction& Action)
		{
			return Action.Kind == EStoryEncounterActionKind::SetRoomRewardOverride
				&& Action.ActionId == TEXT("override_next_room01_gold_reward");
		});
	TestNotNull(TEXT("Hub portal encounter sets a room reward override"), RewardAction);
	if (!RewardAction)
	{
		return false;
	}

	TestEqual(TEXT("Reward override targets the next room"),
		RewardAction->RewardOverrideTarget,
		EStoryRewardOverrideTarget::NextRoom);
	TestFalse(TEXT("Reward override is not a clear action"), RewardAction->bClearRoomRewardOverride);
	TestEqual(TEXT("Reward option count"), RewardAction->RewardLootOptions.Num(), 1);
	if (RewardAction->RewardLootOptions.Num() == 1)
	{
		const FLootOption& Reward = RewardAction->RewardLootOptions[0];
		TestEqual(TEXT("First-run next-room reward is gold"), Reward.LootType, ELootType::Gold);
		TestEqual(TEXT("First-run next-room gold amount"), Reward.Amount, 50);
		TestFalse(TEXT("First-run next-room reward has a display name"), Reward.DisplayName.IsEmpty());
		TestNotNull(TEXT("First-run next-room reward has the gold icon"), Reward.Icon.Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunHubSeedsNextRoomGoldRewardBeforeCardPickupTest,
	"DevKit.StoryRewardOverride.FirstRunHubSeedsNextRoomGoldRewardBeforeCardPickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunHubSeedsNextRoomGoldRewardBeforeCardPickupTest::RunTest(const FString& Parameters)
{
	UStoryEncounterPointDA* Point = Cast<UStoryEncounterPointDA>(StaticLoadObject(
		UStoryEncounterPointDA::StaticClass(),
		nullptr,
		TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_HubMoveHint.EP_FirstRun_HubMoveHint")));
	TestNotNull(TEXT("First-run hub move encounter point exists"), Point);
	if (!Point)
	{
		return false;
	}

	TestEqual(TEXT("First-run hub reward seed is not gated by card pickup"),
		Point->Condition.Kind,
		EStoryEncounterConditionKind::None);

	const FStoryEncounterAction* RewardAction = Point->Actions.FindByPredicate(
		[](const FStoryEncounterAction& Action)
		{
			return Action.Kind == EStoryEncounterActionKind::SetRoomRewardOverride
				&& Action.ActionId == TEXT("seed_first_room_gold_reward");
		});
	TestNotNull(TEXT("Hub move encounter seeds the first combat room reward"), RewardAction);
	if (!RewardAction)
	{
		return false;
	}

	TestEqual(TEXT("Hub seed targets the next room"),
		RewardAction->RewardOverrideTarget,
		EStoryRewardOverrideTarget::NextRoom);
	TestFalse(TEXT("Hub seed is not a clear action"), RewardAction->bClearRoomRewardOverride);
	TestEqual(TEXT("Seeded reward option count"), RewardAction->RewardLootOptions.Num(), 1);
	if (RewardAction->RewardLootOptions.Num() == 1)
	{
		const FLootOption& Reward = RewardAction->RewardLootOptions[0];
		TestEqual(TEXT("Seeded first combat room reward is gold"), Reward.LootType, ELootType::Gold);
		TestEqual(TEXT("Seeded first combat room gold amount"), Reward.Amount, 50);
		TestNotNull(TEXT("Seeded first combat room reward has the gold icon"), Reward.Icon.Get());
	}

	return true;
}

#endif
