#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "Story/FirstRunTutorialDirectorSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorBuildsMoonlightSecondRoomPlanTest,
	"DevKit.FirstRunTutorialDirector.BuildsMoonlightSecondRoomPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorBuildsMoonlightSecondRoomPlanTest::RunTest(const FString& Parameters)
{
	FStoryNextRoomPlan Plan;
	TestTrue(TEXT("GoldRoomCleared builds the second-room moonlight plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::GoldRoomCleared,
			Plan));

	TestTrue(TEXT("Moonlight second room plan forces a single portal"), Plan.bForceSinglePortal);
	TestEqual(TEXT("Default forced portal is index 0"), Plan.PortalIndex, 0);
	TestTrue(TEXT("Moonlight second room overrides reward options"), Plan.bOverrideRewardOptions);
	TestEqual(TEXT("Moonlight second room offers one card reward"), Plan.RewardOptionsOverride.Num(), 1);
	if (Plan.RewardOptionsOverride.Num() == 1)
	{
		TestEqual(TEXT("Moonlight reward is a rune"), Plan.RewardOptionsOverride[0].LootType, ELootType::Rune);
		TestNotNull(TEXT("Moonlight rune asset is loaded"), Plan.RewardOptionsOverride[0].RuneAsset.Get());
		TestNotNull(TEXT("Moonlight uses an explicit card preview icon"), Plan.RewardOptionsOverride[0].Icon.Get());
	}
	TestFalse(TEXT("Moonlight second room does not use a special enemy reward"), Plan.bMarkLastEnemyAsSpecialRewardEnemy);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorBuildsTransitionCurrencyPlansTest,
	"DevKit.FirstRunTutorialDirector.BuildsFixedPostGoldSequencePlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorBuildsTransitionCurrencyPlansTest::RunTest(const FString& Parameters)
{
	FStoryNextRoomPlan MaterialPlan;
	TestTrue(TEXT("BuffCardRoom builds third-room material plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::BuffCardRoom,
			MaterialPlan));

	TestTrue(TEXT("Material room forces a single portal"), MaterialPlan.bForceSinglePortal);
	TestTrue(TEXT("Material room overrides reward options"), MaterialPlan.bOverrideRewardOptions);
	TestEqual(TEXT("Material room has one reward option"), MaterialPlan.RewardOptionsOverride.Num(), 1);
	if (MaterialPlan.RewardOptionsOverride.Num() == 1)
	{
		TestEqual(TEXT("Third room reward is material"),
			MaterialPlan.RewardOptionsOverride[0].LootType,
			ELootType::Material);
	}
	TestFalse(TEXT("Material room does not use a special enemy reward"), MaterialPlan.bMarkLastEnemyAsSpecialRewardEnemy);

	FStoryNextRoomPlan FixedRunePlan;
	TestTrue(TEXT("MoonlightRoom builds fourth-room fixed rune plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::MoonlightRoom,
			FixedRunePlan));

	TestTrue(TEXT("Fixed rune room forces a single portal"), FixedRunePlan.bForceSinglePortal);
	TestTrue(TEXT("Fixed rune room overrides reward options"), FixedRunePlan.bOverrideRewardOptions);
	TestEqual(TEXT("Fixed rune room has three reward options"), FixedRunePlan.RewardOptionsOverride.Num(), 3);
	for (const FLootOption& Option : FixedRunePlan.RewardOptionsOverride)
	{
		TestEqual(TEXT("Fixed rune room option is a rune"), Option.LootType, ELootType::Rune);
		TestNotNull(TEXT("Fixed rune room rune asset is loaded"), Option.RuneAsset.Get());
		TestNotNull(TEXT("Fixed rune room uses an explicit card preview icon"), Option.Icon.Get());
	}
	TestTrue(TEXT("Fixed rune room carries the enemy attack buff"), FixedRunePlan.bOverrideBuffs);

	FStoryNextRoomPlan PrayerPlan;
	TestTrue(TEXT("TransitionRoom01 builds fifth-room prayer plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::TransitionRoom01,
			PrayerPlan));

	TestTrue(TEXT("Prayer room forces a single portal"), PrayerPlan.bForceSinglePortal);
	TestNotNull(TEXT("Prayer room plan loads DA_PrayRoom"), PrayerPlan.RoomDataOverride.Get());
	if (PrayerPlan.RoomDataOverride)
	{
		TestTrue(TEXT("Prayer room override points at DA_PrayRoom"),
			PrayerPlan.RoomDataOverride->GetPathName().Contains(TEXT("DA_PrayRoom")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorBuildsPostTutorialDeckTest,
	"DevKit.FirstRunTutorialDirector.BuildsPostTutorialDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorBuildsPostTutorialDeckTest::RunTest(const FString& Parameters)
{
	TArray<URuneDataAsset*> DeckAssets;
	UFirstRunTutorialDirectorSubsystem::BuildDefaultPostTutorialDeck(DeckAssets);

	TestEqual(TEXT("Post tutorial deck has four cards"), DeckAssets.Num(), 4);
	if (DeckAssets.Num() == 4)
	{
		TestNotNull(TEXT("First attack card exists"), DeckAssets[0]);
		TestNotNull(TEXT("Second attack card exists"), DeckAssets[1]);
		TestNotNull(TEXT("Moonlight card exists"), DeckAssets[2]);
		TestNotNull(TEXT("Finisher card exists"), DeckAssets[3]);
		TestEqual(TEXT("First two cards use the same attack asset"), DeckAssets[0], DeckAssets[1]);
	}

	return true;
}

#endif
