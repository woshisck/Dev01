#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Component/CombatDeckComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h"
#include "AbilitySystem/GameplayEffect/GE_RuneBurn.h"
#include "AbilitySystem/Execution/GEExec_BurnDamage.h"
#include "AbilitySystem/Execution/GEExec_PoisonDamage.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_ApplyGEInRadius.h"
#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"
#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Data/AbilityData.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"
#include "Data/WeaponComboConfigDA.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictHitReactBlocksMovementControlTest,
	"DevKit.CombatDeck.HitReactBlocksMovementControlWithoutConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictHitReactBlocksMovementControlTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for HitReact movement test"), World);
	if (!World)
	{
		return false;
	}

	AYogCharacterBase* Character = World->SpawnActor<AYogCharacterBase>();
	TestNotNull(TEXT("Character spawned for HitReact movement test"), Character);
	if (!Character)
	{
		return false;
	}

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	TestNotNull(TEXT("Character has Yog ASC"), ASC);
	if (!ASC)
	{
		Character->Destroy();
		return false;
	}

	ASC->InitAbilityActorInfo(Character, Character);
	Character->bMovable = true;
	if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"));
	ASC->AddLooseGameplayTag(HitReactTag);

	TestFalse(TEXT("HitReact blocks player/enemy movement control even without StateConflict data config"), Character->bMovable);
	if (const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		TestTrue(TEXT("HitReact keeps movement mode available for montage root motion"), MoveComp->MovementMode != MOVE_None);
	}

	Character->Destroy();

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphBuildsRuntimeWindowTest,
	"DevKit.CombatDeck.ComboGraphBuildsRuntimeWindowConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphBuildsRuntimeWindowTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraphNode* Node = NewObject<UGameplayAbilityComboGraphNode>();
	Node->NodeId = TEXT("L2H");
	Node->GameplayAbilityClass = UGA_Player_LightAtk2::StaticClass();
	Node->AbilityTagOverride = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"));
	Node->bUseNodeComboWindow = true;
	Node->ComboWindowStartFrame = 12;
	Node->ComboWindowEndFrame = 20;
	Node->ComboWindowTotalFrames = 30;

	const FWeaponComboNodeConfig RuntimeConfig = Node->BuildRuntimeConfig(ECombatGraphInputAction::Heavy);

	TestEqual(TEXT("Graph node exports its NodeId"), RuntimeConfig.NodeId, FName(TEXT("L2H")));
	TestEqual(TEXT("Graph edge input becomes runtime input"), RuntimeConfig.InputAction, ECardRequiredAction::Heavy);
	TestEqual(TEXT("Graph node exports gameplay ability class fallback"), RuntimeConfig.GameplayAbilityClass, Node->GameplayAbilityClass);
	TestFalse(TEXT("Graph node frame window is ignored by runtime while montage notifies drive combo windows"), RuntimeConfig.bOverrideComboWindow);
	TestEqual(TEXT("Combo window start frame is exported"), RuntimeConfig.ComboWindowStartFrame, 12);
	TestEqual(TEXT("Combo window end frame is exported"), RuntimeConfig.ComboWindowEndFrame, 20);
	TestEqual(TEXT("Combo window total frames is exported"), RuntimeConfig.ComboWindowTotalFrames, 30);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphDashNodeExportsRuntimeConfigTest,
	"DevKit.CombatDeck.ComboGraphDashNodeExportsRuntimeConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphDashNodeExportsRuntimeConfigTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>();
	UGameplayAbilityComboGraphNode* DashNode = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	DashNode->Graph = Graph;
	DashNode->NodeId = TEXT("Dash");
	DashNode->RootInputAction = ECombatGraphInputAction::Dash;
	DashNode->AbilityTagOverride = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash"));
	DashNode->DashSaveMode = EComboDashSaveMode::ForcePreserve;
	DashNode->DashSaveExpireSeconds = 0.75f;
	DashNode->bSavePendingLinkContext = false;
	DashNode->bClearCombatTagsOnDashEnd = true;
	DashNode->bBreakComboOnDashCancel = false;
	Graph->AllNodes = { DashNode };
	Graph->RootNodes = { DashNode };

	TArray<FText> Warnings;
	Graph->ValidateComboGraph(Warnings);
	const bool bHasMontageWarning = Warnings.ContainsByPredicate([](const FText& Warning)
	{
		return Warning.ToString().Contains(TEXT("has no MontageConfig"));
	});
	TestFalse(TEXT("Dash graph node does not require MontageConfig"), bHasMontageWarning);

	const FWeaponComboNodeConfig RuntimeConfig = DashNode->BuildRuntimeConfig(ECombatGraphInputAction::Dash);
	TestEqual(TEXT("Dash node maps to card-neutral runtime action"), RuntimeConfig.InputAction, ECardRequiredAction::Any);
	TestEqual(TEXT("Dash node exports save mode"), RuntimeConfig.DashSaveMode, EComboDashSaveMode::ForcePreserve);
	TestEqual(TEXT("Dash node exports save expiry"), RuntimeConfig.DashSaveExpireSeconds, 0.75f);
	TestFalse(TEXT("Dash node can disable pending link save"), RuntimeConfig.bSavePendingLinkContext);
	TestTrue(TEXT("Dash node exports combat tag cleanup"), RuntimeConfig.bClearCombatTagsOnDashEnd);
	TestFalse(TEXT("Dash node can keep save on cancel"), RuntimeConfig.bBreakComboOnDashCancel);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphWarnsDuplicateChildInputTest,
	"DevKit.CombatDeck.ComboGraphWarnsDuplicateChildInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphWarnsDuplicateChildInputTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>();
	UGameplayAbilityComboGraphNode* Root = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	UGameplayAbilityComboGraphNode* FirstChild = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	UGameplayAbilityComboGraphNode* SecondChild = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	UGameplayAbilityComboGraphEdge* FirstEdge = NewObject<UGameplayAbilityComboGraphEdge>(Graph);
	UGameplayAbilityComboGraphEdge* SecondEdge = NewObject<UGameplayAbilityComboGraphEdge>(Graph);

	Root->Graph = Graph;
	Root->NodeId = TEXT("Root");
	Root->AbilityTagOverride = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo1"));
	Root->MontageConfig = NewObject<UMontageConfigDA>(Graph);

	FirstChild->Graph = Graph;
	FirstChild->NodeId = TEXT("LightA");
	FirstChild->AbilityTagOverride = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo2"));
	FirstChild->MontageConfig = NewObject<UMontageConfigDA>(Graph);

	SecondChild->Graph = Graph;
	SecondChild->NodeId = TEXT("LightB");
	SecondChild->AbilityTagOverride = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo3"));
	SecondChild->MontageConfig = NewObject<UMontageConfigDA>(Graph);

	FirstEdge->InputAction = ECombatGraphInputAction::Light;
	SecondEdge->InputAction = ECombatGraphInputAction::Light;
	Root->ChildrenNodes = { FirstChild, SecondChild };
	FirstChild->ParentNodes = { Root };
	SecondChild->ParentNodes = { Root };
	Root->Edges.Add(FirstChild, FirstEdge);
	Root->Edges.Add(SecondChild, SecondEdge);
	Graph->AllNodes = { Root, FirstChild, SecondChild };
	Graph->RootNodes = { Root };

	TArray<FText> Warnings;
	Graph->ValidateComboGraph(Warnings);

	const bool bHasDuplicateInputWarning = Warnings.ContainsByPredicate([](const FText& Warning)
	{
		return Warning.ToString().Contains(TEXT("multiple children for input"));
	});
	TestTrue(TEXT("Graph validation reports duplicate child input under the same parent"), bHasDuplicateInputWarning);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckComboEffectScalingDefaultsOffTest,
	"DevKit.CombatDeck.ComboEffectScalingDefaultsOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckComboEffectScalingDefaultsOffTest::RunTest(const FString& Parameters)
{
	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({ AttackCard });

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Light;
	Context.ComboIndex = 3;
	Context.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult Result = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("Default card consumes"), Result.bHadCard);
	TestEqual(TEXT("Default card does not receive combo scaling"), Result.AppliedMultiplier, 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckComboEffectScalingAppliesAndCapsTest,
	"DevKit.CombatDeck.ComboEffectScalingAppliesAndCaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckComboEffectScalingAppliesAndCapsTest::RunTest(const FString& Parameters)
{
	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	AttackCard.bUseComboEffectScaling = true;
	AttackCard.ComboScalarPerIndex = 0.25f;
	AttackCard.MaxComboScalar = 0.5f;

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({ AttackCard, AttackCard, AttackCard });

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Light;

	Context.ComboIndex = 1;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult FirstResult = Deck->ResolveAttackCardWithContext(Context);
	TestEqual(TEXT("Combo1 multiplier is unchanged"), FirstResult.AppliedMultiplier, 1.0f);

	Context.ComboIndex = 2;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult SecondResult = Deck->ResolveAttackCardWithContext(Context);
	TestEqual(TEXT("Combo2 receives one stack"), SecondResult.AppliedMultiplier, 1.25f);

	Context.ComboIndex = 4;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult CappedResult = Deck->ResolveAttackCardWithContext(Context);
	TestEqual(TEXT("Combo scaling respects max scalar"), CappedResult.AppliedMultiplier, 1.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkMultiplierCombinesWithComboScalingTest,
	"DevKit.CombatDeck.LinkMultiplierCombinesWithComboScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkMultiplierCombinesWithComboScalingTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Attack"));

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	AttackCard.CardEffectTags.AddTag(AttackEffectTag);

	FCombatCardConfig LinkCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	LinkCard.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;
	LinkCard.bUseComboEffectScaling = true;
	LinkCard.ComboScalarPerIndex = 0.25f;
	LinkCard.MaxComboScalar = 0.5f;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Forward;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Multiplier = 1.5f;
	Recipe.Condition.RequiredNeighborEffectTags.AddTag(AttackEffectTag);
	LinkCard.LinkRecipes.Add(Recipe);

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({ AttackCard, LinkCard });

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Light;
	Context.ComboIndex = 1;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	Deck->ResolveAttackCardWithContext(Context);

	Context.ComboIndex = 3;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult LinkResult = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("Forward link triggers"), LinkResult.bTriggeredForwardLink);
	TestEqual(TEXT("Link multiplier combines with combo multiplier"), LinkResult.AppliedMultiplier, 2.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffFlowCombatCardEffectContextStoresResolveDataTest,
	"DevKit.CombatDeck.BuffFlowStoresCombatCardEffectContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuffFlowCombatCardEffectContextStoresResolveDataTest::RunTest(const FString& Parameters)
{
	FCombatCardConfig AttackCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
	FCombatCardInstance AttackCard;
	AttackCard.InstanceGuid = FGuid::NewGuid();
	AttackCard.Config = AttackCardConfig;

	FCombatDeckActionContext ActionContext;
	ActionContext.ComboIndex = 3;
	ActionContext.ComboNodeId = TEXT("L3");
	ActionContext.bIsComboFinisher = true;

	FCombatCardResolveResult ResolveResult;
	ResolveResult.bHadCard = true;
	ResolveResult.ConsumedCard = AttackCard;
	ResolveResult.AppliedMultiplier = 1.5f;

	UBuffFlowComponent* BuffFlowComponent = NewObject<UBuffFlowComponent>();
	BuffFlowComponent->StartCombatCardFlow(nullptr, AttackCard, ActionContext, ResolveResult, nullptr, true);

	TestTrue(TEXT("Combat-card context is marked available"), BuffFlowComponent->HasCombatCardEffectContext());
	const FCombatCardEffectContext& StoredContext = BuffFlowComponent->GetLastCombatCardEffectContext();
	TestEqual(TEXT("Stored combo index"), StoredContext.ComboIndex, 3);
	TestEqual(TEXT("Stored combo bonus stacks"), StoredContext.ComboBonusStacks, 2);
	TestEqual(TEXT("Stored multiplier"), StoredContext.EffectMultiplier, 1.5f);
	TestTrue(TEXT("Stored finisher flag"), StoredContext.bIsComboFinisher);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightMultipleRecipesTest,
	"DevKit.CombatDeck.MoonlightMultipleRecipesSelectByEffectTagAndDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightMultipleRecipesTest::RunTest(const FString& Parameters)
{
	const FGameplayTag BurnEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Burn"));
	const FGameplayTag PoisonEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Poison"));
	const FGameplayTag ShieldEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Shield"));

	UFlowAsset* BurnForwardFlow = NewObject<UFlowAsset>();
	UFlowAsset* PoisonForwardFlow = NewObject<UFlowAsset>();
	UFlowAsset* ShieldReversedFlow = NewObject<UFlowAsset>();

	FCombatCardConfig BurnCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	BurnCard.CardEffectTags.AddTag(BurnEffectTag);

	FCombatCardConfig PoisonCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	PoisonCard.CardEffectTags.AddTag(PoisonEffectTag);

	FCombatCardConfig ShieldCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	ShieldCard.CardEffectTags.AddTag(ShieldEffectTag);

	FCombatCardConfig MoonlightForward{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightForward.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;

	FCombatCardLinkRecipe BurnForwardRecipe;
	BurnForwardRecipe.Direction = ECombatCardLinkOrientation::Forward;
	BurnForwardRecipe.LinkFlow = BurnForwardFlow;
	BurnForwardRecipe.Multiplier = 1.2f;
	BurnForwardRecipe.Condition.RequiredNeighborEffectTags.AddTag(BurnEffectTag);
	MoonlightForward.LinkRecipes.Add(BurnForwardRecipe);

	FCombatCardLinkRecipe PoisonForwardRecipe;
	PoisonForwardRecipe.Direction = ECombatCardLinkOrientation::Forward;
	PoisonForwardRecipe.LinkFlow = PoisonForwardFlow;
	PoisonForwardRecipe.Multiplier = 1.4f;
	PoisonForwardRecipe.Condition.RequiredNeighborEffectTags.AddTag(PoisonEffectTag);
	MoonlightForward.LinkRecipes.Add(PoisonForwardRecipe);

	FCombatCardLinkRecipe ShieldReversedRecipe;
	ShieldReversedRecipe.Direction = ECombatCardLinkOrientation::Reversed;
	ShieldReversedRecipe.LinkFlow = ShieldReversedFlow;
	ShieldReversedRecipe.Multiplier = 1.6f;
	ShieldReversedRecipe.Condition.RequiredNeighborEffectTags.AddTag(ShieldEffectTag);
	MoonlightForward.LinkRecipes.Add(ShieldReversedRecipe);

	UCombatDeckComponent* ForwardDeck = NewObject<UCombatDeckComponent>();
	ForwardDeck->SetDeckListForTest({ PoisonCard, MoonlightForward });

	ForwardDeck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	const FCombatCardResolveResult ForwardResult = ForwardDeck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Forward Moonlight recipe triggers on the matching poison card"), ForwardResult.bTriggeredForwardLink);
	TestEqual(TEXT("Forward Moonlight selects the poison recipe multiplier"), ForwardResult.AppliedMultiplier, 1.4f);

	FCombatCardConfig MoonlightReversed = MoonlightForward;
	MoonlightReversed.DefaultLinkOrientation = ECombatCardLinkOrientation::Reversed;

	UCombatDeckComponent* ReversedDeck = NewObject<UCombatDeckComponent>();
	ReversedDeck->SetDeckListForTest({ MoonlightReversed, ShieldCard });

	const FCombatCardResolveResult PendingResult = ReversedDeck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Reversed Moonlight opens pending link with the matching recipe list"), PendingResult.bPendingBackwardLink);

	const FCombatCardResolveResult ReversedResult = ReversedDeck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Reversed Moonlight recipe triggers on the matching shield card"), ReversedResult.bTriggeredBackwardLink);
	TestEqual(TEXT("Reversed Moonlight selects the shield recipe multiplier"), ReversedResult.AppliedMultiplier, 1.6f);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedRequiresComboContinuationTest,
	"DevKit.CombatDeck.RecipeReversedRequiresComboContinuation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedRequiresComboContinuationTest::RunTest(const FString& Parameters)
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

	FCombatDeckActionContext MoonlightContext;
	MoonlightContext.ActionType = ECardRequiredAction::Light;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link during a valid combo"), MoonlightResult.bPendingBackwardLink);

	FCombatDeckActionContext RestartedAttackContext = MoonlightContext;
	RestartedAttackContext.AttackInstanceGuid = FGuid::NewGuid();
	RestartedAttackContext.bComboContinued = false;

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(RestartedAttackContext);
	TestTrue(TEXT("Restarted attack card is still consumed"), AttackResult.bHadCard);
	TestFalse(TEXT("Pending reversed link is cleared when the next attack is not a combo continuation"), AttackResult.bTriggeredBackwardLink);
	TestTrue(TEXT("Restarted attack falls back to its own base flow"), AttackResult.bTriggeredBaseFlow);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedMoonlightRecipesTriggerAllEffectsTest,
	"DevKit.CombatDeck.GeneratedMoonlightRecipesTriggerAllConfiguredEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedMoonlightRecipesTriggerAllEffectsTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* MoonlightForwardDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward.DA_Rune512_Moonlight_Forward"));
	URuneDataAsset* MoonlightReversedDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Reversed.DA_Rune512_Moonlight_Reversed"));

	TestNotNull(TEXT("Generated forward Moonlight DA exists"), MoonlightForwardDA);
	TestNotNull(TEXT("Generated reversed Moonlight DA exists"), MoonlightReversedDA);
	if (!MoonlightForwardDA || !MoonlightReversedDA)
	{
		return false;
	}

	const TArray<FName> EffectTagNames = {
		TEXT("Card.Effect.Burn"),
		TEXT("Card.Effect.Poison"),
		TEXT("Card.Effect.Split"),
		TEXT("Card.Effect.Shield"),
		TEXT("Card.Effect.Pierce"),
		TEXT("Card.Effect.Attack"),
		TEXT("Card.Effect.Defense.ReduceDamage"),
	};

	const FCombatCardConfig& ForwardMoonlightConfig = MoonlightForwardDA->RuneInfo.CombatCard;
	const FCombatCardConfig& ReversedMoonlightConfig = MoonlightReversedDA->RuneInfo.CombatCard;
	TestEqual(TEXT("Forward Moonlight has all forward and reversed generated recipes"), ForwardMoonlightConfig.LinkRecipes.Num(), 14);
	TestEqual(TEXT("Reversed Moonlight has all forward and reversed generated recipes"), ReversedMoonlightConfig.LinkRecipes.Num(), 14);

	for (const FName& EffectTagName : EffectTagNames)
	{
		const FGameplayTag EffectTag = FGameplayTag::RequestGameplayTag(EffectTagName, false);
		TestTrue(FString::Printf(TEXT("Effect tag exists: %s"), *EffectTagName.ToString()), EffectTag.IsValid());
		if (!EffectTag.IsValid())
		{
			continue;
		}

		FCombatCardConfig NeighborCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
		NeighborCard.CardEffectTags.AddTag(EffectTag);

		UCombatDeckComponent* ForwardDeck = NewObject<UCombatDeckComponent>();
		ForwardDeck->SetDeckListForTest({ NeighborCard, ForwardMoonlightConfig });

		FCombatDeckActionContext ForwardNeighborContext;
		ForwardNeighborContext.ActionType = ECardRequiredAction::Light;
		ForwardNeighborContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
		ForwardNeighborContext.bComboContinued = true;
		ForwardNeighborContext.AttackInstanceGuid = FGuid::NewGuid();
		ForwardDeck->ResolveAttackCardWithContext(ForwardNeighborContext);

		FCombatDeckActionContext ForwardMoonlightContext = ForwardNeighborContext;
		ForwardMoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
		const FCombatCardResolveResult ForwardResult = ForwardDeck->ResolveAttackCardWithContext(ForwardMoonlightContext);
		TestTrue(FString::Printf(TEXT("Generated forward Moonlight recipe triggers for %s"), *EffectTagName.ToString()),
			ForwardResult.bTriggeredForwardLink);
		TestTrue(FString::Printf(TEXT("Generated forward Moonlight recipe has LinkFlow for %s"), *EffectTagName.ToString()),
			ForwardResult.LinkedTargetCard.Config.LinkRecipes.ContainsByPredicate([EffectTag](const FCombatCardLinkRecipe& Recipe)
			{
				return Recipe.Direction == ECombatCardLinkOrientation::Forward
					&& Recipe.Condition.RequiredNeighborEffectTags.HasTagExact(EffectTag)
					&& Recipe.LinkFlow != nullptr;
			}));

		UCombatDeckComponent* ReversedDeck = NewObject<UCombatDeckComponent>();
		ReversedDeck->SetDeckListForTest({ ReversedMoonlightConfig, NeighborCard });

		FCombatDeckActionContext ReversedMoonlightContext;
		ReversedMoonlightContext.ActionType = ECardRequiredAction::Light;
		ReversedMoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
		ReversedMoonlightContext.bComboContinued = true;
		ReversedMoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
		const FCombatCardResolveResult PendingResult = ReversedDeck->ResolveAttackCardWithContext(ReversedMoonlightContext);
		TestTrue(FString::Printf(TEXT("Generated reversed Moonlight opens pending link for %s"), *EffectTagName.ToString()),
			PendingResult.bPendingBackwardLink);

		FCombatDeckActionContext ReversedNeighborContext = ReversedMoonlightContext;
		ReversedNeighborContext.AttackInstanceGuid = FGuid::NewGuid();
		const FCombatCardResolveResult ReversedResult = ReversedDeck->ResolveAttackCardWithContext(ReversedNeighborContext);
		TestTrue(FString::Printf(TEXT("Generated reversed Moonlight recipe triggers for %s"), *EffectTagName.ToString()),
			ReversedResult.bTriggeredBackwardLink);
		TestTrue(FString::Printf(TEXT("Generated reversed Moonlight recipe has LinkFlow for %s"), *EffectTagName.ToString()),
			ReversedResult.LinkedSourceCard.Config.LinkRecipes.ContainsByPredicate([EffectTag](const FCombatCardLinkRecipe& Recipe)
			{
				return Recipe.Direction == ECombatCardLinkOrientation::Reversed
					&& Recipe.Condition.RequiredNeighborEffectTags.HasTagExact(EffectTag)
					&& Recipe.LinkFlow != nullptr;
			}));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedComboDrivenAssetsConfiguredTest,
	"DevKit.CombatDeck.GeneratedComboDrivenAssetsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedComboDrivenAssetsConfiguredTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* AttackDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack.DA_Rune512_Attack"));
	TestNotNull(TEXT("Generated attack DA exists"), AttackDA);
	if (!AttackDA)
	{
		return false;
	}

	const FCombatCardConfig& AttackCard = AttackDA->RuneInfo.CombatCard;
	TestTrue(TEXT("Attack card uses combo effect scaling"), AttackCard.bUseComboEffectScaling);
	TestEqual(TEXT("Attack card combo scalar per index"), AttackCard.ComboScalarPerIndex, 0.25f);
	TestEqual(TEXT("Attack card max combo scalar"), AttackCard.MaxComboScalar, 0.5f);

	UFlowAsset* AttackFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Attack_Base.FA_Rune512_Attack_Base"));
	TestNotNull(TEXT("Generated attack base flow exists"), AttackFlow);
	if (!AttackFlow)
	{
		return false;
	}

	int32 ApplyAttributeNodeCount = 0;
	for (const TPair<FGuid, UFlowNode*>& Pair : AttackFlow->GetNodes())
	{
		if (const UBFNode_ApplyAttributeModifier* ApplyNode = Cast<UBFNode_ApplyAttributeModifier>(Pair.Value))
		{
			++ApplyAttributeNodeCount;
			TestTrue(TEXT("Attack apply-attribute node uses combat-card multiplier"), ApplyNode->bUseCombatCardEffectMultiplier);
		}
	}
	TestTrue(TEXT("Attack flow has apply-attribute node"), ApplyAttributeNodeCount > 0);

	UFlowAsset* MoonlightBaseFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Base.FA_Rune512_Moonlight_Base"));
	TestNotNull(TEXT("Generated moonlight base flow exists"), MoonlightBaseFlow);
	if (!MoonlightBaseFlow)
	{
		return false;
	}

	UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : MoonlightBaseFlow->GetNodes())
	{
		SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
		if (SlashNode)
		{
			break;
		}
	}

	TestNotNull(TEXT("Moonlight base flow has slash-wave node"), SlashNode);
	if (!SlashNode)
	{
		return false;
	}

	TestEqual(TEXT("Moonlight base projectile count"), SlashNode->ProjectileCount, 1);
	TestTrue(TEXT("Moonlight base adds combo stacks to projectile count"), SlashNode->bAddComboStacksToProjectileCount);
	TestEqual(TEXT("Moonlight base projectiles per combo stack"), SlashNode->ProjectilesPerComboStack, 1);
	TestEqual(TEXT("Moonlight base max bonus projectiles"), SlashNode->MaxBonusProjectiles, 2);
	TestEqual(TEXT("Moonlight base combo cone angle"), SlashNode->ProjectileConeAngleDegrees, 15.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedVfxUsesAtomicNiagaraTest,
	"DevKit.CombatDeck.GeneratedVfxUsesAtomicNiagara",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedVfxUsesAtomicNiagaraTest::RunTest(const FString& Parameters)
{
	auto ValidateMoonlightFlow = [this](const TCHAR* FlowPath, const TCHAR* Label) -> bool
	{
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		int32 SlashNodeCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (const UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value))
			{
				++SlashNodeCount;
				TestNull(FString::Printf(TEXT("%s projectile visual Niagara is cleared"), Label), SlashNode->ProjectileVisualNiagaraSystem);
				TestFalse(FString::Printf(TEXT("%s keeps default projectile visuals visible"), Label), SlashNode->bHideDefaultProjectileVisuals);
				TestNull(FString::Printf(TEXT("%s launch Niagara is cleared"), Label), SlashNode->LaunchNiagaraSystem);
				TestNull(FString::Printf(TEXT("%s hit Niagara is cleared"), Label), SlashNode->HitNiagaraSystem);
				TestNull(FString::Printf(TEXT("%s expire Niagara is cleared"), Label), SlashNode->ExpireNiagaraSystem);
			}
		}

		TestTrue(FString::Printf(TEXT("%s has slash-wave projectile node"), Label), SlashNodeCount > 0);
		return SlashNodeCount > 0;
	};

	auto ValidateStandaloneNiagaraFlow = [this](
		const TCHAR* FlowPath,
		const TCHAR* Label,
		const TCHAR* ExpectedEffectName,
		const TCHAR* ExpectedSystemName,
		const bool bExpectBurnDot) -> bool
	{
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		int32 ActiveUnexpectedNiagaraCount = 0;
		int32 ActiveFlipbookCount = 0;
		const UBFNode_PlayNiagara* ExpectedNiagaraNode = nullptr;
		UBFNode_ApplyEffect* BurnEffectNode = nullptr;
		const FName ExpectedEffectFName(ExpectedEffectName);
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (const UBFNode_PlayNiagara* NiagaraNode = Cast<UBFNode_PlayNiagara>(Pair.Value))
			{
				if (NiagaraNode->NiagaraSystem && NiagaraNode->EffectName == ExpectedEffectFName)
				{
					ExpectedNiagaraNode = NiagaraNode;
				}
				else if (NiagaraNode->NiagaraSystem)
				{
					++ActiveUnexpectedNiagaraCount;
				}
			}
			if (const UBFNode_PlayFlipbookVFX* FlipbookNode = Cast<UBFNode_PlayFlipbookVFX>(Pair.Value))
			{
				if (FlipbookNode->Texture || FlipbookNode->Material || FlipbookNode->PlaneMesh || FlipbookNode->EffectName != NAME_None)
				{
					++ActiveFlipbookCount;
				}
			}
			if (UBFNode_ApplyEffect* ApplyNode = Cast<UBFNode_ApplyEffect>(Pair.Value))
			{
				if (ApplyNode->Effect == UGE_RuneBurn::StaticClass())
				{
					BurnEffectNode = ApplyNode;
				}
			}
		}

		TestEqual(FString::Printf(TEXT("%s has no unexpected active Niagara nodes"), Label), ActiveUnexpectedNiagaraCount, 0);
		TestEqual(FString::Printf(TEXT("%s has no active flipbook nodes"), Label), ActiveFlipbookCount, 0);
		TestNotNull(FString::Printf(TEXT("%s has expected Play Niagara node"), Label), ExpectedNiagaraNode);
		if (ExpectedNiagaraNode)
		{
			TestTrue(FString::Printf(TEXT("%s Niagara system matches expected asset"), Label),
				GetNameSafe(ExpectedNiagaraNode->NiagaraSystem).Contains(ExpectedSystemName));
			TestEqual(FString::Printf(TEXT("%s Niagara targets last damage target"), Label),
				ExpectedNiagaraNode->AttachTarget,
				EBFTargetSelector::LastDamageTarget);
			TestTrue(FString::Printf(TEXT("%s Niagara attaches to target mesh"), Label), ExpectedNiagaraNode->bAttachToTarget);
			TestTrue(FString::Printf(TEXT("%s Niagara scale is compact"), Label),
				ExpectedNiagaraNode->Scale.X <= 0.5f && ExpectedNiagaraNode->Scale.Y <= 0.5f && ExpectedNiagaraNode->Scale.Z <= 0.5f);
			TestTrue(FString::Printf(TEXT("%s Niagara has explicit lifetime"), Label), ExpectedNiagaraNode->Lifetime > 0.f);
			TestFalse(FString::Printf(TEXT("%s Niagara survives Flow cleanup until lifetime"), Label), ExpectedNiagaraNode->bDestroyWithFlow);
		}

		if (bExpectBurnDot)
		{
			TestNotNull(FString::Printf(TEXT("%s applies persistent burn DOT"), Label), BurnEffectNode);
			if (BurnEffectNode)
			{
				TestEqual(FString::Printf(TEXT("%s burn targets last damage target"), Label), BurnEffectNode->Target, EBFTargetSelector::LastDamageTarget);
				TestFalse(FString::Printf(TEXT("%s burn GE survives Flow cleanup"), Label), BurnEffectNode->bRemoveEffectOnCleanup);
			}
		}

		return ExpectedNiagaraNode && ActiveUnexpectedNiagaraCount == 0 && ActiveFlipbookCount == 0;
	};

	bool bAllValid = true;
	bAllValid &= ValidateMoonlightFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Base.FA_Rune512_Moonlight_Base"),
		TEXT("Moonlight base"));
	bAllValid &= ValidateMoonlightFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Attack.FA_Rune512_Moonlight_Forward_Attack"),
		TEXT("Moonlight forward attack"));
	bAllValid &= ValidateMoonlightFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Attack.FA_Rune512_Moonlight_Reversed_Attack"),
		TEXT("Moonlight reversed attack"));
	bAllValid &= ValidateMoonlightFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Poison.FA_Rune512_Moonlight_Forward_Poison"),
		TEXT("Moonlight forward poison"));
	bAllValid &= ValidateMoonlightFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Poison.FA_Rune512_Moonlight_Reversed_Poison"),
		TEXT("Moonlight reversed poison"));
	bAllValid &= ValidateStandaloneNiagaraFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Burn_Base.FA_Rune512_Burn_Base"),
		TEXT("Burn base"),
		TEXT("Rune.Burn.ApplyNiagara"),
		TEXT("NS_Fire_Floor"),
		true);
	bAllValid &= ValidateStandaloneNiagaraFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Poison_Base.FA_Rune512_Poison_Base"),
		TEXT("Poison base"),
		TEXT("Rune.Poison.ApplyNiagara"),
		TEXT("NS_Smoke_7_acid"),
		false);

	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightBurnFlowUsesAtomicVfxNodesTest,
	"DevKit.CombatDeck.MoonlightBurnFlowUsesAtomicVfxNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightBurnFlowUsesAtomicVfxNodesTest::RunTest(const FString& Parameters)
{
	const FGameplayTag BurnHitTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Rune.MoonlightBurnHit"), false);
	TestTrue(TEXT("Moonlight burn hit event tag exists"), BurnHitTag.IsValid());

	auto ValidateBurnFlow = [this, BurnHitTag](const TCHAR* FlowPath, const TCHAR* Label) -> bool
	{
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s Flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
		UBFNode_WaitGameplayEvent* WaitNode = nullptr;
		UBFNode_PlayNiagara* BurnVfxNode = nullptr;
		UBFNode_ApplyEffect* BurnEffectNode = nullptr;
		int32 ActiveFlipbookCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (!SlashNode)
			{
				SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			}
			if (!WaitNode)
			{
				WaitNode = Cast<UBFNode_WaitGameplayEvent>(Pair.Value);
			}
			if (UBFNode_PlayNiagara* NiagaraNode = Cast<UBFNode_PlayNiagara>(Pair.Value))
			{
				if (NiagaraNode->EffectName == FName(TEXT("Rune.Moonlight.BurnHitNiagara")))
				{
					BurnVfxNode = NiagaraNode;
				}
			}
			if (const UBFNode_PlayFlipbookVFX* FlipbookNode = Cast<UBFNode_PlayFlipbookVFX>(Pair.Value))
			{
				if (FlipbookNode->Texture || FlipbookNode->Material || FlipbookNode->PlaneMesh || FlipbookNode->EffectName != NAME_None)
				{
					++ActiveFlipbookCount;
				}
			}
			if (UBFNode_ApplyEffect* ApplyNode = Cast<UBFNode_ApplyEffect>(Pair.Value))
			{
				if (ApplyNode->Effect == UGE_RuneBurn::StaticClass())
				{
					BurnEffectNode = ApplyNode;
				}
			}
		}

		TestNotNull(FString::Printf(TEXT("%s has slash-wave projectile node"), Label), SlashNode);
		TestNotNull(FString::Printf(TEXT("%s has wait gameplay event node"), Label), WaitNode);
		TestNotNull(FString::Printf(TEXT("%s has burn Niagara node"), Label), BurnVfxNode);
		TestNotNull(FString::Printf(TEXT("%s has persistent burn DOT node"), Label), BurnEffectNode);
		if (!SlashNode || !WaitNode || !BurnVfxNode || !BurnEffectNode)
		{
			return false;
		}

		TestEqual(FString::Printf(TEXT("%s slash-wave sends burn hit event"), Label), SlashNode->HitGameplayEventTag, BurnHitTag);
		TestNull(FString::Printf(TEXT("%s slash-wave does not use inline hit Niagara"), Label), SlashNode->HitNiagaraSystem);
		TestNull(FString::Printf(TEXT("%s slash-wave does not use inline expire Niagara"), Label), SlashNode->ExpireNiagaraSystem);
		TestEqual(FString::Printf(TEXT("%s wait node listens to burn hit event"), Label), WaitNode->EventTag, BurnHitTag);
		TestEqual(FString::Printf(TEXT("%s wait node listens on BuffOwner"), Label), WaitNode->Target, EBFTargetSelector::BuffOwner);
		TestEqual(FString::Printf(TEXT("%s burn VFX targets last damage target"), Label), BurnVfxNode->AttachTarget, EBFTargetSelector::LastDamageTarget);
		TestTrue(FString::Printf(TEXT("%s burn VFX uses NS_Fire_Floor"), Label), GetNameSafe(BurnVfxNode->NiagaraSystem).Contains(TEXT("NS_Fire_Floor")));
		TestTrue(FString::Printf(TEXT("%s burn VFX attaches to target"), Label), BurnVfxNode->bAttachToTarget);
		TestTrue(FString::Printf(TEXT("%s burn VFX remains compact"), Label),
			BurnVfxNode->Scale.X <= 0.5f && BurnVfxNode->Scale.Y <= 0.5f && BurnVfxNode->Scale.Z <= 0.5f);
		TestTrue(FString::Printf(TEXT("%s burn VFX lifetime covers DOT preview"), Label), BurnVfxNode->Lifetime >= 3.f);
		TestFalse(FString::Printf(TEXT("%s burn VFX is not destroyed by short Flow cleanup"), Label), BurnVfxNode->bDestroyWithFlow);
		TestEqual(FString::Printf(TEXT("%s has no active flipbook nodes"), Label), ActiveFlipbookCount, 0);
		TestEqual(FString::Printf(TEXT("%s burn DOT targets last damage target"), Label), BurnEffectNode->Target, EBFTargetSelector::LastDamageTarget);
		TestFalse(FString::Printf(TEXT("%s burn DOT survives Flow cleanup"), Label), BurnEffectNode->bRemoveEffectOnCleanup);
		TestEqual(FString::Printf(TEXT("%s burn DOT uses Data.Damage.Burn"), Label), BurnEffectNode->SetByCallerTag1.GetTagName(), FName(TEXT("Data.Damage.Burn")));

		return true;
	};

	const bool bForwardValid = ValidateBurnFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Burn.FA_Rune512_Moonlight_Forward_Burn"),
		TEXT("Forward burn Moonlight"));
	const bool bReversedValid = ValidateBurnFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Burn.FA_Rune512_Moonlight_Reversed_Burn"),
		TEXT("Reversed burn Moonlight"));

	return bForwardValid && bReversedValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightPoisonFlowUsesAtomicVfxNodesTest,
	"DevKit.CombatDeck.MoonlightPoisonFlowUsesAtomicVfxNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightPoisonFlowUsesAtomicVfxNodesTest::RunTest(const FString& Parameters)
{
	const FGameplayTag PoisonHitTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Rune.MoonlightPoisonHit"), false);
	const FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
	TestTrue(TEXT("Moonlight poison hit event tag exists"), PoisonHitTag.IsValid());
	TestTrue(TEXT("Data.Damage set-by-caller tag exists"), DamageTag.IsValid());

	auto ValidatePoisonFlow = [this, PoisonHitTag, DamageTag](const TCHAR* FlowPath, const TCHAR* Label) -> bool
	{
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s Flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
		UBFNode_WaitGameplayEvent* WaitNode = nullptr;
		UBFNode_PlayNiagara* PoisonHitVfxNode = nullptr;
		UBFNode_PlayNiagara* PoisonSpreadVfxNode = nullptr;
		int32 ActiveFlipbookCount = 0;
		UBFNode_ApplyEffect* PrimaryPoisonNode = nullptr;
		UBFNode_ApplyGEInRadius* RadiusPoisonNode = nullptr;

		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (!SlashNode)
			{
				SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			}
			if (!WaitNode)
			{
				WaitNode = Cast<UBFNode_WaitGameplayEvent>(Pair.Value);
			}
			if (UBFNode_PlayNiagara* NiagaraNode = Cast<UBFNode_PlayNiagara>(Pair.Value))
			{
				if (NiagaraNode->EffectName == FName(TEXT("Rune.Moonlight.PoisonHitNiagara")))
				{
					PoisonHitVfxNode = NiagaraNode;
				}
				else if (NiagaraNode->EffectName == FName(TEXT("Rune.Moonlight.PoisonSpreadNiagara")))
				{
					PoisonSpreadVfxNode = NiagaraNode;
				}
			}
			if (const UBFNode_PlayFlipbookVFX* FlipbookNode = Cast<UBFNode_PlayFlipbookVFX>(Pair.Value))
			{
				if (FlipbookNode->Texture || FlipbookNode->Material || FlipbookNode->PlaneMesh || FlipbookNode->EffectName != NAME_None)
				{
					++ActiveFlipbookCount;
				}
			}
			if (!PrimaryPoisonNode)
			{
				PrimaryPoisonNode = Cast<UBFNode_ApplyEffect>(Pair.Value);
			}
			if (!RadiusPoisonNode)
			{
				RadiusPoisonNode = Cast<UBFNode_ApplyGEInRadius>(Pair.Value);
			}
		}

		TestNotNull(FString::Printf(TEXT("%s has slash-wave projectile node"), Label), SlashNode);
		TestNotNull(FString::Printf(TEXT("%s has wait gameplay event node"), Label), WaitNode);
		TestNotNull(FString::Printf(TEXT("%s has poison hit Niagara node"), Label), PoisonHitVfxNode);
		TestNotNull(FString::Printf(TEXT("%s has poison spread Niagara node"), Label), PoisonSpreadVfxNode);
		TestEqual(FString::Printf(TEXT("%s has no active flipbook nodes"), Label), ActiveFlipbookCount, 0);
		TestNotNull(FString::Printf(TEXT("%s has primary poison node"), Label), PrimaryPoisonNode);
		TestNotNull(FString::Printf(TEXT("%s has radius poison node"), Label), RadiusPoisonNode);
		if (!SlashNode || !WaitNode || !PoisonHitVfxNode || !PoisonSpreadVfxNode || !PrimaryPoisonNode || !RadiusPoisonNode)
		{
			return false;
		}

		TestEqual(FString::Printf(TEXT("%s slash-wave sends poison hit event"), Label), SlashNode->HitGameplayEventTag, PoisonHitTag);
		TestNull(FString::Printf(TEXT("%s slash-wave does not use inline hit Niagara"), Label), SlashNode->HitNiagaraSystem);
		TestNull(FString::Printf(TEXT("%s slash-wave does not use inline expire Niagara"), Label), SlashNode->ExpireNiagaraSystem);
		TestNull(FString::Printf(TEXT("%s slash-wave does not use inline additional hit GE"), Label), SlashNode->AdditionalHitEffect);

		TestEqual(FString::Printf(TEXT("%s wait node listens to poison hit event"), Label), WaitNode->EventTag, PoisonHitTag);
		TestEqual(FString::Printf(TEXT("%s wait node listens on BuffOwner"), Label), WaitNode->Target, EBFTargetSelector::BuffOwner);

		TestTrue(FString::Printf(TEXT("%s poison hit VFX uses acid smoke Niagara"), Label),
			GetNameSafe(PoisonHitVfxNode->NiagaraSystem).Contains(TEXT("NS_Smoke_7_acid")));
		TestTrue(FString::Printf(TEXT("%s poison spread VFX uses acid smoke Niagara"), Label),
			GetNameSafe(PoisonSpreadVfxNode->NiagaraSystem).Contains(TEXT("NS_Smoke_7_acid")));
		TestEqual(FString::Printf(TEXT("%s poison hit VFX targets last damage target"), Label),
			PoisonHitVfxNode->AttachTarget,
			EBFTargetSelector::LastDamageTarget);
		TestEqual(FString::Printf(TEXT("%s poison spread VFX targets last damage target"), Label),
			PoisonSpreadVfxNode->AttachTarget,
			EBFTargetSelector::LastDamageTarget);
		TestTrue(FString::Printf(TEXT("%s poison hit VFX attaches to target"), Label), PoisonHitVfxNode->bAttachToTarget);
		TestFalse(FString::Printf(TEXT("%s poison spread VFX spawns at target location"), Label), PoisonSpreadVfxNode->bAttachToTarget);
		TestTrue(FString::Printf(TEXT("%s poison hit VFX remains compact"), Label),
			PoisonHitVfxNode->Scale.X <= 0.5f && PoisonHitVfxNode->Scale.Y <= 0.5f && PoisonHitVfxNode->Scale.Z <= 0.5f);
		TestTrue(FString::Printf(TEXT("%s poison spread VFX remains compact"), Label),
			PoisonSpreadVfxNode->Scale.X <= 0.6f && PoisonSpreadVfxNode->Scale.Y <= 0.6f && PoisonSpreadVfxNode->Scale.Z <= 0.6f);
		TestTrue(FString::Printf(TEXT("%s poison hit VFX has explicit lifetime"), Label), PoisonHitVfxNode->Lifetime > 0.f);
		TestTrue(FString::Printf(TEXT("%s poison spread VFX has explicit lifetime"), Label), PoisonSpreadVfxNode->Lifetime > 0.f);

		TestTrue(FString::Printf(TEXT("%s primary poison GE is assigned"), Label),
			PrimaryPoisonNode->Effect != nullptr && PrimaryPoisonNode->Effect->GetName().Contains(TEXT("GE_Poison")));
		TestEqual(FString::Printf(TEXT("%s applies three poison stacks to primary target"), Label), PrimaryPoisonNode->ApplicationCount, 3);
		TestEqual(FString::Printf(TEXT("%s primary poison targets last damage target"), Label), PrimaryPoisonNode->Target, EBFTargetSelector::LastDamageTarget);

		TestTrue(FString::Printf(TEXT("%s radius poison GE is assigned"), Label), RadiusPoisonNode->Effect != nullptr);
		TestEqual(FString::Printf(TEXT("%s radius is 300cm"), Label), RadiusPoisonNode->Radius.Value, 300.f);
		TestEqual(FString::Printf(TEXT("%s radius source is last damage target"), Label), RadiusPoisonNode->LocationSource, EBFTargetSelector::LastDamageTarget);
		TestTrue(FString::Printf(TEXT("%s excludes the primary target from radius spread"), Label), RadiusPoisonNode->bExcludeLocationSourceActor);
		TestEqual(FString::Printf(TEXT("%s limits secondary targets"), Label), RadiusPoisonNode->MaxTargets, 3);
		TestEqual(FString::Printf(TEXT("%s applies one stack to each secondary target"), Label), RadiusPoisonNode->ApplicationCount, 1);
		TestEqual(FString::Printf(TEXT("%s uses Data.Damage set-by-caller"), Label), RadiusPoisonNode->SetByCallerTag1, DamageTag);
		TestEqual(FString::Printf(TEXT("%s secondary poison damage is small"), Label), RadiusPoisonNode->SetByCallerValue1.Value, 5.f);

		return true;
	};

	const bool bForwardValid = ValidatePoisonFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Poison.FA_Rune512_Moonlight_Forward_Poison"),
		TEXT("Forward poison Moonlight"));
	const bool bReversedValid = ValidatePoisonFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Poison.FA_Rune512_Moonlight_Reversed_Poison"),
		TEXT("Reversed poison Moonlight"));

	return bForwardValid && bReversedValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckPoisonGameplayEffectsConfiguredTest,
	"DevKit.CombatDeck.PoisonGameplayEffectsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckPoisonGameplayEffectsConfiguredTest::RunTest(const FString& Parameters)
{
	const FGameplayTag PoisonedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Poisoned"), false);
	const FGameplayTag PoisonPercentTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Poison.PercentPerStack"), false);
	const FGameplayTag PoisonArmorPercentTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Poison.ArmorPercentPerStack"), false);
	TestTrue(TEXT("Buff.Status.Poisoned tag exists"), PoisonedTag.IsValid());
	TestTrue(TEXT("Data.Poison.PercentPerStack tag exists"), PoisonPercentTag.IsValid());
	TestTrue(TEXT("Data.Poison.ArmorPercentPerStack tag exists"), PoisonArmorPercentTag.IsValid());

	auto ValidatePoisonGE = [this, PoisonedTag](const TCHAR* PackagePath, const TCHAR* Label, const bool bExpectedExecuteOnApply, const int32 MinStackLimit) -> bool
	{
		UGameplayEffect* Effect = nullptr;
		{
			const FString Package(PackagePath);
			const FString AssetName = FPackageName::GetLongPackageAssetName(Package);
			const FString ClassPath = Package + TEXT(".") + AssetName + TEXT("_C");
			UClass* EffectClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), nullptr, *ClassPath));
			Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
		}

		TestNotNull(FString::Printf(TEXT("%s GE exists"), Label), Effect);
		if (!Effect)
		{
			return false;
		}

		TestEqual(FString::Printf(TEXT("%s uses duration"), Label), Effect->DurationPolicy, EGameplayEffectDurationType::HasDuration);
		TestEqual(FString::Printf(TEXT("%s period is 1s"), Label), Effect->Period.GetValueAtLevel(1.f), 1.f);
		TestEqual(FString::Printf(TEXT("%s execute-on-apply flag"), Label), Effect->bExecutePeriodicEffectOnApplication, bExpectedExecuteOnApply);
		TestEqual(FString::Printf(TEXT("%s has no legacy modifiers"), Label), Effect->Modifiers.Num(), 0);
		TestTrue(FString::Printf(TEXT("%s grants poisoned tag"), Label),
			PoisonedTag.IsValid() && Effect->GetGrantedTags().HasTagExact(PoisonedTag));
		TestEqual(FString::Printf(TEXT("%s stacks by target"), Label), Effect->StackingType, EGameplayEffectStackingType::AggregateByTarget);
		TestTrue(FString::Printf(TEXT("%s stack limit"), Label), Effect->StackLimitCount >= MinStackLimit);

		const bool bHasPoisonExecution = Effect->Executions.ContainsByPredicate([](const FGameplayEffectExecutionDefinition& ExecDef)
		{
			return ExecDef.CalculationClass == UGEExec_PoisonDamage::StaticClass();
		});
		TestTrue(FString::Printf(TEXT("%s uses GEExec_PoisonDamage"), Label), bHasPoisonExecution);

		return bHasPoisonExecution;
	};

	const bool bPrimaryValid = ValidatePoisonGE(
		TEXT("/Game/Code/GAS/Abilities/Shared/GE_Poison"),
		TEXT("GE_Poison"),
		false,
		20);
	const bool bSplashValid = ValidatePoisonGE(
		TEXT("/Game/Docs/BuffDocs/Playtest_GA/DeathPoison/GE_PoisonSplash"),
		TEXT("GE_PoisonSplash"),
		true,
		10);

	return bPrimaryValid && bSplashValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckBurnGameplayEffectConfiguredTest,
	"DevKit.CombatDeck.BurnGameplayEffectConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckBurnGameplayEffectConfiguredTest::RunTest(const FString& Parameters)
{
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Burning"), false);
	const FGameplayTag BurnDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Burn"), false);
	TestTrue(TEXT("Buff.Status.Burning tag exists"), BurningTag.IsValid());
	TestTrue(TEXT("Data.Damage.Burn tag exists"), BurnDamageTag.IsValid());

	const UGameplayEffect* Effect = UGE_RuneBurn::StaticClass()->GetDefaultObject<UGameplayEffect>();
	TestNotNull(TEXT("UGE_RuneBurn CDO exists"), Effect);
	if (!Effect)
	{
		return false;
	}

	TestEqual(TEXT("Burn uses duration"), Effect->DurationPolicy, EGameplayEffectDurationType::HasDuration);
	TestEqual(TEXT("Burn period is 1s"), Effect->Period.GetValueAtLevel(1.f), 1.f);
	TestEqual(TEXT("Burn does not tick on application"), Effect->bExecutePeriodicEffectOnApplication, false);
	TestTrue(TEXT("Burn grants burning tag"), BurningTag.IsValid() && Effect->GetGrantedTags().HasTagExact(BurningTag));
	TestEqual(TEXT("Burn is unique by target"), Effect->StackingType, EGameplayEffectStackingType::AggregateByTarget);
	TestEqual(TEXT("Burn stack limit is 1"), Effect->StackLimitCount, 1);

	const bool bHasBurnExecution = Effect->Executions.ContainsByPredicate([](const FGameplayEffectExecutionDefinition& ExecDef)
	{
		return ExecDef.CalculationClass == UGEExec_BurnDamage::StaticClass();
	});
	TestTrue(TEXT("Burn uses GEExec_BurnDamage"), bHasBurnExecution);
	return bHasBurnExecution;
}

#endif
