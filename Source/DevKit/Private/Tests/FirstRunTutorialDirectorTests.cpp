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
	TestTrue(TEXT("Second room overrides reward options to Moonlight"), Plan.bOverrideRewardOptions);
	TestEqual(TEXT("Second room has one fixed reward override"), Plan.RewardOptionsOverride.Num(), 1);
	if (Plan.RewardOptionsOverride.Num() == 1)
	{
		TestEqual(TEXT("Second room reward is a rune"), Plan.RewardOptionsOverride[0].LootType, ELootType::Rune);
		TestNotNull(TEXT("Second room Moonlight rune asset is loaded"), Plan.RewardOptionsOverride[0].RuneAsset.Get());
		TestNotNull(TEXT("Second room uses an explicit card preview icon"), Plan.RewardOptionsOverride[0].Icon.Get());
		if (const URuneDataAsset* MoonlightRune = Plan.RewardOptionsOverride[0].RuneAsset.Get())
		{
			const FGameplayTag MoonlightIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Moonlight"), false);
			TestTrue(TEXT("Second room reward resolves to a Moonlight card"),
				MoonlightRune->RuneInfo.CombatCard.CardIdTag == MoonlightIdTag
				|| MoonlightRune->GetPathName().Contains(TEXT("Moonlight")));
		}
	}
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

	FStoryNextRoomPlan DefaultRewardPlan;
	TestTrue(TEXT("MoonlightRoom builds fourth-room default reward plan"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::MoonlightRoom,
			DefaultRewardPlan));

	TestTrue(TEXT("Fourth room forces a single portal"), DefaultRewardPlan.bForceSinglePortal);
	TestFalse(TEXT("Fourth room uses the room/default reward table"), DefaultRewardPlan.bOverrideRewardOptions);
	TestEqual(TEXT("Fourth room has no fixed reward option"), DefaultRewardPlan.RewardOptionsOverride.Num(), 0);
	TestFalse(TEXT("Fourth room does not carry a tutorial enemy attack buff"), DefaultRewardPlan.bOverrideBuffs);

	FStoryNextRoomPlan TransitionRoom01Plan;
	TestTrue(TEXT("TransitionRoom01 plan still produced (WaterDungeon → 01a/01b via portal[0] pool)"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::TransitionRoom01,
			TransitionRoom01Plan));

	TestTrue(TEXT("TransitionRoom01 keeps forced single portal"), TransitionRoom01Plan.bForceSinglePortal);
	TestEqual(TEXT("TransitionRoom01 still uses portal[0] so the WaterDungeon → corridor route fires"),
		TransitionRoom01Plan.PortalIndex, 0);
	TestNull(TEXT("TransitionRoom01 should no longer hard-override the next room data"),
		TransitionRoom01Plan.RoomDataOverride.Get());

	FStoryNextRoomPlan TransitionRoom02Plan;
	TestTrue(TEXT("TransitionRoom02 plan produced (corridor 01a/01b → PrayerRoom via portal[1])"),
		UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(
			EFirstRunTutorialStage::TransitionRoom02,
			TransitionRoom02Plan));

	TestTrue(TEXT("TransitionRoom02 forces a single portal"), TransitionRoom02Plan.bForceSinglePortal);
	TestEqual(TEXT("TransitionRoom02 forces portal[1] so the prayer-room exit is taken"),
		TransitionRoom02Plan.PortalIndex, 1);
	TestNotNull(
		TEXT("TransitionRoom02 pins RoomDataOverride to DA_PrayRoom so SelectRoomByTag's type roll can't drop the only Event room from portal[1]"),
		TransitionRoom02Plan.RoomDataOverride.Get());
	if (const URoomDataAsset* OverrideRoom = TransitionRoom02Plan.RoomDataOverride.Get())
	{
		TestTrue(TEXT("TransitionRoom02 RoomDataOverride is the PrayerRoom asset"),
			OverrideRoom->GetPathName().Contains(TEXT("DA_PrayRoom")));
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

	TestEqual(TEXT("Post tutorial deck has three non-finisher cards"), DeckAssets.Num(), 3);
	if (DeckAssets.Num() == 3)
	{
		TestNotNull(TEXT("Burn card exists"), DeckAssets[0]);
		TestNotNull(TEXT("Knockback card exists"), DeckAssets[1]);
		TestNotNull(TEXT("Moonlight card exists"), DeckAssets[2]);
		TestNotEqual(TEXT("Burn and knockback use separate card assets"), DeckAssets[0], DeckAssets[1]);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunTutorialDirectorPrayerSacrificeKeepsDefaultWhenFinisherDeprecatedTest,
	"DevKit.FirstRunTutorialDirector.PrayerSacrificeKeepsDefaultWhenFinisherDeprecated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstRunTutorialDirectorPrayerSacrificeKeepsDefaultWhenFinisherDeprecatedTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* MoonlightShadow = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_MoonlightShadow.DA_Rune512_Sacrifice_MoonlightShadow"));
	URuneDataAsset* Finisher = UFirstRunTutorialDirectorSubsystem::LoadFirstRunFinisherRune();

	TestNotNull(TEXT("Moonlight shadow sacrifice rune exists"), MoonlightShadow);
	TestNull(TEXT("First-run finisher rune is deprecated"), Finisher);
	if (!MoonlightShadow)
	{
		return false;
	}

	TestEqual(
		TEXT("Active prayer stage keeps altar reward while finisher is deprecated"),
		UFirstRunTutorialDirectorSubsystem::ResolveSacrificeRewardForStage(
			EFirstRunTutorialStage::PrayerRoom,
			true,
			MoonlightShadow),
		MoonlightShadow);

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
