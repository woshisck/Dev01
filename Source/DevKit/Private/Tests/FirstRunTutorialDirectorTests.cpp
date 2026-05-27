#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Story/FirstRunTutorialDirectorSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorBuildsBuffCardRoomPlanTest,
	"DevKit.FirstRunTutorialDirector.BuildsBuffCardRoomPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorBuildsBuffCardRoomPlanTest::RunTest(const FString& Parameters)
{
	FStoryNextRoomPlan Plan;
	TestTrue(TEXT("GoldRoomCleared builds the next BuffCardRoom plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::GoldRoomCleared,
			Plan));

	TestTrue(TEXT("Buff card room plan forces a single portal"), Plan.bForceSinglePortal);
	TestEqual(TEXT("Default forced portal is index 0"), Plan.PortalIndex, 0);
	TestTrue(TEXT("Buff card room overrides reward options"), Plan.bOverrideRewardOptions);
	TestEqual(TEXT("Buff card room offers three card rewards"), Plan.RewardOptionsOverride.Num(), 3);

	for (const FLootOption& Option : Plan.RewardOptionsOverride)
	{
		TestEqual(TEXT("Buff card room option is a rune"), Option.LootType, ELootType::Rune);
		TestNotNull(TEXT("Buff card room rune asset is loaded"), Option.RuneAsset.Get());
		TestNotNull(TEXT("Buff card room uses an explicit card preview icon"), Option.Icon.Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorBuildsTransitionCurrencyPlansTest,
	"DevKit.FirstRunTutorialDirector.BuildsTransitionCurrencyPlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorBuildsTransitionCurrencyPlansTest::RunTest(const FString& Parameters)
{
	FStoryNextRoomPlan FirstTransitionPlan;
	TestTrue(TEXT("MoonlightRoom builds first currency transition plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::MoonlightRoom,
			FirstTransitionPlan));

	TestTrue(TEXT("First transition overrides reward options"), FirstTransitionPlan.bOverrideRewardOptions);
	TestEqual(TEXT("First transition has one reward option"), FirstTransitionPlan.RewardOptionsOverride.Num(), 1);
	if (FirstTransitionPlan.RewardOptionsOverride.Num() == 1)
	{
		TestNotEqual(TEXT("First transition reward is not a rune"),
			FirstTransitionPlan.RewardOptionsOverride[0].LootType,
			ELootType::Rune);
	}

	FStoryNextRoomPlan SecondTransitionPlan;
	TestTrue(TEXT("TransitionRoom01 builds second currency transition plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::TransitionRoom01,
			SecondTransitionPlan));

	TestTrue(TEXT("Second transition overrides reward options"), SecondTransitionPlan.bOverrideRewardOptions);
	TestEqual(TEXT("Second transition has one reward option"), SecondTransitionPlan.RewardOptionsOverride.Num(), 1);
	if (SecondTransitionPlan.RewardOptionsOverride.Num() == 1)
	{
		TestNotEqual(TEXT("Second transition reward is not a rune"),
			SecondTransitionPlan.RewardOptionsOverride[0].LootType,
			ELootType::Rune);
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
