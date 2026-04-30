#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Component/CombatDeckComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h"
#include "Data/AbilityData.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/WeaponComboConfigDA.h"
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
	TestTrue(TEXT("Normal attack cards ignore Light/Heavy mismatches"), Result.bActionMatched);
	TestTrue(TEXT("Mismatched action triggers base flow"), Result.bTriggeredBaseFlow);
	TestTrue(TEXT("Normal attack cards count as released"), Result.bTriggeredMatchedFlow);

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
	TestFalse(TEXT("Normal attack does not trigger finisher matched flow"), NormalAttack.bTriggeredMatchedFlow);
	TestFalse(TEXT("Normal attack does not trigger finisher bonus"), NormalAttack.bTriggeredFinisher);

	Deck->AdvanceShuffleForTest(Deck->GetShuffleCooldownDuration());

	const FCombatCardResolveResult ComboFinisher = Deck->ResolveAttackCard(ECardRequiredAction::Light, true, false);
	TestTrue(TEXT("Combo finisher consumes the finisher card"), ComboFinisher.bHadCard);
	TestTrue(TEXT("Combo finisher triggers finisher matched flow"), ComboFinisher.bTriggeredMatchedFlow);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMontageConfigSelectionTest,
	"DevKit.CombatDeck.MontageConfigSelectsByTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMontageConfigSelectionTest::RunTest(const FString& Parameters)
{
	UAbilityData* AbilityData = NewObject<UAbilityData>();
	UMontageConfigDA* DefaultConfig = NewObject<UMontageConfigDA>();
	UMontageConfigDA* BranchConfig = NewObject<UMontageConfigDA>();

	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo3"));
	const FGameplayTag BranchTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"));

	FTaggedMontageConfig DefaultCandidate;
	DefaultCandidate.Priority = 0;
	DefaultCandidate.MontageConfig = DefaultConfig;

	FTaggedMontageConfig BranchCandidate;
	BranchCandidate.Priority = 10;
	BranchCandidate.RequiredTags.AddTag(BranchTag);
	BranchCandidate.MontageConfig = BranchConfig;

	FAbilityMontageConfigList ConfigList;
	ConfigList.Configs = { DefaultCandidate, BranchCandidate };
	AbilityData->MontageConfigMap.Add(AbilityTag, ConfigList);

	TestEqual(TEXT("Default config is selected without branch tags"),
		AbilityData->GetMontageConfig(AbilityTag, FGameplayTagContainer()), DefaultConfig);

	FGameplayTagContainer BranchContext;
	BranchContext.AddTag(BranchTag);
	TestEqual(TEXT("Higher-priority matching branch config is selected"),
		AbilityData->GetMontageConfig(AbilityTag, BranchContext), BranchConfig);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMontageAttackDataSelectionTest,
	"DevKit.CombatDeck.MontageHitWindowSelectsAttackDataByTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMontageAttackDataSelectionTest::RunTest(const FString& Parameters)
{
	UMontageConfigDA* MontageConfig = NewObject<UMontageConfigDA>();
	UMNE_HitWindow* HitWindow = NewObject<UMNE_HitWindow>(MontageConfig);
	MontageConfig->Entries.Add(HitWindow);

	UMontageAttackDataAsset* DefaultAttackData = NewObject<UMontageAttackDataAsset>();
	DefaultAttackData->ActDamage = 10.f;
	UMontageAttackDataAsset* BranchAttackData = NewObject<UMontageAttackDataAsset>();
	BranchAttackData->ActDamage = 30.f;

	const FGameplayTag BranchTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"));

	FTaggedMontageAttackData DefaultCandidate;
	DefaultCandidate.Priority = 0;
	DefaultCandidate.AttackData = DefaultAttackData;

	FTaggedMontageAttackData BranchCandidate;
	BranchCandidate.Priority = 10;
	BranchCandidate.RequiredTags.AddTag(BranchTag);
	BranchCandidate.AttackData = BranchAttackData;

	HitWindow->AttackDataCandidates = { DefaultCandidate, BranchCandidate };

	TestEqual(TEXT("Default attack data is selected without branch tags"),
		MontageConfig->ResolveAttackData(FGameplayTagContainer()), DefaultAttackData);

	FGameplayTagContainer BranchContext;
	BranchContext.AddTag(BranchTag);
	TestEqual(TEXT("Branch attack data is selected by tags"),
		MontageConfig->ResolveAttackData(BranchContext), BranchAttackData);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponComboConfigRoutesMixedFinisherTest,
	"DevKit.CombatDeck.ComboConfigRoutesLightLightHeavyFinisher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponComboConfigRoutesMixedFinisherTest::RunTest(const FString& Parameters)
{
	UWeaponComboConfigDA* ComboConfig = NewObject<UWeaponComboConfigDA>();

	FWeaponComboNodeConfig L1;
	L1.NodeId = TEXT("L1");
	L1.InputAction = ECardRequiredAction::Light;

	FWeaponComboNodeConfig L2;
	L2.NodeId = TEXT("L2");
	L2.ParentNodeId = TEXT("L1");
	L2.InputAction = ECardRequiredAction::Light;

	FWeaponComboNodeConfig L2H;
	L2H.NodeId = TEXT("L2H");
	L2H.ParentNodeId = TEXT("L2");
	L2H.InputAction = ECardRequiredAction::Heavy;
	L2H.bIsComboFinisher = true;

	ComboConfig->RootNodes = { TEXT("L1") };
	ComboConfig->Nodes = { L1, L2, L2H };

	const FWeaponComboNodeConfig* Root = ComboConfig->FindRootNode(ECardRequiredAction::Light);
	TestNotNull(TEXT("Light input selects L1 root"), Root);
	TestEqual(TEXT("Root node is L1"), Root ? Root->NodeId : NAME_None, FName(TEXT("L1")));

	const FWeaponComboNodeConfig* Second = ComboConfig->FindChildNode(TEXT("L1"), ECardRequiredAction::Light);
	TestNotNull(TEXT("Second light selects L2"), Second);
	TestEqual(TEXT("Second node is L2"), Second ? Second->NodeId : NAME_None, FName(TEXT("L2")));

	const FWeaponComboNodeConfig* Finisher = ComboConfig->FindChildNode(TEXT("L2"), ECardRequiredAction::Heavy);
	TestNotNull(TEXT("Heavy after L-L selects independent finisher node"), Finisher);
	TestEqual(TEXT("Finisher node is L2H"), Finisher ? Finisher->NodeId : NAME_None, FName(TEXT("L2H")));
	TestTrue(TEXT("L-L-H node is marked as finisher"), Finisher && Finisher->bIsComboFinisher);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckContextDedupesCommitAndHitTest,
	"DevKit.CombatDeck.ContextDedupesCommitAndHitForSameAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckContextDedupesCommitAndHitTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
	});

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Light;
	Context.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	Context.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult CommitResult = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("OnCommit consumes the first card"), CommitResult.bHadCard);

	Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	const FCombatCardResolveResult HitResult = Deck->ResolveAttackCardWithContext(Context);
	TestFalse(TEXT("OnHit for the same attack guid does not consume again"), HitResult.bHadCard);
	TestEqual(TEXT("Only one card was consumed"), Deck->GetRemainingDeckSnapshot().Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckOnHitCardIgnoresCommitTest,
	"DevKit.CombatDeck.OnHitCardConsumesOnlyOnHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckOnHitCardIgnoresCommitTest::RunTest(const FString& Parameters)
{
	FCombatCardConfig OnHitCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	OnHitCard.TriggerTiming = ECombatCardTriggerTiming::OnHit;

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({ OnHitCard });

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Light;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	Context.TriggerTiming = ECombatCardTriggerTiming::OnCommit;

	const FCombatCardResolveResult CommitResult = Deck->ResolveAttackCardWithContext(Context);
	TestFalse(TEXT("OnHit card is not consumed by OnCommit"), CommitResult.bHadCard);
	TestEqual(TEXT("Card remains available after ignored OnCommit"), Deck->GetRemainingDeckSnapshot().Num(), 1);

	Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	const FCombatCardResolveResult HitResult = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("OnHit card is consumed by OnHit"), HitResult.bHadCard);
	TestTrue(TEXT("OnHit card triggers base release"), HitResult.bTriggeredBaseFlow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkReadPreviousTest,
	"DevKit.CombatDeck.LinkReadPreviousUsesLastResolvedCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkReadPreviousTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig First{ ECombatCardType::Attack, ECardRequiredAction::Light };
	FCombatCardConfig ReadPrevious{ ECombatCardType::Link, ECardRequiredAction::Light };
	ReadPrevious.LinkMode = ECardLinkMode::ReadPrevious;

	Deck->SetDeckListForTest({ First, ReadPrevious });

	const FCombatCardResolveResult FirstResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("First card resolves normally"), FirstResult.bHadCard);

	const FCombatCardResolveResult LinkResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("ReadPrevious sees the previous resolved card"), LinkResult.bTriggeredLink);

	return true;
}

#endif
