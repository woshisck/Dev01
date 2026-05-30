#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "Story/FirstRunTutorialDirectorSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorBuildsPostInitialRoomPlanTest,
	"DevKit.FirstRunTutorialDirector.BuildsPostInitialRoomPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorBuildsPostInitialRoomPlanTest::RunTest(const FString& Parameters)
{
	FStoryNextRoomPlan Plan;
	TestTrue(TEXT("GoldRoomCleared builds the post-initial tutorial room plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::GoldRoomCleared,
			Plan));

	TestTrue(TEXT("Post-initial room plan forces a single portal"), Plan.bForceSinglePortal);
	TestEqual(TEXT("Default forced portal is index 0"), Plan.PortalIndex, 0);
	TestFalse(TEXT("Second room does not override reward options; first clear separately spawns gold plus the initial three-card pickup"),
		Plan.bOverrideRewardOptions);
	TestEqual(TEXT("Second room has no fixed reward override"), Plan.RewardOptionsOverride.Num(), 0);
	TestFalse(TEXT("Post-initial room does not use a special enemy reward"), Plan.bMarkLastEnemyAsSpecialRewardEnemy);

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
		TestEqual(TEXT("Material reward uses the HUD primary material currency"),
			MaterialPlan.RewardOptionsOverride[0].MetaCurrencyTag,
			FGameplayTag::RequestGameplayTag(TEXT("Currency.Meta.Common.A"), false));
	}
	TestFalse(TEXT("Material room does not use a special enemy reward"), MaterialPlan.bMarkLastEnemyAsSpecialRewardEnemy);

	FStoryNextRoomPlan FixedRunePlan;
	TestTrue(TEXT("MoonlightRoom builds fourth-room fixed rune plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::MoonlightRoom,
			FixedRunePlan));

	TestTrue(TEXT("Fixed rune room forces a single portal"), FixedRunePlan.bForceSinglePortal);
	TestTrue(TEXT("Fixed moonlight room overrides reward options"), FixedRunePlan.bOverrideRewardOptions);
	TestEqual(TEXT("Fixed moonlight room has one reward option"), FixedRunePlan.RewardOptionsOverride.Num(), 1);
	for (const FLootOption& Option : FixedRunePlan.RewardOptionsOverride)
	{
		TestEqual(TEXT("Fixed moonlight room option is a rune"), Option.LootType, ELootType::Rune);
		TestNotNull(TEXT("Fixed moonlight room rune asset is loaded"), Option.RuneAsset.Get());
		TestNotNull(TEXT("Fixed moonlight room uses an explicit card preview icon"), Option.Icon.Get());
	}
	TestTrue(TEXT("Fixed moonlight room carries the enemy attack buff"), FixedRunePlan.bOverrideBuffs);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorPrayerSacrificeOverridesToFinisherTest,
	"DevKit.FirstRunTutorialDirector.PrayerSacrificeOverridesToFinisher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorPrayerSacrificeOverridesToFinisherTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* MoonlightShadow = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_MoonlightShadow.DA_Rune512_Sacrifice_MoonlightShadow"));
	URuneDataAsset* Finisher = UFirstRunTutorialDirectorSubsystem::LoadFirstRunFinisherRune();

	TestNotNull(TEXT("Moonlight shadow sacrifice rune exists"), MoonlightShadow);
	TestNotNull(TEXT("First-run finisher rune exists"), Finisher);
	if (!MoonlightShadow || !Finisher)
	{
		return false;
	}

	TestEqual(
		TEXT("Active prayer stage overrides altar reward to finisher"),
		UFirstRunTutorialDirectorSubsystem::ResolveSacrificeRewardForStage(
			EFirstRunTutorialStage::PrayerRoom,
			true,
			MoonlightShadow),
		Finisher);

	TestEqual(
		TEXT("Inactive tutorial keeps altar reward"),
		UFirstRunTutorialDirectorSubsystem::ResolveSacrificeRewardForStage(
			EFirstRunTutorialStage::PrayerRoom,
			false,
			MoonlightShadow),
		MoonlightShadow);

	TestEqual(
		TEXT("Other tutorial stages keep altar reward"),
		UFirstRunTutorialDirectorSubsystem::ResolveSacrificeRewardForStage(
			EFirstRunTutorialStage::MoonlightRoom,
			true,
			MoonlightShadow),
		MoonlightShadow);

	return true;
}

#endif
