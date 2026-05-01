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
#include "FlowAsset.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkConfigForwardTest,
	"DevKit.CombatDeck.LinkConfigForwardReadsPreviousAttackCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkConfigForwardTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.LinkConfig.Direction = ECombatCardLinkDirection::ForwardReadPrevious;
	MoonlightCard.LinkConfig.ForwardEffect.Flow = NewObject<UFlowAsset>();
	MoonlightCard.LinkConfig.ForwardEffect.Multiplier = 1.5f;
	MoonlightCard.LinkConfig.ForwardEffect.Condition.RequiredNeighborTypes.Add(ECombatCardType::Attack);

	Deck->SetDeckListForTest({ AttackCard, MoonlightCard });

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Attack card resolves before Moonlight"), AttackResult.bHadCard);

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Moonlight resolves as the second card"), MoonlightResult.bHadCard);
	TestTrue(TEXT("Moonlight forward link reads the previous attack card"), MoonlightResult.bTriggeredForwardLink);
	TestFalse(TEXT("Forward link does not also release the base flow"), MoonlightResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Forward link multiplier is reported"), MoonlightResult.AppliedMultiplier, 1.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkConfigBackwardTest,
	"DevKit.CombatDeck.LinkConfigBackwardEmpowersNextAttackCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkConfigBackwardTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.LinkConfig.Direction = ECombatCardLinkDirection::BackwardEmpowerNext;
	MoonlightCard.LinkConfig.BackwardEffect.Flow = NewObject<UFlowAsset>();
	MoonlightCard.LinkConfig.BackwardEffect.Multiplier = 1.25f;
	MoonlightCard.LinkConfig.BackwardEffect.Condition.RequiredNeighborTypes.Add(ECombatCardType::Attack);

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };

	Deck->SetDeckListForTest({ MoonlightCard, AttackCard });

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Moonlight is consumed first"), MoonlightResult.bHadCard);
	TestTrue(TEXT("Moonlight opens a pending backward link"), MoonlightResult.bPendingBackwardLink);

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Next attack card is consumed"), AttackResult.bHadCard);
	TestTrue(TEXT("Next attack triggers Moonlight backward link"), AttackResult.bTriggeredBackwardLink);
	TestTrue(TEXT("Next attack still releases its own base flow"), AttackResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Backward link multiplier is reported"), AttackResult.AppliedMultiplier, 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkConfigBothUsesForwardPriorityTest,
	"DevKit.CombatDeck.LinkConfigBothUsesForwardPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkConfigBothUsesForwardPriorityTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.LinkConfig.Direction = ECombatCardLinkDirection::Both;
	MoonlightCard.LinkConfig.ForwardEffect.Flow = NewObject<UFlowAsset>();
	MoonlightCard.LinkConfig.ForwardEffect.Condition.RequiredNeighborTypes.Add(ECombatCardType::Attack);
	MoonlightCard.LinkConfig.BackwardEffect.Flow = NewObject<UFlowAsset>();
	MoonlightCard.LinkConfig.BackwardEffect.Condition.RequiredNeighborTypes.Add(ECombatCardType::Attack);

	Deck->SetDeckListForTest({ AttackCard, MoonlightCard, AttackCard });

	Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Both-direction link triggers forward when previous attack matches"), MoonlightResult.bTriggeredForwardLink);
	TestFalse(TEXT("Forward match does not also open a backward pending link"), MoonlightResult.bPendingBackwardLink);

	const FCombatCardResolveResult FinalAttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestFalse(TEXT("Final attack does not receive a backward link from a forward-resolved Moonlight"), FinalAttackResult.bTriggeredBackwardLink);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightChainDefaultsWhenOnlyLinksTest,
	"DevKit.CombatDeck.MoonlightLinksDoNotChainIntoEachOtherByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightChainDefaultsWhenOnlyLinksTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.LinkConfig.Direction = ECombatCardLinkDirection::Both;
	MoonlightCard.LinkConfig.ForwardEffect.Flow = NewObject<UFlowAsset>();
	MoonlightCard.LinkConfig.BackwardEffect.Flow = NewObject<UFlowAsset>();

	Deck->SetDeckListForTest({ MoonlightCard, MoonlightCard, MoonlightCard, MoonlightCard });

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
		TestTrue(FString::Printf(TEXT("Moonlight %d is consumed"), Index + 1), Result.bHadCard);
		TestTrue(FString::Printf(TEXT("Moonlight %d releases base flow"), Index + 1), Result.bTriggeredBaseFlow);
		TestFalse(FString::Printf(TEXT("Moonlight %d does not forward-link to another Moonlight"), Index + 1), Result.bTriggeredForwardLink);
		TestFalse(FString::Printf(TEXT("Moonlight %d does not backward-link to another Moonlight"), Index + 1), Result.bTriggeredBackwardLink);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckNormalCardTriggersBaseFlowTest,
	"DevKit.CombatDeck.NormalCardTriggersBaseFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckNormalCardTriggersBaseFlowTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig NormalCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	NormalCard.BaseFlow = NewObject<UFlowAsset>();

	Deck->SetDeckListForTest({ NormalCard });

	const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Normal card is consumed"), Result.bHadCard);
	TestTrue(TEXT("Normal card triggers BaseFlow"), Result.bTriggeredBaseFlow);
	TestFalse(TEXT("Normal card does not trigger link"), Result.bTriggeredLink);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeForwardUsesEffectTagsTest,
	"DevKit.CombatDeck.RecipeForwardUsesEffectTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeForwardUsesEffectTagsTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Attack"));
	const FGameplayTag MoonlightIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Moonlight"));

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig AttackCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	AttackCard.CardEffectTags.AddTag(AttackEffectTag);

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.CardIdTag = MoonlightIdTag;
	MoonlightCard.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Forward;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Multiplier = 1.5f;
	Recipe.Condition.RequiredNeighborEffectTags.AddTag(AttackEffectTag);
	MoonlightCard.LinkRecipes.Add(Recipe);

	Deck->SetDeckListForTest({ AttackCard, MoonlightCard });

	Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);

	TestTrue(TEXT("Moonlight forward recipe is triggered by previous Card.Effect.Attack"), MoonlightResult.bTriggeredForwardLink);
	TestFalse(TEXT("Forward recipe does not release base flow"), MoonlightResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Forward recipe multiplier is reported"), MoonlightResult.AppliedMultiplier, 1.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedUsesEffectTagsTest,
	"DevKit.CombatDeck.RecipeReversedUsesEffectTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedUsesEffectTagsTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Attack"));

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.DefaultLinkOrientation = ECombatCardLinkOrientation::Reversed;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Reversed;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Multiplier = 1.25f;
	Recipe.Condition.RequiredNeighborEffectTags.AddTag(AttackEffectTag);
	MoonlightCard.LinkRecipes.Add(Recipe);

	FCombatCardConfig AttackCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	AttackCard.CardEffectTags.AddTag(AttackEffectTag);

	Deck->SetDeckListForTest({ MoonlightCard, AttackCard });

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link"), MoonlightResult.bPendingBackwardLink);

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Next Card.Effect.Attack triggers reversed Moonlight recipe"), AttackResult.bTriggeredBackwardLink);
	TestEqual(TEXT("Reversed recipe multiplier is reported"), AttackResult.AppliedMultiplier, 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedClearsOnComboExitTest,
	"DevKit.CombatDeck.RecipeReversedClearsOnComboExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedClearsOnComboExitTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Attack"));

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.DefaultLinkOrientation = ECombatCardLinkOrientation::Reversed;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Reversed;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Condition.RequiredNeighborEffectTags.AddTag(AttackEffectTag);
	MoonlightCard.LinkRecipes.Add(Recipe);

	FCombatCardConfig AttackCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	AttackCard.CardEffectTags.AddTag(AttackEffectTag);

	Deck->SetDeckListForTest({ MoonlightCard, AttackCard });

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link"), MoonlightResult.bPendingBackwardLink);

	Deck->NotifyComboStateExited();

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Next attack card is still consumed"), AttackResult.bHadCard);
	TestFalse(TEXT("Pending reversed link is cleared after combo exit"), AttackResult.bTriggeredBackwardLink);
	TestTrue(TEXT("Attack card falls back to its own base flow"), AttackResult.bTriggeredBaseFlow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckEditMovesAndReversesCardsTest,
	"DevKit.CombatDeck.EditMovesAndReversesCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckEditMovesAndReversesCardsTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig First{ ECombatCardType::Normal, ECardRequiredAction::Any };
	First.DisplayName = FText::FromString(TEXT("First"));

	FCombatCardConfig Moonlight{ ECombatCardType::Link, ECardRequiredAction::Any };
	Moonlight.DisplayName = FText::FromString(TEXT("Moonlight"));
	Moonlight.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;

	Deck->SetDeckListForTest({ First, Moonlight });

	TestTrue(TEXT("Moving Moonlight to the first slot succeeds"), Deck->MoveCardInDeck(1, 0));
	const TArray<FCombatCardInstance> MovedCards = Deck->GetFullDeckSnapshot();
	TestEqual(TEXT("Moonlight is now first"), MovedCards[0].Config.DisplayName.ToString(), FString(TEXT("Moonlight")));

	TestTrue(TEXT("Toggling link orientation succeeds"), Deck->ToggleCardLinkOrientationByIndex(0));
	const TArray<FCombatCardInstance> ReversedCards = Deck->GetFullDeckSnapshot();
	TestEqual(TEXT("Moonlight is reversed after toggle"), ReversedCards[0].LinkOrientation, ECombatCardLinkOrientation::Reversed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeDoesNotLinkToLinkCardsTest,
	"DevKit.CombatDeck.RecipeDoesNotLinkToLinkCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeDoesNotLinkToLinkCardsTest::RunTest(const FString& Parameters)
{
	const FGameplayTag MoonlightEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Moonlight"));

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig FirstLink{ ECombatCardType::Link, ECardRequiredAction::Any };
	FirstLink.CardEffectTags.AddTag(MoonlightEffectTag);

	FCombatCardConfig SecondLink{ ECombatCardType::Link, ECardRequiredAction::Any };
	SecondLink.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Forward;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Condition.RequiredNeighborEffectTags.AddTag(MoonlightEffectTag);
	SecondLink.LinkRecipes.Add(Recipe);

	Deck->SetDeckListForTest({ FirstLink, SecondLink });

	Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);

	TestTrue(TEXT("Second link card is consumed"), Result.bHadCard);
	TestFalse(TEXT("Recipe does not trigger against another link card"), Result.bTriggeredForwardLink);
	TestTrue(TEXT("Second link card falls back to base flow"), Result.bTriggeredBaseFlow);

	return true;
}

#endif
