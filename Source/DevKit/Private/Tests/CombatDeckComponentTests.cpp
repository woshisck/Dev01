#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Component/CombatDeckComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckConsumesFourCardsTest,
	"DevKit.CombatDeck.ConsumesFourCardsThenShuffles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckConsumesFourCardsTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetShuffleCooldownDuration(1.0f);
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Heavy },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
	});

	for (int32 i = 0; i < 3; ++i)
	{
		const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
		TestTrue(TEXT("Card is consumed before the deck is empty"), Result.bHadCard);
		TestFalse(TEXT("Deck should not shuffle before the final card"), Result.bStartedShuffle);
	}

	const FCombatCardResolveResult FinalResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Fourth attack consumes the fourth card"), FinalResult.bHadCard);
	TestTrue(TEXT("Fourth consumed card starts shuffle"), FinalResult.bStartedShuffle);
	TestEqual(TEXT("Deck enters shuffling state"), Deck->GetDeckState(), EDeckState::EmptyShuffling);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckShuffleWindowTest,
	"DevKit.CombatDeck.ShuffleWindowReturnsNoCardThenRefills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckShuffleWindowTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetShuffleCooldownDuration(1.0f);
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
	});

	Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);

	const FCombatCardResolveResult DuringShuffle = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestFalse(TEXT("Attacks during shuffle do not trigger cards"), DuringShuffle.bHadCard);

	Deck->AdvanceShuffleForTest(1.0f);

	TestEqual(TEXT("Deck returns to ready after cooldown"), Deck->GetDeckState(), EDeckState::Ready);
	TestEqual(TEXT("Deck refills active sequence in fixed order"), Deck->GetDeckSnapshot().Num(), 1);

	const FCombatCardResolveResult AfterRefill = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Next attack after refill can trigger a card"), AfterRefill.bHadCard);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckActionMismatchTest,
	"DevKit.CombatDeck.ActionMismatchConsumesOnlyBaseFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckActionMismatchTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
	});

	const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Heavy, false, false);

	TestTrue(TEXT("Mismatched action still consumes a card"), Result.bHadCard);
	TestFalse(TEXT("Mismatched action does not count as matched"), Result.bActionMatched);
	TestTrue(TEXT("Mismatched action triggers base flow"), Result.bTriggeredBaseFlow);
	TestFalse(TEXT("Mismatched action does not trigger matched flow"), Result.bTriggeredMatchedFlow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckFinisherRequirementTest,
	"DevKit.CombatDeck.FinisherRequiresComboFinisher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckFinisherRequirementTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	FCombatCardConfig FinisherCard{ ECombatCardType::Finisher, ECardRequiredAction::Any };
	FinisherCard.bRequiresComboFinisher = true;
	Deck->SetDeckListForTest({ FinisherCard });

	const FCombatCardResolveResult NormalAttack = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Normal attack consumes the finisher card"), NormalAttack.bHadCard);
	TestFalse(TEXT("Normal attack does not trigger finisher bonus"), NormalAttack.bTriggeredFinisher);

	Deck->AdvanceShuffleForTest(Deck->GetShuffleCooldownDuration());

	const FCombatCardResolveResult ComboFinisher = Deck->ResolveAttackCard(ECardRequiredAction::Light, true, false);
	TestTrue(TEXT("Combo finisher consumes the finisher card"), ComboFinisher.bHadCard);
	TestTrue(TEXT("Combo finisher triggers finisher bonus"), ComboFinisher.bTriggeredFinisher);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckSourceRestoreTest,
	"DevKit.CombatDeck.SourceAssetsCanRestoreDeckOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckSourceRestoreTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* FirstRune = NewObject<URuneDataAsset>();
	FirstRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light };

	URuneDataAsset* SecondRune = NewObject<URuneDataAsset>();
	SecondRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Heavy };

	UCombatDeckComponent* SourceDeck = NewObject<UCombatDeckComponent>();
	SourceDeck->AddCardFromRuneReward(FirstRune);
	SourceDeck->AddCardFromRuneReward(SecondRune);

	TArray<URuneDataAsset*> SavedAssets = SourceDeck->GetDeckSourceAssets();

	UCombatDeckComponent* RestoredDeck = NewObject<UCombatDeckComponent>();
	RestoredDeck->LoadDeckFromSourceAssets(SavedAssets, 1.0f, 0);

	TestEqual(TEXT("Restored active sequence keeps source asset order"), RestoredDeck->GetDeckSnapshot().Num(), 2);

	const FCombatCardResolveResult FirstResult = RestoredDeck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("First restored card matches Light"), FirstResult.bActionMatched);

	const FCombatCardResolveResult SecondResult = RestoredDeck->ResolveAttackCard(ECardRequiredAction::Heavy, false, false);
	TestTrue(TEXT("Second restored card matches Heavy"), SecondResult.bActionMatched);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRemainingSnapshotTest,
	"DevKit.CombatDeck.RemainingSnapshotDropsConsumedCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRemainingSnapshotTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Heavy },
	});

	TestEqual(TEXT("Remaining snapshot starts with every active card"), Deck->GetRemainingDeckSnapshot().Num(), 2);

	Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);

	const TArray<FCombatCardInstance> RemainingSnapshot = Deck->GetRemainingDeckSnapshot();
	TestEqual(TEXT("Consumed cards are removed from remaining snapshot"), RemainingSnapshot.Num(), 1);
	if (RemainingSnapshot.IsValidIndex(0))
	{
		TestEqual(TEXT("Next remaining card keeps fixed order"), RemainingSnapshot[0].Config.RequiredAction, ECardRequiredAction::Heavy);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckDashSaveConsumeResultTest,
	"DevKit.CombatDeck.DashSaveConsumeReportsWhetherTagsWereConsumed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckDashSaveConsumeResultTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for DashSave ASC test"), World);
	if (!World)
	{
		return false;
	}

	AActor* OwnerActor = World->SpawnActor<AActor>();
	UYogAbilitySystemComponent* ASC = NewObject<UYogAbilitySystemComponent>(OwnerActor);
	OwnerActor->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(OwnerActor, OwnerActor);

	TestFalse(TEXT("Consuming without saved combo tags reports false"), ASC->ConsumeDashSave());

	const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
	FGameplayTagContainer SavedTags;
	SavedTags.AddTag(CanComboTag);

	ASC->ApplyDashSave(SavedTags);
	TestEqual(TEXT("DashSave tag is applied before consumption"), ASC->GetTagCount(CanComboTag), 1);

	TestTrue(TEXT("Consuming saved combo tags reports true"), ASC->ConsumeDashSave());
	TestEqual(TEXT("DashSave tag is removed after consumption"), ASC->GetTagCount(CanComboTag), 0);
	TestFalse(TEXT("Consuming again after cleanup reports false"), ASC->ConsumeDashSave());

	OwnerActor->Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMeleeActionMappingTest,
	"DevKit.CombatDeck.MeleeAbilitiesMapToCombatDeckActionContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMeleeActionMappingTest::RunTest(const FString& Parameters)
{
	UGA_Player_LightAtk1* LightAttack = NewObject<UGA_Player_LightAtk1>();
	TestEqual(TEXT("Light attack maps to Light card action"), LightAttack->GetCombatDeckActionType(), ECardRequiredAction::Light);
	TestFalse(TEXT("Light attack 1 is not a combo finisher"), LightAttack->IsCombatDeckComboFinisher());

	UGA_Player_HeavyAtk1* HeavyAttack = NewObject<UGA_Player_HeavyAtk1>();
	TestEqual(TEXT("Heavy attack maps to Heavy card action"), HeavyAttack->GetCombatDeckActionType(), ECardRequiredAction::Heavy);
	TestFalse(TEXT("Heavy attack 1 is not a combo finisher"), HeavyAttack->IsCombatDeckComboFinisher());

	UGA_Player_LightAtk4* LightFinisher = NewObject<UGA_Player_LightAtk4>();
	TestEqual(TEXT("Light combo finisher remains a Light card action"), LightFinisher->GetCombatDeckActionType(), ECardRequiredAction::Light);
	TestTrue(TEXT("Light combo 4 is a combo finisher"), LightFinisher->IsCombatDeckComboFinisher());

	UGA_Player_HeavyAtk4* HeavyFinisher = NewObject<UGA_Player_HeavyAtk4>();
	TestEqual(TEXT("Heavy combo finisher remains a Heavy card action"), HeavyFinisher->GetCombatDeckActionType(), ECardRequiredAction::Heavy);
	TestTrue(TEXT("Heavy combo 4 is a combo finisher"), HeavyFinisher->IsCombatDeckComboFinisher());

	return true;
}

#endif
