#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "GameModes/YogGameMode.h"
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

#endif
