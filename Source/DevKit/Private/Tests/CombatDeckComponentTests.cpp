#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Component/CombatDeckComponent.h"
#include "Component/BufferComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Component/SacrificeRuneComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayerDash.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h"
#include "AbilitySystem/GameplayEffect/GE_RuneBurn.h"
#include "AbilitySystem/Execution/GEExec_BurnDamage.h"
#include "AbilitySystem/Execution/GEExec_PoisonDamage.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_ApplyGEInRadius.h"
#include "BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h"
#include "BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h"
#include "BuffFlow/Nodes/BFNode_CombatCardContext.h"
#include "BuffFlow/Nodes/BFNode_DoDamage.h"
#include "BuffFlow/Nodes/BFNode_GrantSacrificePassive.h"
#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"
#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"
#include "BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h"
#include "BuffFlow/Nodes/BFNode_MathFloat.h"
#include "BuffFlow/Nodes/BFNode_OnDamageDealt.h"
#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Data/AbilityData.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "Data/SpecialAttackDataAsset.h"
#include "Data/WeaponComboNodeConfig.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include "UI/WeaponComboTextUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"
#include "NiagaraSystem.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckNormalReleaseHoldsFinisherReleaseCardTest,
	"DevKit.CombatDeck.NormalReleaseHoldsFinisherReleaseCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckNormalReleaseHoldsFinisherReleaseCardTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig FinisherReleaseCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	FinisherReleaseCard.bRequiresComboFinisher = true;
	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	Deck->SetDeckListForTest({ FinisherReleaseCard, AttackCard });

	FCombatDeckActionContext NormalContext;
	NormalContext.ActionType = ECardRequiredAction::Light;
	NormalContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	NormalContext.ReleaseMode = ECombatCardReleaseMode::Normal;
	NormalContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult NormalResult = Deck->ResolveAttackCardWithContext(NormalContext);

	TestTrue(TEXT("A finisher-release card is visible to the normal release attempt"), NormalResult.bHadCard);
	TestFalse(TEXT("Normal release does not match a finisher-release card"), NormalResult.bReleaseModeMatched);
	TestFalse(TEXT("Release mismatch does not execute base flow"), NormalResult.bTriggeredBaseFlow);
	TestFalse(TEXT("Release mismatch does not trigger a finisher"), NormalResult.bTriggeredFinisher);
	TestEqual(TEXT("Release mismatch holds the card in the current slot"), Deck->GetCurrentIndex(), 0);

	FCombatDeckActionContext FinisherContext = NormalContext;
	FinisherContext.bIsComboFinisher = true;
	FinisherContext.ReleaseMode = ECombatCardReleaseMode::Finisher;
	FinisherContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult FinisherResult = Deck->ResolveAttackCardWithContext(FinisherContext);

	TestTrue(TEXT("Finisher release consumes the held card"), FinisherResult.bHadCard);
	TestTrue(TEXT("Finisher release matches the held card"), FinisherResult.bReleaseModeMatched);
	TestTrue(TEXT("Finisher release runs the card flow"), FinisherResult.bTriggeredBaseFlow);
	TestTrue(TEXT("Finisher-release card raises finisher trigger result"), FinisherResult.bTriggeredFinisher);
	TestEqual(TEXT("Finisher release advances to the next card"), Deck->GetCurrentIndex(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckFinisherReleaseHoldsNormalCardTest,
	"DevKit.CombatDeck.FinisherReleaseHoldsNormalCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckFinisherReleaseHoldsNormalCardTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
	});

	FCombatDeckActionContext FinisherContext;
	FinisherContext.ActionType = ECardRequiredAction::Light;
	FinisherContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	FinisherContext.bIsComboFinisher = true;
	FinisherContext.ReleaseMode = ECombatCardReleaseMode::Finisher;
	FinisherContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult FinisherResult = Deck->ResolveAttackCardWithContext(FinisherContext);

	TestTrue(TEXT("A normal card is visible to the finisher release attempt"), FinisherResult.bHadCard);
	TestFalse(TEXT("Finisher release does not match a normal card"), FinisherResult.bReleaseModeMatched);
	TestFalse(TEXT("Finisher release does not consume normal card base flow"), FinisherResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Finisher release holds normal cards for normal attacks"), Deck->GetCurrentIndex(), 0);

	FCombatDeckActionContext NormalContext = FinisherContext;
	NormalContext.bIsComboFinisher = false;
	NormalContext.ReleaseMode = ECombatCardReleaseMode::Normal;
	NormalContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult NormalResult = Deck->ResolveAttackCardWithContext(NormalContext);

	TestTrue(TEXT("Normal release consumes the held normal card"), NormalResult.bHadCard);
	TestTrue(TEXT("Normal release matches the held normal card"), NormalResult.bReleaseModeMatched);
	TestTrue(TEXT("Normal release runs base flow"), NormalResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Normal release advances the deck"), Deck->GetCurrentIndex(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckActionSlotRoutesSkillCardAwayFromAttackSequenceTest,
	"DevKit.CombatDeck.ActionSlotRoutesSkillCardAwayFromAttackSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckActionSlotRoutesSkillCardAwayFromAttackSequenceTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig SkillCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	SkillCard.RequiredActionSlot = ECombatDeckActionSlot::Skill;
	SkillCard.RequiredFlowRole = ECombatDeckFlowRole::Catalyst;
	Deck->SetDeckListForTest({ SkillCard });

	FCombatDeckActionContext AttackContext;
	AttackContext.ActionType = ECardRequiredAction::Light;
	AttackContext.ActionSlot = ECombatDeckActionSlot::Attack;
	AttackContext.FlowRole = ECombatDeckFlowRole::Starter;
	AttackContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(AttackContext);

	TestFalse(TEXT("Attack sequence ignores a skill-slot card"), AttackResult.bHadCard);
	TestFalse(TEXT("Attack sequence does not trigger a skill-slot card flow"), AttackResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Skill-slot card does not enter the attack sequence"), Deck->GetRemainingDeckSnapshot().Num(), 0);

	FCombatDeckActionContext SkillContext = AttackContext;
	SkillContext.ActionSlot = ECombatDeckActionSlot::Skill;
	SkillContext.FlowRole = ECombatDeckFlowRole::Catalyst;
	SkillContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult SkillResult = Deck->ResolveAttackCardWithContext(SkillContext);

	TestTrue(TEXT("Skill-slot context consumes the held card"), SkillResult.bHadCard);
	TestTrue(TEXT("Skill-slot context matches the held card"), SkillResult.bActionSlotMatched);
	TestTrue(TEXT("Skill-slot context runs base flow"), SkillResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Skill-slot card does not advance the attack sequence"), Deck->GetCurrentIndex(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckSkillSlotCanConsumeOnCommitTest,
	"DevKit.CombatDeck.SkillSlotCanConsumeOnCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckSkillSlotCanConsumeOnCommitTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig SkillCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	SkillCard.RequiredActionSlot = ECombatDeckActionSlot::Skill;
	SkillCard.RequiredFlowRole = ECombatDeckFlowRole::Catalyst;
	SkillCard.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	Deck->SetDeckListForTest({ SkillCard });

	FCombatDeckActionContext PreviewContext;
	PreviewContext.ActionType = ECardRequiredAction::Any;
	PreviewContext.ActionSlot = ECombatDeckActionSlot::Skill;
	PreviewContext.FlowRole = ECombatDeckFlowRole::Catalyst;
	PreviewContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	PreviewContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult PreviewResult = Deck->ResolveAttackCardWithContext(PreviewContext);

	TestFalse(TEXT("OnCommit without consume flag only previews/prepares card flow"), PreviewResult.bHadCard);
	TestEqual(TEXT("OnCommit preview holds the skill card"), Deck->GetCurrentIndex(), 0);

	FCombatDeckActionContext ConsumeContext = PreviewContext;
	ConsumeContext.bConsumeOnCommit = true;
	ConsumeContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult ConsumeResult = Deck->ResolveAttackCardWithContext(ConsumeContext);

	TestTrue(TEXT("Skill-slot OnCommit can consume when explicitly requested"), ConsumeResult.bHadCard);
	TestTrue(TEXT("Skill-slot OnCommit matches slot"), ConsumeResult.bActionSlotMatched);
	TestTrue(TEXT("Skill-slot OnCommit matches catalyst role"), ConsumeResult.bFlowRoleMatched);
	TestTrue(TEXT("Skill-slot OnCommit runs base flow"), ConsumeResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Consumed skill-slot card does not advance the attack sequence"), Deck->GetCurrentIndex(), 0);

	ConsumeContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult RepeatResult = Deck->ResolveAttackCardWithContext(ConsumeContext);
	TestTrue(TEXT("Single skill slot can trigger again on the next skill use"), RepeatResult.bHadCard);
	TestEqual(TEXT("Repeated skill slot trigger still leaves attack sequence untouched"), Deck->GetCurrentIndex(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckFlowRoleMismatchHoldsCardTest,
	"DevKit.CombatDeck.FlowRoleMismatchHoldsCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckFlowRoleMismatchHoldsCardTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig FinisherRoleCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	FinisherRoleCard.RequiredActionSlot = ECombatDeckActionSlot::WeaponSkill;
	FinisherRoleCard.RequiredFlowRole = ECombatDeckFlowRole::Finisher;
	Deck->SetDeckListForTest({ FinisherRoleCard });

	FCombatDeckActionContext CatalystContext;
	CatalystContext.ActionType = ECardRequiredAction::Heavy;
	CatalystContext.ActionSlot = ECombatDeckActionSlot::WeaponSkill;
	CatalystContext.FlowRole = ECombatDeckFlowRole::Catalyst;
	CatalystContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	CatalystContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult CatalystResult = Deck->ResolveAttackCardWithContext(CatalystContext);

	TestTrue(TEXT("A finisher-role card is visible to catalyst attempts"), CatalystResult.bHadCard);
	TestTrue(TEXT("Catalyst attempt still matches the action slot"), CatalystResult.bActionSlotMatched);
	TestFalse(TEXT("Catalyst attempt does not match finisher flow role"), CatalystResult.bFlowRoleMatched);
	TestFalse(TEXT("Flow-role mismatch does not trigger base flow"), CatalystResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Flow-role mismatch holds the card in the current slot"), Deck->GetCurrentIndex(), 0);

	FCombatDeckActionContext FinisherContext = CatalystContext;
	FinisherContext.FlowRole = ECombatDeckFlowRole::Finisher;
	FinisherContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult FinisherResult = Deck->ResolveAttackCardWithContext(FinisherContext);

	TestTrue(TEXT("Finisher role consumes the held card"), FinisherResult.bHadCard);
	TestTrue(TEXT("Finisher role matches the held card"), FinisherResult.bFlowRoleMatched);
	TestTrue(TEXT("Finisher role runs base flow"), FinisherResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Matched finisher-role card does not advance the attack sequence"), Deck->GetCurrentIndex(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckSingleActionSlotsRouteFromSourceAssetsTest,
	"DevKit.CombatDeck.SingleActionSlotsRouteFromSourceAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckSingleActionSlotsRouteFromSourceAssetsTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* AttackRune = NewObject<URuneDataAsset>();
	AttackRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
	AttackRune->RuneInfo.CombatCard.RequiredActionSlot = ECombatDeckActionSlot::Attack;

	URuneDataAsset* SkillRune = NewObject<URuneDataAsset>();
	SkillRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
	SkillRune->RuneInfo.CombatCard.RequiredActionSlot = ECombatDeckActionSlot::Skill;
	SkillRune->RuneInfo.CombatCard.RequiredFlowRole = ECombatDeckFlowRole::Catalyst;

	URuneDataAsset* WeaponSkillRune = NewObject<URuneDataAsset>();
	WeaponSkillRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
	WeaponSkillRune->RuneInfo.CombatCard.RequiredActionSlot = ECombatDeckActionSlot::WeaponSkill;
	WeaponSkillRune->RuneInfo.CombatCard.RequiredFlowRole = ECombatDeckFlowRole::Finisher;

	URuneDataAsset* DashRune = NewObject<URuneDataAsset>();
	DashRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
	DashRune->RuneInfo.CombatCard.RequiredActionSlot = ECombatDeckActionSlot::Dash;
	DashRune->RuneInfo.CombatCard.RequiredFlowRole = ECombatDeckFlowRole::Catalyst;

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->LoadDeckFromExactSourceAssets({ AttackRune, SkillRune, WeaponSkillRune, DashRune }, 1.0f, 4);

	TestEqual(TEXT("Only attack-slot cards enter the attack sequence"), Deck->GetDeckSnapshot().Num(), 1);
	TestEqual(TEXT("Skill source asset is stored in the skill slot"),
		Deck->GetActionSlotCardSnapshot(ECombatDeckActionSlot::Skill).SourceData.Get(), SkillRune);
	TestEqual(TEXT("Weapon skill source asset is stored in the weapon-skill slot"),
		Deck->GetActionSlotCardSnapshot(ECombatDeckActionSlot::WeaponSkill).SourceData.Get(), WeaponSkillRune);
	TestEqual(TEXT("Dash source asset is stored in the dash slot"),
		Deck->GetActionSlotCardSnapshot(ECombatDeckActionSlot::Dash).SourceData.Get(), DashRune);
	TestEqual(TEXT("Full deck UI snapshot stays attack-slot only"), Deck->GetFullDeckSnapshot().Num(), 1);

	const TArray<URuneDataAsset*> SourceAssets = Deck->GetDeckSourceAssets();
	TestTrue(TEXT("Saved source assets include attack card"), SourceAssets.Contains(AttackRune));
	TestTrue(TEXT("Saved source assets include skill slot card"), SourceAssets.Contains(SkillRune));
	TestTrue(TEXT("Saved source assets include weapon skill slot card"), SourceAssets.Contains(WeaponSkillRune));
	TestTrue(TEXT("Saved source assets include dash slot card"), SourceAssets.Contains(DashRune));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckDeprecatedFinisherCardsAreIgnoredTest,
	"DevKit.CombatDeck.DeprecatedFinisherCardsAreIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckDeprecatedFinisherCardsAreIgnoredTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	FCombatCardConfig FinisherCard{ ECombatCardType::Finisher, ECardRequiredAction::Any };
	FinisherCard.bRequiresComboFinisher = true;
	Deck->SetDeckListForTest({ FinisherCard });

	const FCombatCardResolveResult NormalAttack = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestFalse(TEXT("Deprecated finisher card is not inserted into the deck"), NormalAttack.bHadCard);

	const FCombatCardResolveResult ComboFinisher = Deck->ResolveAttackCard(ECardRequiredAction::Light, true, false);
	TestFalse(TEXT("Combo finisher flag does not reactivate deprecated finisher cards"), ComboFinisher.bHadCard);
	TestFalse(TEXT("Deprecated finisher bonus never triggers"), ComboFinisher.bTriggeredFinisher);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckDeprecatedFinisherDoesNotSuppressRefreshTest,
	"DevKit.CombatDeck.DeprecatedFinisherDoesNotSuppressRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckDeprecatedFinisherDoesNotSuppressRefreshTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetShuffleCooldownDuration(1.0f);

	FCombatCardConfig FinisherCard{ ECombatCardType::Finisher, ECardRequiredAction::Any };
	FinisherCard.bRequiresComboFinisher = true;
	FinisherCard.DisplayName = FText::FromString(TEXT("Finisher"));

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Light };
	AttackCard.DisplayName = FText::FromString(TEXT("Attack"));

	Deck->SetDeckListForTest({ FinisherCard, AttackCard });

	const TArray<FCombatCardInstance> InitialCards = Deck->GetDeckSnapshot();
	TestEqual(TEXT("Deprecated finisher is filtered out of the active deck"), InitialCards.Num(), 1);
	if (InitialCards.Num() == 1)
	{
		TestEqual(TEXT("Attack card remains after finisher filtering"), InitialCards[0].Config.CardType, ECombatCardType::Attack);
	}

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Light, false, false);
	TestTrue(TEXT("Attack card can still be consumed"), AttackResult.bHadCard);
	TestTrue(TEXT("Consuming the remaining attack card starts shuffle"), AttackResult.bStartedShuffle);
	TestFalse(TEXT("Deprecated finisher card never enters suppression tracking"),
		Deck->IsCardSuppressedFromActiveSequenceForTest(FGuid::NewGuid()));

	Deck->AdvanceShuffleForTest(Deck->GetShuffleCooldownDuration());
	const TArray<FCombatCardInstance> RefreshedCards = Deck->GetDeckSnapshot();
	TestEqual(TEXT("Refresh keeps only the non-finisher card"), RefreshedCards.Num(), 1);
	if (RefreshedCards.Num() == 1)
	{
		TestEqual(TEXT("Attack card fills the refreshed deck"), RefreshedCards[0].Config.CardType, ECombatCardType::Attack);
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRewardCardAutoReloadsIntoVisibleSequenceTest,
	"DevKit.CombatDeck.RewardCardAutoReloadsIntoVisibleSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRewardCardAutoReloadsIntoVisibleSequenceTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* FirstRune = NewObject<URuneDataAsset>();
	FirstRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light };

	URuneDataAsset* SecondRune = NewObject<URuneDataAsset>();
	SecondRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light };

	URuneDataAsset* RewardRune = NewObject<URuneDataAsset>();
	RewardRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Heavy };

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->LoadDeckFromExactSourceAssets({ FirstRune, SecondRune }, 1.0f, 2);
	TestEqual(TEXT("Initial active sequence is capped at two cards"), Deck->GetRemainingDeckSnapshot().Num(), 2);

	TestTrue(TEXT("Reward rune enters combat deck"), Deck->AddCardFromRuneReward(RewardRune));
	TestEqual(TEXT("Reward pickup starts one automatic reload shuffle"), Deck->GetDeckState(), EDeckState::EmptyShuffling);

	Deck->AdvanceShuffleForTest(Deck->GetShuffleCooldownDuration());
	const TArray<FCombatCardInstance> VisibleCards = Deck->GetRemainingDeckSnapshot();
	bool bRewardVisible = false;
	for (const FCombatCardInstance& Card : VisibleCards)
	{
		bRewardVisible |= Card.SourceData == RewardRune;
	}

	TestTrue(TEXT("Reward card is visible after the automatic reload"), bRewardVisible);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComboGraphMeleeAbilityHasActionAndDeathGuardsTest,
	"DevKit.CombatDeck.ComboGraphMeleeAbilityHasActionAndDeathGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComboGraphMeleeAbilityHasActionAndDeathGuardsTest::RunTest(const FString& Parameters)
{
	UGA_MeleeAttack* Ability = NewObject<UGA_MeleeAttack>();
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"));

	TestTrue(TEXT("Generic ComboGraph melee ability is blocked while dead"),
		Ability->GetActivationBlockedTags().HasTagExact(DeadTag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDashHasDefaultInterruptAndDeathGuardsTest,
	"DevKit.CombatDeck.PlayerDashHasDefaultInterruptAndDeathGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDashHasDefaultInterruptAndDeathGuardsTest::RunTest(const FString& Parameters)
{
	UGA_PlayerDash* Ability = NewObject<UGA_PlayerDash>();
	const FGameplayTag DashTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash"));
	const FGameplayTag DashInvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.DashInvincible"));
	const FGameplayTag ActionTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast"));
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"));

	TestTrue(TEXT("Dash ability carries the dash action tag by default"),
		Ability->GetAbilityTags().HasTagExact(DashTag));
	TestTrue(TEXT("Dash ability owns the dash action tag while active"),
		Ability->GetActivationOwnedTags().HasTagExact(DashTag));
	TestTrue(TEXT("Dash ability owns invincibility while active"),
		Ability->GetActivationOwnedTags().HasTagExact(DashInvincibleTag));
	TestTrue(TEXT("Dash cancels currently active action abilities by default"),
		Ability->GetCancelAbilitiesWithTag().HasTagExact(ActionTag));
	TestTrue(TEXT("Dash cannot activate while dead"),
		Ability->GetActivationBlockedTags().HasTagExact(DeadTag));
	TestTrue(TEXT("Dash cannot retrigger while another dash action is active"),
		Ability->GetActivationBlockedTags().HasTagExact(DashTag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDeathClearsComboRuntimeStateTest,
	"DevKit.CombatDeck.PlayerDeathClearsComboRuntimeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDeathClearsComboRuntimeStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for player death combo runtime test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for death combo runtime test"), Player);
	if (!Player)
	{
		return false;
	}

	UYogAbilitySystemComponent* ASC = Player->GetASC();
	TestNotNull(TEXT("Player has Yog ASC"), ASC);
	TestNotNull(TEXT("Player has ComboRuntimeComponent"), Player->ComboRuntimeComponent.Get());
	if (!ASC || !Player->ComboRuntimeComponent)
	{
		Player->Destroy();
		return false;
	}

	ASC->InitAbilityActorInfo(Player, Player);

	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>(Player);
	UGameplayAbilityComboGraphNode* Root = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	Root->Graph = Graph;
	Root->NodeId = TEXT("L1");
	Root->RootInputAction = EYogComboGraphInputAction::Light;
	Root->Montage = NewObject<UAnimMontage>(Graph);
	Graph->AllNodes = { Root };
	Graph->RootNodes = { Root };

	Player->ComboRuntimeComponent->LoadComboGraph(Graph);
	TestTrue(TEXT("Combo runtime can enter an active graph node before death"),
		Player->ComboRuntimeComponent->TryActivateComboGraphNode(EYogComboGraphInputAction::Light, FGameplayTagContainer()));
	TestEqual(TEXT("Combo runtime stores current node before death"),
		Player->ComboRuntimeComponent->GetCurrentNodeId(), FName(TEXT("L1")));
	TestTrue(TEXT("Combo runtime stores active attack guid before death"),
		Player->ComboRuntimeComponent->GetActiveAttackGuid().IsValid());

	Player->Die();

	TestEqual(TEXT("Player death clears current combo node"),
		Player->ComboRuntimeComponent->GetCurrentNodeId(), FName(NAME_None));
	TestEqual(TEXT("Player death clears active graph node"),
		Player->ComboRuntimeComponent->GetActiveGraphNodeId(), FName(NAME_None));
	TestFalse(TEXT("Player death clears active attack guid"),
		Player->ComboRuntimeComponent->GetActiveAttackGuid().IsValid());

	Player->Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDefaultUnarmedComboGraphLoadsFallbackTest,
	"DevKit.CombatDeck.PlayerDefaultUnarmedComboGraphLoadsFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDefaultUnarmedComboGraphLoadsFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for unarmed combo fallback test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for unarmed combo fallback test"), Player);
	if (!Player)
	{
		return false;
	}

	TestNotNull(TEXT("Player has a C++ default unarmed combo graph asset"),
		Player->DefaultUnarmedComboGraph.Get());
	if (Player->DefaultUnarmedComboGraph)
	{
		TestEqual(TEXT("Default unarmed combo graph points at the Disarm graph asset"),
			Player->DefaultUnarmedComboGraph->GetPathName(),
			FString(TEXT("/Game/Code/Weapon/Disarm/GA_ComboGraph_Disarm.GA_ComboGraph_Disarm")));
	}

	UGameplayAbilityComboGraph* DefaultGraph = NewObject<UGameplayAbilityComboGraph>(Player);
	Player->DefaultUnarmedComboGraph = DefaultGraph;
	Player->ApplyDefaultUnarmedComboGraph();

	TestTrue(TEXT("Default unarmed combo graph becomes the active combo graph"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetComboGraph() == DefaultGraph);
	TestTrue(TEXT("Default unarmed combo graph counts as a combo source"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->HasComboSource());

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerWeaponComboGraphOverridesAndResetToUnarmedTest,
	"DevKit.CombatDeck.PlayerWeaponComboGraphOverridesAndResetToUnarmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerWeaponComboGraphOverridesAndResetToUnarmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for weapon combo override test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for weapon combo override test"), Player);
	if (!Player)
	{
		return false;
	}

	UGameplayAbilityComboGraph* DefaultGraph = NewObject<UGameplayAbilityComboGraph>(Player);
	UGameplayAbilityComboGraph* WeaponGraph = NewObject<UGameplayAbilityComboGraph>(Player);
	UWeaponDefinition* WeaponDef = NewObject<UWeaponDefinition>(Player);
	WeaponDef->GameplayAbilityComboGraph = WeaponGraph;

	Player->DefaultUnarmedComboGraph = DefaultGraph;
	Player->ApplyDefaultUnarmedComboGraph();
	Player->ApplyComboGraphFromWeapon(WeaponDef);

	TestTrue(TEXT("Weapon combo graph overrides default unarmed graph"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetComboGraph() == WeaponGraph);

	Player->EquippedWeaponDef = WeaponDef;
	Player->ApplyDefaultUnarmedComboGraph();
	Player->ApplyCurrentEquipmentComboGraph();

	TestTrue(TEXT("Current equipment combo graph refresh restores weapon graph over default"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetComboGraph() == WeaponGraph);

	Player->ResetToDefaultUnarmedCombatState();

	TestNull(TEXT("Reset to unarmed clears equipped weapon definition"), Player->EquippedWeaponDef.Get());
	TestTrue(TEXT("Reset to unarmed restores default combo graph"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetComboGraph() == DefaultGraph);

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSkillMissResetsComboRuntimeToRootTest,
	"DevKit.CombatDeck.WeaponSkillMissResetsComboRuntimeToRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSkillMissResetsComboRuntimeToRootTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for weapon-skill miss reset test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for weapon-skill miss reset test"), Player);
	if (!Player)
	{
		return false;
	}

	UYogAbilitySystemComponent* ASC = Player->GetASC();
	TestNotNull(TEXT("Player has Yog ASC"), ASC);
	TestNotNull(TEXT("Player has ComboRuntimeComponent"), Player->ComboRuntimeComponent.Get());
	if (!ASC || !Player->ComboRuntimeComponent)
	{
		Player->Destroy();
		return false;
	}

	ASC->InitAbilityActorInfo(Player, Player);

	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>(Player);
	UGameplayAbilityComboGraphNode* Root = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	Root->Graph = Graph;
	Root->NodeId = TEXT("L1");
	Root->RootInputAction = EYogComboGraphInputAction::Light;
	Root->Montage = NewObject<UAnimMontage>(Graph);
	Graph->AllNodes = { Root };
	Graph->RootNodes = { Root };

	Player->ComboRuntimeComponent->LoadComboGraph(Graph);
	TestTrue(TEXT("Combo runtime can enter a normal attack node before weapon-skill miss"),
		Player->ComboRuntimeComponent->TryActivateComboGraphNode(EYogComboGraphInputAction::Light, FGameplayTagContainer()));
	TestEqual(TEXT("Combo runtime stores current node before weapon-skill miss"),
		Player->ComboRuntimeComponent->GetCurrentNodeId(), FName(TEXT("L1")));
	TestTrue(TEXT("Combo runtime stores active attack guid before weapon-skill miss"),
		Player->ComboRuntimeComponent->GetActiveAttackGuid().IsValid());

	TestFalse(TEXT("Weapon-skill input with no graph node fails activation"),
		Player->ComboRuntimeComponent->TryActivateWeaponSkill(Player));
	TestEqual(TEXT("Weapon-skill miss clears current combo node"),
		Player->ComboRuntimeComponent->GetCurrentNodeId(), FName(NAME_None));
	TestEqual(TEXT("Weapon-skill miss clears active graph node"),
		Player->ComboRuntimeComponent->GetActiveGraphNodeId(), FName(NAME_None));
	TestFalse(TEXT("Weapon-skill miss clears active attack guid"),
		Player->ComboRuntimeComponent->GetActiveAttackGuid().IsValid());

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSwitchWeaponSwapsActiveAndInactiveSlotsTest,
	"DevKit.CombatDeck.PlayerSwitchWeaponSwapsActiveAndInactiveSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerSwitchWeaponSwapsActiveAndInactiveSlotsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for weapon switch test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for weapon switch test"), Player);
	if (!Player)
	{
		return false;
	}

	UGameplayAbilityComboGraph* GraphA = NewObject<UGameplayAbilityComboGraph>(Player);
	UGameplayAbilityComboGraph* GraphB = NewObject<UGameplayAbilityComboGraph>(Player);
	UWeaponDefinition* WeaponA = NewObject<UWeaponDefinition>(Player);
	UWeaponDefinition* WeaponB = NewObject<UWeaponDefinition>(Player);
	WeaponA->GameplayAbilityComboGraph = GraphA;
	WeaponB->GameplayAbilityComboGraph = GraphB;

	AWeaponInstance* InstanceA = World->SpawnActor<AWeaponInstance>();
	AWeaponInstance* InstanceB = World->SpawnActor<AWeaponInstance>();
	TestNotNull(TEXT("Active weapon instance spawned"), InstanceA);
	TestNotNull(TEXT("Inactive weapon instance spawned"), InstanceB);
	if (!InstanceA || !InstanceB)
	{
		if (InstanceA)
		{
			InstanceA->Destroy();
		}
		if (InstanceB)
		{
			InstanceB->Destroy();
		}
		Player->Destroy();
		return false;
	}

	Player->EquippedWeaponDef = WeaponA;
	Player->InactiveWeaponDef = WeaponB;
	Player->EquippedWeaponInstance = InstanceA;
	Player->InactiveWeaponInstance = InstanceB;
	InstanceA->SetActorHiddenInGame(false);
	InstanceB->SetActorHiddenInGame(true);

	Player->ApplyComboGraphFromWeapon(WeaponA);
	TestTrue(TEXT("Player reports a second weapon can be switched to"), Player->CanSwitchWeapon());

	Player->SwitchWeapon();

	TestEqual(TEXT("Switch weapon promotes inactive definition"),
		Player->EquippedWeaponDef.Get(), WeaponB);
	TestEqual(TEXT("Switch weapon demotes previous active definition"),
		Player->InactiveWeaponDef.Get(), WeaponA);
	TestEqual(TEXT("Switch weapon promotes inactive instance"),
		Player->EquippedWeaponInstance.Get(), InstanceB);
	TestEqual(TEXT("Switch weapon demotes previous active instance"),
		Player->InactiveWeaponInstance.Get(), InstanceA);
	TestFalse(TEXT("Promoted weapon instance is visible"), InstanceB->IsHidden());
	TestTrue(TEXT("Demoted weapon instance is hidden"), InstanceA->IsHidden());
	TestTrue(TEXT("Promoted weapon combo graph is active"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetWeaponComboGraph() == GraphB);

	Player->EquippedWeaponInstance = nullptr;
	Player->InactiveWeaponInstance = nullptr;
	InstanceA->Destroy();
	InstanceB->Destroy();
	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSwitchWeaponPreservesIndependentDecksTest,
	"DevKit.CombatDeck.PlayerSwitchWeaponPreservesIndependentDecks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerSwitchWeaponPreservesIndependentDecksTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for independent weapon deck test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for independent weapon deck test"), Player);
	if (!Player)
	{
		return false;
	}

	auto MakeAttackRune = [](UObject* Outer, const TCHAR* Name) -> URuneDataAsset*
	{
		URuneDataAsset* Rune = NewObject<URuneDataAsset>(Outer, FName(Name));
		Rune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
		Rune->RuneInfo.CombatCard.DisplayName = FText::FromString(Name);
		return Rune;
	};

	URuneDataAsset* WeaponACard = MakeAttackRune(Player, TEXT("WeaponACard"));
	URuneDataAsset* WeaponAReward = MakeAttackRune(Player, TEXT("WeaponAReward"));
	URuneDataAsset* WeaponBCard = MakeAttackRune(Player, TEXT("WeaponBCard"));
	URuneDataAsset* WeaponBReward = MakeAttackRune(Player, TEXT("WeaponBReward"));

	UWeaponDefinition* WeaponA = NewObject<UWeaponDefinition>(Player);
	UWeaponDefinition* WeaponB = NewObject<UWeaponDefinition>(Player);
	WeaponA->InitialCombatDeck = { WeaponACard };
	WeaponB->InitialCombatDeck = { WeaponBCard };

	Player->EquippedWeaponDef = WeaponA;
	Player->InactiveWeaponDef = WeaponB;
	Player->CombatDeckComponent->LoadDeckFromWeapon(WeaponA);
	TestTrue(TEXT("Weapon A can add a reward card to its own deck"),
		Player->CombatDeckComponent->AddCardFromRuneReward(WeaponAReward));

	Player->SwitchWeapon();
	TArray<URuneDataAsset*> WeaponBInitialSources = Player->CombatDeckComponent->GetDeckSourceAssets();
	TestTrue(TEXT("Switch to weapon B loads weapon B deck"),
		WeaponBInitialSources.Contains(WeaponBCard));
	TestFalse(TEXT("Switch to weapon B does not carry weapon A reward into weapon B deck"),
		WeaponBInitialSources.Contains(WeaponAReward));

	TestTrue(TEXT("Weapon B can add a reward card to its own deck"),
		Player->CombatDeckComponent->AddCardFromRuneReward(WeaponBReward));

	Player->SwitchWeapon();
	TArray<URuneDataAsset*> RestoredWeaponASources = Player->CombatDeckComponent->GetDeckSourceAssets();
	TestTrue(TEXT("Switch back to weapon A restores weapon A base card"),
		RestoredWeaponASources.Contains(WeaponACard));
	TestTrue(TEXT("Switch back to weapon A preserves weapon A reward"),
		RestoredWeaponASources.Contains(WeaponAReward));
	TestFalse(TEXT("Switch back to weapon A does not include weapon B reward"),
		RestoredWeaponASources.Contains(WeaponBReward));

	Player->SwitchWeapon();
	TArray<URuneDataAsset*> RestoredWeaponBSources = Player->CombatDeckComponent->GetDeckSourceAssets();
	TestTrue(TEXT("Switch back to weapon B restores weapon B base card"),
		RestoredWeaponBSources.Contains(WeaponBCard));
	TestTrue(TEXT("Switch back to weapon B preserves weapon B reward"),
		RestoredWeaponBSources.Contains(WeaponBReward));
	TestFalse(TEXT("Switch back to weapon B does not include weapon A reward"),
		RestoredWeaponBSources.Contains(WeaponAReward));

	Player->Destroy();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputBufferConsumesLatestAttackInputTest,
	"DevKit.CombatDeck.InputBufferConsumesLatestAttackInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputBufferConsumesLatestAttackInputTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for input buffer test"), World);
	if (!World)
	{
		return false;
	}

	AActor* OwnerActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Owner spawned for input buffer test"), OwnerActor);
	if (!OwnerActor)
	{
		return false;
	}

	UBufferComponent* Buffer = NewObject<UBufferComponent>(OwnerActor);
	TestNotNull(TEXT("Buffer component created"), Buffer);
	if (!Buffer)
	{
		OwnerActor->Destroy();
		return false;
	}

	OwnerActor->AddInstanceComponent(Buffer);
	Buffer->RegisterComponent();

	EInputCommandType ConsumedType = EInputCommandType::NormalAttack;
	const float SinceTime = World->GetTimeSeconds() - 1.0f;
	Buffer->RecordNormalAttack();
	Buffer->RecordSpecialAttack();
	TestTrue(TEXT("Latest attack input is consumed"), Buffer->ConsumeLatestAttackInputSince(SinceTime, ConsumedType));
	TestEqual(TEXT("SpecialAttack wins when it was recorded after NormalAttack"), ConsumedType, EInputCommandType::SpecialAttack);

	Buffer->ClearBuffer();
	Buffer->RecordSpecialAttack();
	Buffer->RecordNormalAttack();
	TestTrue(TEXT("Latest attack input is consumed after order swap"), Buffer->ConsumeLatestAttackInputSince(SinceTime, ConsumedType));
	TestEqual(TEXT("NormalAttack wins when it was recorded after SpecialAttack"), ConsumedType, EInputCommandType::NormalAttack);

	OwnerActor->Destroy();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphBuildsRuntimeWindowTest,
	"DevKit.CombatDeck.ComboGraphBuildsRuntimeWindowConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphBuildsRuntimeWindowTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraphNode* Node = NewObject<UGameplayAbilityComboGraphNode>();
	Node->NodeId = TEXT("L2H");
	Node->bUseNodeComboWindow = true;
	Node->ComboWindowStartFrame = 12;
	Node->ComboWindowEndFrame = 20;
	Node->TotalFrames = 30;

	const FWeaponComboNodeConfig RuntimeConfig = FWeaponComboNodeConfig::FromComboGraphNode(Node, ECardRequiredAction::Heavy);

	TestEqual(TEXT("Graph node exports its NodeId"), RuntimeConfig.NodeId, FName(TEXT("L2H")));
	TestEqual(TEXT("Graph edge input becomes runtime input"), RuntimeConfig.InputAction, ECardRequiredAction::Heavy);
	TestTrue(TEXT("Graph node frame window drives runtime combo windows"), RuntimeConfig.bOverrideComboWindow);
	TestEqual(TEXT("Combo window start frame is exported"), RuntimeConfig.ComboWindowStartFrame, 12);
	TestEqual(TEXT("Combo window end frame is exported"), RuntimeConfig.ComboWindowEndFrame, 20);
	TestEqual(TEXT("Combo window total frames is exported"), RuntimeConfig.ComboWindowTotalFrames, 30);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphSupportsNamedInputsTest,
	"DevKit.CombatDeck.ComboGraphSupportsNamedInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphSupportsNamedInputsTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>();
	UGameplayAbilityComboGraphNode* WeaponSkillRoot = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	WeaponSkillRoot->Graph = Graph;
	WeaponSkillRoot->NodeId = TEXT("WeaponSkillRoot");
	WeaponSkillRoot->RootInputAction = EYogComboGraphInputAction::WeaponSkill;
	WeaponSkillRoot->Montage = NewObject<UAnimMontage>(Graph);

	Graph->RootNodes = { WeaponSkillRoot };
	Graph->AllNodes = { WeaponSkillRoot };

	TestTrue(TEXT("WeaponSkill can be used as a combo graph root input"),
		Graph->FindRootComboNode(EYogComboGraphInputAction::WeaponSkill) == WeaponSkillRoot);
	TestNull(TEXT("WeaponSkill roots are not selected by NormalAttack input"),
		Graph->FindRootComboNode(EYogComboGraphInputAction::Light));

	const TArray<TSubclassOf<UGameplayAbilityComboGraphNode>> NodeClasses = Graph->GetSupportedNodeClasses();
	TestTrue(TEXT("Combo graph exposes its configured node class set"),
		NodeClasses.Contains(UGameplayAbilityComboGraphNode::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphLegacyDashInputRoutesToWeaponSkillTest,
	"DevKit.CombatDeck.ComboGraphLegacyDashInputRoutesToWeaponSkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphLegacyDashInputRoutesToWeaponSkillTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>();
	const EYogComboGraphInputAction LegacyDashInput = static_cast<EYogComboGraphInputAction>(5);

	UGameplayAbilityComboGraphNode* Root = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	Root->Graph = Graph;
	Root->NodeId = TEXT("DisarmDashRoot");
	Root->RootInputAction = LegacyDashInput;
	Root->Montage = NewObject<UAnimMontage>(Graph);

	UGameplayAbilityComboGraphNode* Child = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	Child->Graph = Graph;
	Child->NodeId = TEXT("DisarmDashFollowup");
	Child->Montage = NewObject<UAnimMontage>(Graph);

	UGameplayAbilityComboGraphEdge* Edge = NewObject<UGameplayAbilityComboGraphEdge>(Graph);
	Edge->InputAction = LegacyDashInput;
	Root->ChildrenNodes = { Child };
	Child->ParentNodes = { Root };
	Root->Edges.Add(Child, Edge);
	Graph->RootNodes = { Root };
	Graph->AllNodes = { Root, Child };

	TestTrue(TEXT("Legacy Dash root input routes to WeaponSkill"),
		Graph->FindRootComboNode(EYogComboGraphInputAction::WeaponSkill) == Root);
	TestNull(TEXT("Legacy Dash root input is not selected by NormalAttack"),
		Graph->FindRootComboNode(EYogComboGraphInputAction::Light));
	TestTrue(TEXT("Legacy Dash edge input routes to WeaponSkill"),
		Graph->FindChildComboNode(TEXT("DisarmDashRoot"), EYogComboGraphInputAction::WeaponSkill) == Child);
	TestNull(TEXT("Legacy Dash edge input is not selected by NormalAttack"),
		Graph->FindChildComboNode(TEXT("DisarmDashRoot"), EYogComboGraphInputAction::Light));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayAbilityComboGraphInvalidInputDoesNotMatchNormalAttackTest,
	"DevKit.CombatDeck.ComboGraphInvalidInputDoesNotMatchNormalAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayAbilityComboGraphInvalidInputDoesNotMatchNormalAttackTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>();
	const EYogComboGraphInputAction InvalidInput = static_cast<EYogComboGraphInputAction>(255);

	UGameplayAbilityComboGraphNode* Root = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	Root->Graph = Graph;
	Root->NodeId = TEXT("InvalidRoot");
	Root->RootInputAction = InvalidInput;
	Root->Montage = NewObject<UAnimMontage>(Graph);

	Graph->RootNodes = { Root };
	Graph->AllNodes = { Root };

	TestNull(TEXT("Invalid root input is not selected by NormalAttack"),
		Graph->FindRootComboNode(EYogComboGraphInputAction::Light));
	TestNull(TEXT("Invalid root input is not selected by WeaponSkill"),
		Graph->FindRootComboNode(EYogComboGraphInputAction::WeaponSkill));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpecialAttackDataAssetCarriesComboGraphTest,
	"DevKit.CombatDeck.SpecialAttackDataAssetCarriesComboGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpecialAttackDataAssetCarriesComboGraphTest::RunTest(const FString& Parameters)
{
	USpecialAttackDataAsset* SpecialAttack = NewObject<USpecialAttackDataAsset>();
	UGameplayAbilityComboGraph* SpecialAttackGraph = NewObject<UGameplayAbilityComboGraph>(SpecialAttack);
	SpecialAttack->Config.ComboGraph = SpecialAttackGraph;

	TestEqual(TEXT("Special attack data asset owns its combo graph reference"),
		SpecialAttack->Config.ComboGraph.Get(), SpecialAttackGraph);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerLoadsWeaponAndSpecialAttackComboGraphsTest,
	"DevKit.CombatDeck.PlayerLoadsWeaponAndSpecialAttackComboGraphs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerLoadsWeaponAndSpecialAttackComboGraphsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for dual combo graph load test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for dual combo graph load test"), Player);
	if (!Player)
	{
		return false;
	}

	UGameplayAbilityComboGraph* WeaponGraph = NewObject<UGameplayAbilityComboGraph>(Player);
	UGameplayAbilityComboGraph* SpecialAttackGraph = NewObject<UGameplayAbilityComboGraph>(Player);
	USpecialAttackDataAsset* SpecialAttack = NewObject<USpecialAttackDataAsset>(Player);
	SpecialAttack->Config.ComboGraph = SpecialAttackGraph;

	UWeaponDefinition* WeaponDef = NewObject<UWeaponDefinition>(Player);
	WeaponDef->GameplayAbilityComboGraph = WeaponGraph;
	WeaponDef->DefaultSpecialAttack = SpecialAttack;

	Player->ApplyComboGraphFromWeapon(WeaponDef);

	TestTrue(TEXT("Weapon combo graph is loaded as the normal weapon graph"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetWeaponComboGraph() == WeaponGraph);
	TestTrue(TEXT("Special attack combo graph is loaded separately from the weapon graph"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetSpecialAttackComboGraph() == SpecialAttackGraph);
	TestTrue(TEXT("The weapon graph remains the active runtime graph after equipment load"),
		Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->GetComboGraph() == WeaponGraph);

	Player->ResetToDefaultUnarmedCombatState();
	TestNull(TEXT("Reset to unarmed clears the equipped special attack graph"),
		Player->ComboRuntimeComponent ? Player->ComboRuntimeComponent->GetSpecialAttackComboGraph() : nullptr);

	Player->Destroy();
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
	Root->Montage = NewObject<UAnimMontage>(Graph);

	FirstChild->Graph = Graph;
	FirstChild->NodeId = TEXT("LightA");
	FirstChild->Montage = NewObject<UAnimMontage>(Graph);

	SecondChild->Graph = Graph;
	SecondChild->NodeId = TEXT("LightB");
	SecondChild->Montage = NewObject<UAnimMontage>(Graph);

	FirstEdge->InputAction = EYogComboGraphInputAction::Light;
	SecondEdge->InputAction = EYogComboGraphInputAction::Light;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponComboHintTextUnlimitedLinesTest,
	"DevKit.CombatDeck.WeaponComboHintTextUnlimitedLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponComboHintTextUnlimitedLinesTest::RunTest(const FString& Parameters)
{
	UGameplayAbilityComboGraph* Graph = NewObject<UGameplayAbilityComboGraph>();
	UWeaponDefinition* Weapon = NewObject<UWeaponDefinition>();
	Weapon->GameplayAbilityComboGraph = Graph;

	UGameplayAbilityComboGraphNode* LightRoot = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	UGameplayAbilityComboGraphNode* HeavyRoot = NewObject<UGameplayAbilityComboGraphNode>(Graph);
	LightRoot->Graph = Graph;
	LightRoot->NodeId = TEXT("LightRoot");
	LightRoot->RootInputAction = EYogComboGraphInputAction::Light;
	HeavyRoot->Graph = Graph;
	HeavyRoot->NodeId = TEXT("HeavyRoot");
	HeavyRoot->RootInputAction = EYogComboGraphInputAction::Heavy;

	auto AddChild = [Graph](UGameplayAbilityComboGraphNode* Parent, const TCHAR* NodeId, EYogComboGraphInputAction InputAction)
	{
		UGameplayAbilityComboGraphNode* Child = NewObject<UGameplayAbilityComboGraphNode>(Graph);
		UGameplayAbilityComboGraphEdge* Edge = NewObject<UGameplayAbilityComboGraphEdge>(Graph);
		Child->Graph = Graph;
		Child->NodeId = NodeId;
		Edge->InputAction = InputAction;
		Parent->ChildrenNodes.Add(Child);
		Child->ParentNodes.Add(Parent);
		Parent->Edges.Add(Child, Edge);
		Graph->AllNodes.Add(Child);
		return Child;
	};

	Graph->RootNodes = { LightRoot, HeavyRoot };
	Graph->AllNodes = { LightRoot, HeavyRoot };
	AddChild(LightRoot, TEXT("LL"), EYogComboGraphInputAction::Light);
	AddChild(LightRoot, TEXT("LH"), EYogComboGraphInputAction::Heavy);
	AddChild(AddChild(HeavyRoot, TEXT("HL"), EYogComboGraphInputAction::Light), TEXT("HLH"), EYogComboGraphInputAction::Heavy);
	AddChild(AddChild(HeavyRoot, TEXT("HH"), EYogComboGraphInputAction::Heavy), TEXT("HHL"), EYogComboGraphInputAction::Light);
	AddChild(AddChild(AddChild(HeavyRoot, TEXT("HLL"), EYogComboGraphInputAction::Light), TEXT("HLLL"), EYogComboGraphInputAction::Light), TEXT("HLLLH"), EYogComboGraphInputAction::Heavy);

	const FString LimitedText = WeaponComboTextUtils::BuildComboHintText(Weapon, 4, true).ToString();
	TestFalse(TEXT("Explicit line limit still truncates combo hint text"), LimitedText.Contains(TEXT("连段 05")));

	const FString UnlimitedText = WeaponComboTextUtils::BuildComboHintText(Weapon, 0, true).ToString();
	TestTrue(TEXT("Zero line limit means show every combo sequence"), UnlimitedText.Contains(TEXT("连段 05")));

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
	ActionContext.ActionSlot = ECombatDeckActionSlot::WeaponSkill;
	ActionContext.FlowRole = ECombatDeckFlowRole::Finisher;

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
	TestEqual(TEXT("Stored action slot"), StoredContext.ActionSlot, ECombatDeckActionSlot::WeaponSkill);
	TestEqual(TEXT("Stored flow role"), StoredContext.FlowRole, ECombatDeckFlowRole::Finisher);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActiveSkillConfigCombatDeckDefaultsTest,
	"DevKit.CombatDeck.ActiveSkillConfigCombatDeckDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActiveSkillConfigCombatDeckDefaultsTest::RunTest(const FString& Parameters)
{
	UActiveSkillDataAsset* ActiveSkill = NewObject<UActiveSkillDataAsset>();

	TestFalse(TEXT("Active skills do not resolve combat deck cards unless opted in"),
		ActiveSkill->Config.bResolveCombatDeckOnUse);
	TestEqual(TEXT("Active skill combat deck slot is skill"), ActiveSkill->Config.CombatDeckActionSlot, ECombatDeckActionSlot::Skill);
	TestEqual(TEXT("Active skill default flow role is catalyst"), ActiveSkill->Config.CombatDeckFlowRole, ECombatDeckFlowRole::Catalyst);
	TestEqual(TEXT("Active skill default card timing is OnCommit"), ActiveSkill->Config.CombatDeckTriggerTiming, ECombatCardTriggerTiming::OnCommit);

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

	FCombatDeckActionContext MoonlightContext;
	MoonlightContext.ActionType = ECardRequiredAction::Light;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Moonlight is consumed first"), MoonlightResult.bHadCard);
	TestTrue(TEXT("Moonlight opens a pending backward link"), MoonlightResult.bPendingBackwardLink);

	FCombatDeckActionContext AttackContext = MoonlightContext;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(AttackContext);
	TestTrue(TEXT("Next attack card is consumed"), AttackResult.bHadCard);
	TestTrue(TEXT("Next attack triggers Moonlight backward link"), AttackResult.bTriggeredBackwardLink);
	TestTrue(TEXT("Next attack still releases its own base flow"), AttackResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Backward link multiplier is reported"), AttackResult.AppliedMultiplier, 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightShadowReplaySourceCardTest,
	"DevKit.CombatDeck.MoonlightShadowReplayUsesLinkedSourceCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightShadowReplaySourceCardTest::RunTest(const FString& Parameters)
{
	FCombatCardInstance MoonlightCard;
	MoonlightCard.Config = FCombatCardConfig{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.Config.CardIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Moonlight"), false);

	FCombatCardInstance HeavyCard;
	HeavyCard.Config = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Heavy };
	HeavyCard.Config.CardIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Heavy"), false);

	FCombatCardResolveResult Result;
	Result.ConsumedCard = HeavyCard;
	Result.LinkedSourceCard = MoonlightCard;
	Result.LinkedTargetCard = HeavyCard;
	Result.bTriggeredBackwardLink = true;

	const FCombatCardInstance ReplaySourceCard = USacrificeRuneComponent::ResolveShadowReplaySourceCardForTest(Result);
	TestEqual(TEXT("Backward Moonlight replay uses the linked source card"), ReplaySourceCard.Config.CardIdTag, MoonlightCard.Config.CardIdTag);
	TestNotEqual(TEXT("Backward Moonlight replay does not use the consumed heavy card"), ReplaySourceCard.Config.CardIdTag, HeavyCard.Config.CardIdTag);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightShadowReplayDetectsProfileFlowsTest,
	"DevKit.CombatDeck.MoonlightShadowReplayDetectsProfileFlows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightShadowReplayDetectsProfileFlowsTest::RunTest(const FString& Parameters)
{
	UFlowAsset* ForwardAttackFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Attack.FA_Rune512_Moonlight_Forward_Attack"));
	UFlowAsset* ReversedAttackFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Attack.FA_Rune512_Moonlight_Reversed_Attack"));

	TestNotNull(TEXT("Moonlight forward attack flow exists"), ForwardAttackFlow);
	TestNotNull(TEXT("Moonlight reversed attack flow exists"), ReversedAttackFlow);
	if (!ForwardAttackFlow || !ReversedAttackFlow)
	{
		return false;
	}

	TestTrue(TEXT("Forward attack profile flow is replayable from the shadow"),
		USacrificeRuneComponent::FlowHasOffensiveSpawnNodeForTest(ForwardAttackFlow));
	TestTrue(TEXT("Reversed attack profile flow is replayable from the shadow"),
		USacrificeRuneComponent::FlowHasOffensiveSpawnNodeForTest(ReversedAttackFlow));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkConfigBackwardCanStartFromFirstAttackTest,
	"DevKit.CombatDeck.LinkConfigBackwardCanStartFromFirstAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkConfigBackwardCanStartFromFirstAttackTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.LinkConfig.Direction = ECombatCardLinkDirection::BackwardEmpowerNext;
	MoonlightCard.LinkConfig.BackwardEffect.Flow = NewObject<UFlowAsset>();
	MoonlightCard.LinkConfig.BackwardEffect.Condition.RequiredNeighborTypes.Add(ECombatCardType::Attack);

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };

	Deck->SetDeckListForTest({ MoonlightCard, AttackCard });

	FCombatDeckActionContext MoonlightContext;
	MoonlightContext.ActionType = ECardRequiredAction::Light;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bComboContinued = false;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("First attack Moonlight opens a pending backward link"), MoonlightResult.bPendingBackwardLink);

	FCombatDeckActionContext AttackContext = MoonlightContext;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();
	AttackContext.bComboContinued = true;
	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(AttackContext);
	TestTrue(TEXT("Second attack triggers the pending backward link"), AttackResult.bTriggeredBackwardLink);

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

	FCombatDeckActionContext MoonlightContext;
	MoonlightContext.ActionType = ECardRequiredAction::Light;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link"), MoonlightResult.bPendingBackwardLink);
	TestFalse(TEXT("Reversed Moonlight does not release base flow while pending"), MoonlightResult.bTriggeredBaseFlow);

	FCombatDeckActionContext AttackContext = MoonlightContext;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(AttackContext);
	TestTrue(TEXT("Next Card.Effect.Attack triggers reversed Moonlight recipe"), AttackResult.bTriggeredBackwardLink);
	TestEqual(TEXT("Reversed recipe multiplier is reported"), AttackResult.AppliedMultiplier, 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedCanStartFromFirstAttackTest,
	"DevKit.CombatDeck.RecipeReversedCanStartFromFirstAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedCanStartFromFirstAttackTest::RunTest(const FString& Parameters)
{
	const FGameplayTag SplashSplitEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.SplashSplit"));

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.DefaultLinkOrientation = ECombatCardLinkOrientation::Reversed;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Reversed;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Condition.RequiredNeighborEffectTags.AddTag(SplashSplitEffectTag);
	MoonlightCard.LinkRecipes.Add(Recipe);

	FCombatCardConfig SplashCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	SplashCard.CardEffectTags.AddTag(SplashSplitEffectTag);

	Deck->SetDeckListForTest({ MoonlightCard, SplashCard });

	FCombatDeckActionContext MoonlightContext;
	MoonlightContext.ActionType = ECardRequiredAction::Light;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bComboContinued = false;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("First attack reversed Moonlight opens a pending recipe link"), MoonlightResult.bPendingBackwardLink);
	TestFalse(TEXT("First attack reversed Moonlight does not release base flow while pending"), MoonlightResult.bTriggeredBaseFlow);

	FCombatDeckActionContext SplashContext = MoonlightContext;
	SplashContext.AttackInstanceGuid = FGuid::NewGuid();
	SplashContext.bComboContinued = true;
	const FCombatCardResolveResult SplashResult = Deck->ResolveAttackCardWithContext(SplashContext);
	TestTrue(TEXT("Second attack Splash/Split triggers the pending Moonlight recipe"), SplashResult.bTriggeredBackwardLink);

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

	FCombatDeckActionContext ReversedMoonlightContext;
	ReversedMoonlightContext.ActionType = ECardRequiredAction::Light;
	ReversedMoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	ReversedMoonlightContext.bComboContinued = true;
	ReversedMoonlightContext.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult PendingResult = ReversedDeck->ResolveAttackCardWithContext(ReversedMoonlightContext);
	TestTrue(TEXT("Reversed Moonlight opens pending link with the matching recipe list"), PendingResult.bPendingBackwardLink);
	TestFalse(TEXT("Reversed Moonlight does not release base flow while waiting for the next card"), PendingResult.bTriggeredBaseFlow);

	FCombatDeckActionContext ReversedShieldContext = ReversedMoonlightContext;
	ReversedShieldContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult ReversedResult = ReversedDeck->ResolveAttackCardWithContext(ReversedShieldContext);
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

	FCombatDeckActionContext MoonlightContext;
	MoonlightContext.ActionType = ECardRequiredAction::Light;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckFullSnapshotFiltersDeprecatedFinisherTest,
	"DevKit.CombatDeck.FullSnapshotFiltersDeprecatedFinisher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckFullSnapshotFiltersDeprecatedFinisherTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->TemporaryFinisherUnlockCompletedBattles = 3;

	FCombatCardConfig FinisherCard{ ECombatCardType::Finisher, ECardRequiredAction::Any };
	FinisherCard.DisplayName = FText::FromString(TEXT("Finisher"));

	Deck->SetDeckListForTest({ FinisherCard });

	const TArray<FCombatCardInstance> Cards = Deck->GetFullDeckSnapshot();
	TestEqual(TEXT("Full deck snapshot filters deprecated finisher cards"), Cards.Num(), 0);

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

	const TArray<FName> ForwardEffectTagNames = {
		TEXT("Card.Effect.Burn"),
		TEXT("Card.Effect.Poison"),
		TEXT("Card.Effect.Shield"),
		TEXT("Card.Effect.Pierce"),
		TEXT("Card.Effect.Attack"),
		TEXT("Card.Effect.Defense.ReduceDamage"),
	};
	TArray<FName> ReversedEffectTagNames = ForwardEffectTagNames;
	ReversedEffectTagNames.Add(TEXT("Card.Effect.SplashSplit"));

	const FCombatCardConfig& ForwardMoonlightConfig = MoonlightForwardDA->RuneInfo.CombatCard;
	const FCombatCardConfig& ReversedMoonlightConfig = MoonlightReversedDA->RuneInfo.CombatCard;
	TestEqual(TEXT("Forward Moonlight has forward recipes and one reversed Splash/Split family recipe"), ForwardMoonlightConfig.LinkRecipes.Num(), 13);
	TestEqual(TEXT("Reversed Moonlight has forward recipes and one reversed Splash/Split family recipe"), ReversedMoonlightConfig.LinkRecipes.Num(), 13);

	for (const FName& EffectTagName : ForwardEffectTagNames)
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

	for (const FName& EffectTagName : ReversedEffectTagNames)
	{
		const FGameplayTag EffectTag = FGameplayTag::RequestGameplayTag(EffectTagName, false);
		TestTrue(FString::Printf(TEXT("Reversed effect tag exists: %s"), *EffectTagName.ToString()), EffectTag.IsValid());
		if (!EffectTag.IsValid())
		{
			continue;
		}

		FCombatCardConfig NeighborCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
		NeighborCard.CardEffectTags.AddTag(EffectTag);

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
	TestEqual(TEXT("Moonlight base combo cone angle"), SlashNode->ProjectileConeAngleDegrees, 0.f);
	TestTrue(TEXT("Moonlight base fires combo projectiles sequentially"), SlashNode->bSpawnProjectilesSequentially);
	TestEqual(TEXT("Moonlight base sequential projectile interval"), SlashNode->SequentialProjectileSpawnInterval, 0.12f);

	auto ValidateForwardMoonlightComboProjectiles = [this](const TCHAR* FlowPath, const TCHAR* Label, const bool bShouldUseComboProjectiles) -> bool
	{
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		UBFNode_SpawnSlashWaveProjectile* FlowSlashNode = nullptr;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			FlowSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			if (FlowSlashNode)
			{
				break;
			}
		}

		TestNotNull(FString::Printf(TEXT("%s has slash-wave node"), Label), FlowSlashNode);
		if (!FlowSlashNode)
		{
			return false;
		}

		if (bShouldUseComboProjectiles)
		{
			TestEqual(FString::Printf(TEXT("%s base projectile count"), Label), FlowSlashNode->ProjectileCount, 1);
			TestTrue(FString::Printf(TEXT("%s adds combo stacks to projectile count"), Label), FlowSlashNode->bAddComboStacksToProjectileCount);
			TestEqual(FString::Printf(TEXT("%s projectiles per combo stack"), Label), FlowSlashNode->ProjectilesPerComboStack, 1);
			TestEqual(FString::Printf(TEXT("%s max bonus projectiles"), Label), FlowSlashNode->MaxBonusProjectiles, 2);
			TestEqual(FString::Printf(TEXT("%s keeps one path"), Label), FlowSlashNode->ProjectileConeAngleDegrees, 0.f);
			TestTrue(FString::Printf(TEXT("%s fires combo projectiles sequentially"), Label), FlowSlashNode->bSpawnProjectilesSequentially);
			TestEqual(FString::Printf(TEXT("%s sequential projectile interval"), Label), FlowSlashNode->SequentialProjectileSpawnInterval, 0.12f);
		}
		else
		{
			TestFalse(FString::Printf(TEXT("%s does not use combo projectile count"), Label), FlowSlashNode->bAddComboStacksToProjectileCount);
			TestFalse(FString::Printf(TEXT("%s does not use sequential combo projectiles"), Label), FlowSlashNode->bSpawnProjectilesSequentially);
		}

		return true;
	};

	bool bForwardMoonlightComboConfigValid = true;
	bForwardMoonlightComboConfigValid &= ValidateForwardMoonlightComboProjectiles(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Attack.FA_Rune512_Moonlight_Forward_Attack"),
		TEXT("Moonlight forward attack"),
		true);
	bForwardMoonlightComboConfigValid &= ValidateForwardMoonlightComboProjectiles(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Burn.FA_Rune512_Moonlight_Forward_Burn"),
		TEXT("Moonlight forward burn"),
		true);
	bForwardMoonlightComboConfigValid &= ValidateForwardMoonlightComboProjectiles(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Poison.FA_Rune512_Moonlight_Forward_Poison"),
		TEXT("Moonlight forward poison"),
		true);
	bForwardMoonlightComboConfigValid &= ValidateForwardMoonlightComboProjectiles(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Pierce.FA_Rune512_Moonlight_Forward_Pierce"),
		TEXT("Moonlight forward pierce"),
		true);
	bForwardMoonlightComboConfigValid &= ValidateForwardMoonlightComboProjectiles(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Shield.FA_Rune512_Moonlight_Forward_Shield"),
		TEXT("Moonlight forward shield"),
		true);

	UFlowAsset* MoonlightForwardShieldFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Shield.FA_Rune512_Moonlight_Forward_Shield"));
	TestNotNull(TEXT("Moonlight forward shield flow exists for targeted bounce tuning"), MoonlightForwardShieldFlow);
	if (MoonlightForwardShieldFlow)
	{
		UBFNode_SpawnSlashWaveProjectile* ShieldSlashNode = nullptr;
		for (const TPair<FGuid, UFlowNode*>& Pair : MoonlightForwardShieldFlow->GetNodes())
		{
			ShieldSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			if (ShieldSlashNode)
			{
				break;
			}
		}

		TestNotNull(TEXT("Moonlight forward shield has slash-wave node"), ShieldSlashNode);
		if (ShieldSlashNode)
		{
			TestTrue(TEXT("Moonlight forward shield enables targeted bounce"), ShieldSlashNode->bEnableTargetedBounce);
			TestEqual(TEXT("Moonlight forward shield targeted bounce max count"), ShieldSlashNode->TargetedBounceMaxCount, 5);
			TestEqual(TEXT("Moonlight forward shield targeted bounce search radius"), ShieldSlashNode->TargetedBounceSearchRadius, 650.f);
			TestEqual(TEXT("Moonlight forward shield targeted bounce travel range"), ShieldSlashNode->TargetedBounceMaxTravelDistance, 650.f);
			TestTrue(TEXT("Moonlight forward shield uses base moonlight collision X"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->CollisionBoxExtent.X), 30.f));
			TestTrue(TEXT("Moonlight forward shield uses base moonlight collision Y"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->CollisionBoxExtent.Y), 60.f));
			TestTrue(TEXT("Moonlight forward shield uses base moonlight collision Z"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->CollisionBoxExtent.Z), 35.f));
			TestTrue(TEXT("Moonlight forward shield visual scale X"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->VisualScaleMultiplier.X), 0.85f));
			TestEqual(TEXT("Moonlight forward shield can hit initial target plus five bounces"), ShieldSlashNode->MaxHitCount, 6);
		}
	}

	URuneCardEffectProfileDA* MoonlightForwardShieldProfile = LoadObject<URuneCardEffectProfileDA>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Shield.EP_Rune512_Moonlight_Forward_Shield"));
	TestNotNull(TEXT("Moonlight forward shield effect profile exists"), MoonlightForwardShieldProfile);
	if (MoonlightForwardShieldProfile)
	{
		const FRuneCardProfileProjectileConfig& Projectile = MoonlightForwardShieldProfile->Projectile;
		TestTrue(TEXT("Moonlight forward shield profile enables targeted bounce"), Projectile.bEnableTargetedBounce);
		TestEqual(TEXT("Moonlight forward shield profile targeted bounce max count"), Projectile.TargetedBounceMaxCount, 5);
		TestEqual(TEXT("Moonlight forward shield profile targeted bounce search radius"), Projectile.TargetedBounceSearchRadius, 650.f);
		TestEqual(TEXT("Moonlight forward shield profile targeted bounce travel range"), Projectile.TargetedBounceMaxTravelDistance, 650.f);
		TestTrue(TEXT("Moonlight forward shield profile collision X"), FMath::IsNearlyEqual(static_cast<float>(Projectile.CollisionBoxExtent.X), 30.f));
		TestEqual(TEXT("Moonlight forward shield profile max hit count"), Projectile.MaxHitCount, 6);
	}

	bForwardMoonlightComboConfigValid &= ValidateForwardMoonlightComboProjectiles(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_ReduceDamage.FA_Rune512_Moonlight_Forward_ReduceDamage"),
		TEXT("Moonlight forward reduce damage"),
		true);
	return bForwardMoonlightComboConfigValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightForwardShieldTargetedBounceConfiguredTest,
	"DevKit.CombatDeck.MoonlightForwardShieldTargetedBounceConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightForwardShieldTargetedBounceConfiguredTest::RunTest(const FString& Parameters)
{
	UFlowAsset* MoonlightForwardShieldFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Shield.FA_Rune512_Moonlight_Forward_Shield"));
	TestNotNull(TEXT("Moonlight forward shield flow exists for targeted bounce tuning"), MoonlightForwardShieldFlow);
	if (!MoonlightForwardShieldFlow)
	{
		return false;
	}

	UBFNode_SpawnSlashWaveProjectile* ShieldSlashNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : MoonlightForwardShieldFlow->GetNodes())
	{
		ShieldSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
		if (ShieldSlashNode)
		{
			break;
		}
	}

	TestNotNull(TEXT("Moonlight forward shield has slash-wave node"), ShieldSlashNode);
	if (!ShieldSlashNode)
	{
		return false;
	}

	TestTrue(TEXT("Moonlight forward shield enables targeted bounce"), ShieldSlashNode->bEnableTargetedBounce);
	TestEqual(TEXT("Moonlight forward shield targeted bounce max count"), ShieldSlashNode->TargetedBounceMaxCount, 5);
	TestEqual(TEXT("Moonlight forward shield targeted bounce search radius"), ShieldSlashNode->TargetedBounceSearchRadius, 650.f);
	TestEqual(TEXT("Moonlight forward shield targeted bounce travel range"), ShieldSlashNode->TargetedBounceMaxTravelDistance, 650.f);
	TestTrue(TEXT("Moonlight forward shield uses base moonlight collision X"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->CollisionBoxExtent.X), 30.f));
	TestTrue(TEXT("Moonlight forward shield uses base moonlight collision Y"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->CollisionBoxExtent.Y), 60.f));
	TestTrue(TEXT("Moonlight forward shield uses base moonlight collision Z"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->CollisionBoxExtent.Z), 35.f));
	TestTrue(TEXT("Moonlight forward shield visual scale X"), FMath::IsNearlyEqual(static_cast<float>(ShieldSlashNode->VisualScaleMultiplier.X), 0.85f));
	TestEqual(TEXT("Moonlight forward shield can hit initial target plus five bounces"), ShieldSlashNode->MaxHitCount, 6);

	URuneCardEffectProfileDA* MoonlightForwardShieldProfile = LoadObject<URuneCardEffectProfileDA>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile/EP_Rune512_Moonlight_Forward_Shield.EP_Rune512_Moonlight_Forward_Shield"));
	TestNotNull(TEXT("Moonlight forward shield effect profile exists"), MoonlightForwardShieldProfile);
	if (!MoonlightForwardShieldProfile)
	{
		return false;
	}

	const FRuneCardProfileProjectileConfig& Projectile = MoonlightForwardShieldProfile->Projectile;
	TestTrue(TEXT("Moonlight forward shield profile enables targeted bounce"), Projectile.bEnableTargetedBounce);
	TestEqual(TEXT("Moonlight forward shield profile targeted bounce max count"), Projectile.TargetedBounceMaxCount, 5);
	TestEqual(TEXT("Moonlight forward shield profile targeted bounce search radius"), Projectile.TargetedBounceSearchRadius, 650.f);
	TestEqual(TEXT("Moonlight forward shield profile targeted bounce travel range"), Projectile.TargetedBounceMaxTravelDistance, 650.f);
	TestTrue(TEXT("Moonlight forward shield profile collision X"), FMath::IsNearlyEqual(static_cast<float>(Projectile.CollisionBoxExtent.X), 30.f));
	TestEqual(TEXT("Moonlight forward shield profile max hit count"), Projectile.MaxHitCount, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedSplashSplitConfiguredTest,
	"DevKit.CombatDeck.GeneratedSplashSplitConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedSplashSplitConfiguredTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* SplashDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Splash.DA_Rune512_Splash"));
	URuneDataAsset* SplitDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split.DA_Rune512_Split"));
	TestNotNull(TEXT("Generated splash DA exists"), SplashDA);
	TestNotNull(TEXT("Generated split DA exists"), SplitDA);
	if (!SplashDA || !SplitDA)
	{
		return false;
	}

	const FGameplayTag SplashEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Splash"), false);
	const FGameplayTag SplitEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Split"), false);
	const FGameplayTag SplashSplitIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.SplashSplit"), false);
	const FGameplayTag SplashSplitEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.SplashSplit"), false);
	TestTrue(TEXT("Card.Effect.Splash tag exists"), SplashEffectTag.IsValid());
	TestTrue(TEXT("Card.Effect.Split tag exists"), SplitEffectTag.IsValid());
	TestTrue(TEXT("Card.ID.SplashSplit tag exists"), SplashSplitIdTag.IsValid());
	TestTrue(TEXT("Card.Effect.SplashSplit tag exists"), SplashSplitEffectTag.IsValid());

	const FCombatCardConfig& SplashCard = SplashDA->RuneInfo.CombatCard;
	TestEqual(TEXT("Splash card is normal"), SplashCard.CardType, ECombatCardType::Normal);
	TestEqual(TEXT("Splash card triggers on hit"), SplashCard.TriggerTiming, ECombatCardTriggerTiming::OnHit);
	TestEqual(TEXT("Splash card uses shared Splash/Split id"), SplashCard.CardIdTag, SplashSplitIdTag);
	TestTrue(TEXT("Splash card has shared Splash/Split effect tag"), SplashCard.CardEffectTags.HasTagExact(SplashSplitEffectTag));
	TestTrue(TEXT("Splash card has splash effect tag"), SplashCard.CardEffectTags.HasTagExact(SplashEffectTag));

	const FCombatCardConfig& SplitCard = SplitDA->RuneInfo.CombatCard;
	TestEqual(TEXT("Split card is normal"), SplitCard.CardType, ECombatCardType::Normal);
	TestEqual(TEXT("Split card triggers on commit"), SplitCard.TriggerTiming, ECombatCardTriggerTiming::OnCommit);
	TestEqual(TEXT("Split card uses shared Splash/Split id"), SplitCard.CardIdTag, SplashSplitIdTag);
	TestTrue(TEXT("Split card has shared Splash/Split effect tag"), SplitCard.CardEffectTags.HasTagExact(SplashSplitEffectTag));
	TestTrue(TEXT("Split card has split effect tag"), SplitCard.CardEffectTags.HasTagExact(SplitEffectTag));

	UFlowAsset* SplashFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Splash_Base.FA_Rune512_Splash_Base"));
	UFlowAsset* SplitFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Split_Base.FA_Rune512_Split_Base"));
	TestNotNull(TEXT("Generated splash base flow exists"), SplashFlow);
	TestNotNull(TEXT("Generated split base flow exists"), SplitFlow);
	if (!SplashFlow || !SplitFlow)
	{
		return false;
	}

	UBFNode_OnDamageDealt* SplashOnDamageNode = nullptr;
	UBFNode_MathFloat* SplashMathNode = nullptr;
	UBFNode_ApplyGEInRadius* SplashRadiusNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : SplashFlow->GetNodes())
	{
		if (!SplashOnDamageNode)
		{
			SplashOnDamageNode = Cast<UBFNode_OnDamageDealt>(Pair.Value);
		}
		if (!SplashMathNode)
		{
			SplashMathNode = Cast<UBFNode_MathFloat>(Pair.Value);
		}
		if (!SplashRadiusNode)
		{
			SplashRadiusNode = Cast<UBFNode_ApplyGEInRadius>(Pair.Value);
		}
	}
	TestNotNull(TEXT("Splash flow has OnDamageDealt"), SplashOnDamageNode);
	TestNotNull(TEXT("Splash flow has MathFloat"), SplashMathNode);
	TestNotNull(TEXT("Splash flow has ApplyGEInRadius"), SplashRadiusNode);
	if (SplashMathNode)
	{
		TestEqual(TEXT("Splash math uses multiply"), SplashMathNode->Operator, EBFMathOp::Multiply);
		TestEqual(TEXT("Splash math multiplier is 20 percent"), SplashMathNode->B.Value, 0.2f);
	}
	if (SplashRadiusNode)
	{
		TestEqual(TEXT("Splash radius is 300"), SplashRadiusNode->Radius.Value, 300.f);
		TestTrue(TEXT("Splash excludes primary target"), SplashRadiusNode->bExcludeLocationSourceActor);
		TestTrue(TEXT("Splash is enemy only"), SplashRadiusNode->bEnemyOnly);
		TestEqual(TEXT("Splash applies once per target"), SplashRadiusNode->ApplicationCount, 1);
	}

	UBFNode_SpawnRangedProjectiles* SplitProjectileNode = nullptr;
	bool bSplitFlowHasSlashWave = false;
	for (const TPair<FGuid, UFlowNode*>& Pair : SplitFlow->GetNodes())
	{
		if (!SplitProjectileNode)
		{
			SplitProjectileNode = Cast<UBFNode_SpawnRangedProjectiles>(Pair.Value);
		}
		bSplitFlowHasSlashWave |= Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value) != nullptr;
	}
	TestNotNull(TEXT("Split flow has Spawn Ranged Projectiles"), SplitProjectileNode);
	TestFalse(TEXT("Split flow does not spawn moonblade slash waves"), bSplitFlowHasSlashWave);
	if (SplitProjectileNode)
	{
		TestEqual(TEXT("Split spawns two extra projectiles"), SplitProjectileNode->YawOffsets.Num(), 2);
		if (SplitProjectileNode->YawOffsets.Num() == 2)
		{
			TestEqual(TEXT("Split left projectile angle"), SplitProjectileNode->YawOffsets[0], -8.f);
			TestEqual(TEXT("Split right projectile angle"), SplitProjectileNode->YawOffsets[1], 8.f);
		}
		TestTrue(TEXT("Split uses combat card attack damage"), SplitProjectileNode->bUseCombatCardAttackDamage);
		TestTrue(TEXT("Split projectiles share attack instance guid"), SplitProjectileNode->bShareAttackInstanceGuid);
		TestTrue(TEXT("Split projectiles require ranged weapon tag"), SplitProjectileNode->bRequireRangedWeaponTag);
		TestEqual(
			TEXT("Split projectile required weapon tag"),
			SplitProjectileNode->RequiredWeaponTag,
			FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false));
	}

	UFlowAsset* MoonlightReversedSplitFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Split.FA_Rune512_Moonlight_Reversed_Split"));
	TestNotNull(TEXT("Moonlight reversed split flow exists"), MoonlightReversedSplitFlow);
	if (MoonlightReversedSplitFlow)
	{
		UBFNode_SpawnSlashWaveProjectile* ReversedSplitSlashNode = nullptr;
		for (const TPair<FGuid, UFlowNode*>& Pair : MoonlightReversedSplitFlow->GetNodes())
		{
			ReversedSplitSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			if (ReversedSplitSlashNode)
			{
				break;
			}
		}
		TestNotNull(TEXT("Moonlight reversed split uses slash wave"), ReversedSplitSlashNode);
		if (ReversedSplitSlashNode)
		{
			TestTrue(TEXT("Moonlight reversed split splits on first impact"), ReversedSplitSlashNode->bSplitOnFirstHit);
			TestTrue(TEXT("Moonlight reversed split reacts to world collision"), ReversedSplitSlashNode->bDestroyOnWorldStaticHit);
			TestEqual(TEXT("Moonlight reversed split child count"), ReversedSplitSlashNode->SplitProjectileCount, 4);
			TestEqual(TEXT("Moonlight reversed split max generation"), ReversedSplitSlashNode->MaxSplitGenerations, 1);
			TestEqual(TEXT("Moonlight reversed split cone"), ReversedSplitSlashNode->SplitConeAngleDegrees, 100.f);
			TestTrue(TEXT("Moonlight reversed split randomizes child directions"), ReversedSplitSlashNode->bRandomizeSplitDirections);
			TestEqual(TEXT("Moonlight reversed split yaw jitter"), ReversedSplitSlashNode->SplitRandomYawJitterDegrees, 22.f);
			TestEqual(TEXT("Moonlight reversed split pitch jitter"), ReversedSplitSlashNode->SplitRandomPitchDegrees, 0.f);
			TestEqual(TEXT("Moonlight reversed split child distance multiplier"), ReversedSplitSlashNode->SplitMaxDistanceMultiplier, 1.25f);
			TestTrue(TEXT("Moonlight reversed split child bounces on enemy hit"), ReversedSplitSlashNode->bBounceSplitChildrenOnEnemyHit);
			TestEqual(TEXT("Moonlight reversed split child max enemy bounces"), ReversedSplitSlashNode->SplitChildMaxEnemyBounces, 1);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedSacrificePassivesConfiguredTest,
	"DevKit.CombatDeck.GeneratedSacrificePassivesConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedSacrificePassivesConfiguredTest::RunTest(const FString& Parameters)
{
	struct FExpectedSacrificePassive
	{
		const TCHAR* Key;
		ESacrificeRunePassiveType PassiveType;
	};

	const FExpectedSacrificePassive ExpectedPassives[] = {
		{ TEXT("MoonlightShadow"), ESacrificeRunePassiveType::MoonlightShadow },
		{ TEXT("ShadowMark"), ESacrificeRunePassiveType::ShadowMark },
		{ TEXT("GiantSwing"), ESacrificeRunePassiveType::GiantSwing },
	};

	bool bAllValid = true;
	for (const FExpectedSacrificePassive& ExpectedPassive : ExpectedPassives)
	{
		const FString Key(ExpectedPassive.Key);
		const FString DAPath = FString::Printf(
			TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_%s.DA_Rune512_Sacrifice_%s"),
			*Key,
			*Key);
		const FString FlowPath = FString::Printf(
			TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/Flow/FA_Rune512_Sacrifice_%s.FA_Rune512_Sacrifice_%s"),
			*Key,
			*Key);

		URuneDataAsset* RuneDA = LoadObject<URuneDataAsset>(nullptr, *DAPath);
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, *FlowPath);
		bAllValid &= TestNotNull(FString::Printf(TEXT("Sacrifice %s DA exists"), *Key), RuneDA);
		bAllValid &= TestNotNull(FString::Printf(TEXT("Sacrifice %s Flow exists"), *Key), Flow);
		if (!RuneDA || !Flow)
		{
			continue;
		}

		bAllValid &= TestEqual(FString::Printf(TEXT("%s is passive trigger"), *Key), RuneDA->GetTriggerType(), ERuneTriggerType::Passive);
		bAllValid &= TestEqual(FString::Printf(TEXT("%s has empty shape"), *Key), RuneDA->RuneInfo.Shape.Cells.Num(), 0);
		bAllValid &= TestFalse(FString::Printf(TEXT("%s does not enter combat deck"), *Key), RuneDA->RuneInfo.CombatCard.bIsCombatCard);
		bAllValid &= TestEqual(FString::Printf(TEXT("%s has no link recipes"), *Key), RuneDA->RuneInfo.CombatCard.LinkRecipes.Num(), 0);
		bAllValid &= TestTrue(FString::Printf(TEXT("%s RuneInfo uses sacrifice flow"), *Key), RuneDA->RuneInfo.Flow.FlowAsset.Get() == Flow);

		int32 GrantNodeCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (const UBFNode_GrantSacrificePassive* GrantNode = Cast<UBFNode_GrantSacrificePassive>(Pair.Value))
			{
				++GrantNodeCount;
				bAllValid &= TestEqual(FString::Printf(TEXT("%s grant node passive type"), *Key), GrantNode->Config.PassiveType, ExpectedPassive.PassiveType);
			}
			else if (Pair.Value != Flow->GetDefaultEntryNode())
			{
				bAllValid &= TestTrue(FString::Printf(TEXT("%s flow only contains entry plus grant node"), *Key), false);
			}
		}
		bAllValid &= TestEqual(FString::Printf(TEXT("%s has one grant node"), *Key), GrantNodeCount, 1);
	}

	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedGenericStatusCardsConfiguredTest,
	"DevKit.CombatDeck.GeneratedGenericStatusCardsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedGenericStatusCardsConfiguredTest::RunTest(const FString& Parameters)
{
	struct FExpectedStatusCard
	{
		const TCHAR* Key;
		const TCHAR* CardIdTag;
		const TCHAR* CardEffectTag;
	};

	const FExpectedStatusCard ExpectedCards[] = {
		{ TEXT("Burn"), TEXT("Card.ID.Burn"), TEXT("Card.Effect.Burn") },
		{ TEXT("Poison"), TEXT("Card.ID.Poison"), TEXT("Card.Effect.Poison") },
		{ TEXT("Bleed"), TEXT("Card.ID.Bleed"), TEXT("Card.Effect.Bleed") },
		{ TEXT("Rend"), TEXT("Card.ID.Rend"), TEXT("Card.Effect.Rend") },
		{ TEXT("Wound"), TEXT("Card.ID.Wound"), TEXT("Card.Effect.Wound") },
		{ TEXT("Knockback"), TEXT("Card.ID.Knockback"), TEXT("Card.Effect.Knockback") },
		{ TEXT("Fear"), TEXT("Card.ID.Fear"), TEXT("Card.Effect.Fear") },
		{ TEXT("Freeze"), TEXT("Card.ID.Freeze"), TEXT("Card.Effect.Freeze") },
		{ TEXT("Stun"), TEXT("Card.ID.Stun"), TEXT("Card.Effect.Stun") },
		{ TEXT("Curse"), TEXT("Card.ID.Curse"), TEXT("Card.Effect.Curse") },
	};

	bool bAllValid = true;
	for (const FExpectedStatusCard& ExpectedCard : ExpectedCards)
	{
		const FString Key(ExpectedCard.Key);
		const FString DAPath = FString::Printf(
			TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_%s.DA_Rune512_%s"),
			*Key,
			*Key);
		URuneDataAsset* CardDA = LoadObject<URuneDataAsset>(nullptr, *DAPath);
		bAllValid &= TestNotNull(FString::Printf(TEXT("Generated %s DA exists"), *Key), CardDA);
		if (!CardDA)
		{
			continue;
		}

		const FCombatCardConfig& CardConfig = CardDA->RuneInfo.CombatCard;
		const FGameplayTag ExpectedIdTag = FGameplayTag::RequestGameplayTag(FName(ExpectedCard.CardIdTag), false);
		const FGameplayTag ExpectedEffectTag = FGameplayTag::RequestGameplayTag(FName(ExpectedCard.CardEffectTag), false);

		bAllValid &= TestTrue(FString::Printf(TEXT("%s is combat card"), *Key), CardConfig.bIsCombatCard);
		bAllValid &= TestEqual(FString::Printf(TEXT("%s is normal card"), *Key), CardConfig.CardType, ECombatCardType::Normal);
		bAllValid &= TestEqual(FString::Printf(TEXT("%s triggers on hit"), *Key), CardConfig.TriggerTiming, ECombatCardTriggerTiming::OnHit);
		bAllValid &= TestTrue(FString::Printf(TEXT("%s CardIdTag exists"), *Key), ExpectedIdTag.IsValid());
		bAllValid &= TestTrue(FString::Printf(TEXT("%s CardEffectTag exists"), *Key), ExpectedEffectTag.IsValid());
		bAllValid &= TestEqual(FString::Printf(TEXT("%s CardIdTag configured"), *Key), CardConfig.CardIdTag, ExpectedIdTag);
		bAllValid &= TestTrue(FString::Printf(TEXT("%s effect tag configured"), *Key), CardConfig.CardEffectTags.HasTagExact(ExpectedEffectTag));

		const FString FlowPath = FString::Printf(
			TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_%s_Base.FA_Rune512_%s_Base"),
			*Key,
			*Key);
		UFlowAsset* ExpectedFlow = LoadObject<UFlowAsset>(nullptr, *FlowPath);
		bAllValid &= TestNotNull(FString::Printf(TEXT("Generated %s BaseFlow exists"), *Key), ExpectedFlow);
		bAllValid &= TestTrue(FString::Printf(TEXT("%s DA uses generated BaseFlow"), *Key), CardConfig.BaseFlow.Get() == ExpectedFlow);
		bAllValid &= TestTrue(FString::Printf(TEXT("%s RuneInfo Flow uses generated BaseFlow"), *Key), CardDA->RuneInfo.Flow.FlowAsset.Get() == ExpectedFlow);
		bAllValid &= TestEqual(FString::Printf(TEXT("%s has no link recipes"), *Key), CardConfig.LinkRecipes.Num(), 0);
	}

	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedHeavyCardBonusConfiguredTest,
	"DevKit.CombatDeck.GeneratedHeavyCardBonusConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedHeavyCardBonusConfiguredTest::RunTest(const FString& Parameters)
{
	bool bAllValid = true;

	URuneDataAsset* HeavyDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Heavy.DA_Rune512_Heavy"));
	bAllValid &= TestNotNull(TEXT("Generated heavy DA exists"), HeavyDA);
	if (!HeavyDA)
	{
		return false;
	}

	const FGameplayTag HeavyIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Heavy"), false);
	const FGameplayTag HeavyEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Heavy"), false);
	const FGameplayTag KnockbackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Knockback"), false);
	const FGameplayTag KnockbackActionTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
	const FCombatCardConfig& HeavyCard = HeavyDA->RuneInfo.CombatCard;
	bAllValid &= TestTrue(TEXT("Card.ID.Heavy tag exists"), HeavyIdTag.IsValid());
	bAllValid &= TestTrue(TEXT("Card.Effect.Heavy tag exists"), HeavyEffectTag.IsValid());
	bAllValid &= TestTrue(TEXT("Card.Effect.Knockback tag exists"), KnockbackEffectTag.IsValid());
	bAllValid &= TestTrue(TEXT("Action.Knockback tag exists"), KnockbackActionTag.IsValid());
	bAllValid &= TestEqual(TEXT("Heavy card is a normal card"), HeavyCard.CardType, ECombatCardType::Normal);
	bAllValid &= TestEqual(TEXT("Heavy card is rare"), HeavyDA->RuneInfo.RuneConfig.Rarity, ERuneRarity::Rare);
	bAllValid &= TestEqual(TEXT("Heavy card keeps Any action so light and heavy attacks can draw it"), HeavyCard.RequiredAction, ECardRequiredAction::Any);
	bAllValid &= TestEqual(TEXT("Heavy card triggers on hit"), HeavyCard.TriggerTiming, ECombatCardTriggerTiming::OnHit);
	bAllValid &= TestEqual(TEXT("Heavy card id configured"), HeavyCard.CardIdTag, HeavyIdTag);
	bAllValid &= TestTrue(TEXT("Heavy card has Heavy effect tag"), HeavyCard.CardEffectTags.HasTagExact(HeavyEffectTag));
	bAllValid &= TestTrue(TEXT("Heavy card has Knockback effect tag"), HeavyCard.CardEffectTags.HasTagExact(KnockbackEffectTag));
	const FString HeavyDescription = HeavyDA->RuneInfo.RuneConfig.RuneDescription.ToString();
	bAllValid &= TestTrue(TEXT("Heavy description explains light attacks can play it"), HeavyDescription.Contains(TEXT("轻攻击")));
	bAllValid &= TestTrue(TEXT("Heavy description explains base extra damage and knockback"), HeavyDescription.Contains(TEXT("额外伤害")) && HeavyDescription.Contains(TEXT("击退")));
	bAllValid &= TestTrue(TEXT("Heavy description explains coordination requirement"), HeavyDescription.Contains(TEXT("协调需求")));
	bAllValid &= TestTrue(TEXT("Heavy description explains heavy attack bonus"), HeavyDescription.Contains(TEXT("重攻击")) && HeavyDescription.Contains(TEXT("大幅提升")));

	UFlowAsset* HeavyFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Heavy_Base.FA_Rune512_Heavy_Base"));
	bAllValid &= TestNotNull(TEXT("Generated heavy base flow exists"), HeavyFlow);
	if (!HeavyFlow)
	{
		return false;
	}

	const UBFNode_CombatCardContextBranch* HeavyBranch = nullptr;
	const UBFNode_DoDamage* BaseDamageNode = nullptr;
	const UBFNode_DoDamage* ExtraDamageNode = nullptr;
	const UBFNode_SendGameplayEvent* BonusKnockbackNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : HeavyFlow->GetNodes())
	{
		if (const UBFNode_CombatCardContextBranch* Branch = Cast<UBFNode_CombatCardContextBranch>(Pair.Value))
		{
			if (Branch->RequiredAction == ECardRequiredAction::Heavy)
			{
				HeavyBranch = Branch;
			}
		}
		else if (const UBFNode_DoDamage* DamageNode = Cast<UBFNode_DoDamage>(Pair.Value))
		{
			if (DamageNode->TargetSelector == EBFTargetSelector::LastDamageTarget
				&& DamageNode->DamageEffect.Get()
				&& DamageNode->FlatDamage.Value > 0.f
				&& DamageNode->FlatDamage.Value < 20.f)
			{
				BaseDamageNode = DamageNode;
			}
			else if (DamageNode->TargetSelector == EBFTargetSelector::LastDamageTarget
				&& DamageNode->DamageEffect.Get()
				&& DamageNode->FlatDamage.Value >= 20.f)
			{
				ExtraDamageNode = DamageNode;
			}
		}
		else if (const UBFNode_SendGameplayEvent* EventNode = Cast<UBFNode_SendGameplayEvent>(Pair.Value))
		{
			if (EventNode->EventTag == KnockbackActionTag
				&& EventNode->Target == EBFTargetSelector::LastDamageTarget
				&& EventNode->PayloadTarget == EBFTargetSelector::LastDamageTarget
				&& EventNode->Magnitude.Value >= 520.f)
			{
				BonusKnockbackNode = EventNode;
			}
		}
	}

	bAllValid &= TestNotNull(TEXT("Heavy flow has base extra damage for light and heavy attacks"), BaseDamageNode);
	bAllValid &= TestNotNull(TEXT("Heavy flow has a heavy-action context branch"), HeavyBranch);
	if (HeavyBranch)
	{
		bAllValid &= TestTrue(TEXT("Heavy branch only matches heavy attacks"), HeavyBranch->RequiredSourceCardIdTags.HasTagExact(HeavyIdTag));
	}
	bAllValid &= TestNotNull(TEXT("Heavy flow has a large extra damage node for coordinated heavy attacks"), ExtraDamageNode);
	bAllValid &= TestNotNull(TEXT("Heavy flow has a large bonus knockback node for coordinated heavy attacks"), BonusKnockbackNode);

	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedVfxUsesAtomicVfxNodesTest,
	"DevKit.CombatDeck.GeneratedVfxUsesAtomicVfxNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedVfxUsesAtomicVfxNodesTest::RunTest(const FString& Parameters)
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
		int32 UnexpectedNiagaraCount = 0;
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
				if (NiagaraNode->NiagaraSystem && NiagaraNode->EffectName == FName(TEXT("Rune.Moonlight.BurnHitNiagara")))
				{
					BurnVfxNode = NiagaraNode;
				}
				else if (NiagaraNode->NiagaraSystem)
				{
					++UnexpectedNiagaraCount;
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
		TestNotNull(FString::Printf(TEXT("%s has burn Niagara VFX node"), Label), BurnVfxNode);
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
		TestTrue(FString::Printf(TEXT("%s burn VFX uses fire Niagara"), Label),
			GetNameSafe(BurnVfxNode->NiagaraSystem).Contains(TEXT("NS_Fire_Floor")));
		TestTrue(FString::Printf(TEXT("%s burn Niagara attaches to target mesh"), Label), BurnVfxNode->bAttachToTarget);
		TestTrue(FString::Printf(TEXT("%s burn Niagara remains compact"), Label),
			BurnVfxNode->Scale.X <= 0.5f && BurnVfxNode->Scale.Y <= 0.5f && BurnVfxNode->Scale.Z <= 0.5f);
		TestTrue(FString::Printf(TEXT("%s burn Niagara lifetime covers DOT preview"), Label), BurnVfxNode->Lifetime >= 3.f);
		TestFalse(FString::Printf(TEXT("%s burn VFX is not destroyed by short Flow cleanup"), Label), BurnVfxNode->bDestroyWithFlow);
		TestEqual(FString::Printf(TEXT("%s has no unexpected active Niagara nodes"), Label), UnexpectedNiagaraCount, 0);
		TestEqual(FString::Printf(TEXT("%s has no active Flipbook nodes"), Label), ActiveFlipbookCount, 0);
		TestEqual(FString::Printf(TEXT("%s burn DOT targets last damage target"), Label), BurnEffectNode->Target, EBFTargetSelector::LastDamageTarget);
		TestFalse(FString::Printf(TEXT("%s burn DOT survives Flow cleanup"), Label), BurnEffectNode->bRemoveEffectOnCleanup);
		TestEqual(FString::Printf(TEXT("%s burn DOT uses Data.Damage.Burn"), Label), BurnEffectNode->SetByCallerTag1.GetTagName(), FName(TEXT("Data.Damage.Burn")));

		return true;
	};

	const bool bForwardValid = ValidateBurnFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Forward_Burn.FA_Rune512_Moonlight_Forward_Burn"),
		TEXT("Forward burn Moonlight"));

	return bForwardValid;
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

	return bForwardValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckMoonlightReversedGroundPathFlowTest,
	"DevKit.CombatDeck.MoonlightReversedGroundPathFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckMoonlightReversedGroundPathFlowTest::RunTest(const FString& Parameters)
{
	auto ValidateGroundPathFlow = [this](const TCHAR* FlowPath, const TCHAR* Label, const bool bBurn) -> bool
	{
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s Flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		UBFNode_CalcRuneGroundPathTransform* CalcTransformNode = nullptr;
		UBFNode_SpawnRuneGroundPathEffect* GroundPathNode = nullptr;
		UBFNode_SpawnRuneAreaProfile* AreaProfileNode = nullptr;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (!CalcTransformNode)
			{
				CalcTransformNode = Cast<UBFNode_CalcRuneGroundPathTransform>(Pair.Value);
			}
			if (!GroundPathNode)
			{
				GroundPathNode = Cast<UBFNode_SpawnRuneGroundPathEffect>(Pair.Value);
			}
			if (!AreaProfileNode)
			{
				AreaProfileNode = Cast<UBFNode_SpawnRuneAreaProfile>(Pair.Value);
			}
		}

		TestNotNull(FString::Printf(TEXT("%s has Calc Rune Ground Path Transform node"), Label), CalcTransformNode);
		TestNotNull(FString::Printf(TEXT("%s keeps legacy Rune Ground Path node for reference"), Label), GroundPathNode);
		TestNotNull(FString::Printf(TEXT("%s has Spawn Rune Area Profile node"), Label), AreaProfileNode);
		if (!AreaProfileNode || !CalcTransformNode)
		{
			return false;
		}
		TestNotNull(FString::Printf(TEXT("%s area profile is assigned"), Label), AreaProfileNode->Profile.Get());
		if (!AreaProfileNode->Profile)
		{
			return false;
		}

		UFlowNode* EntryNode = Flow->GetDefaultEntryNode();
		TestNotNull(FString::Printf(TEXT("%s has default entry node"), Label), EntryNode);
		if (EntryNode)
		{
			const FConnectedPin EntryConnection = EntryNode->GetConnection(TEXT("Out"));
			TestEqual(FString::Printf(TEXT("%s entry executes calc transform first"), Label), EntryConnection.NodeGuid, CalcTransformNode->GetGuid());
		}

		const FConnectedPin CalcExecConnection = CalcTransformNode->GetConnection(TEXT("Out"));
		TestEqual(FString::Printf(TEXT("%s calc executes area profile spawn"), Label), CalcExecConnection.NodeGuid, AreaProfileNode->GetGuid());

		const FConnectedPin LocationConnection = AreaProfileNode->GetConnection(GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneAreaProfile, SpawnLocationOverride));
		TestEqual(FString::Printf(TEXT("%s spawn location comes from calc node"), Label), LocationConnection.NodeGuid, CalcTransformNode->GetGuid());
		TestEqual(FString::Printf(TEXT("%s spawn location uses calc SpawnLocation output"), Label),
			LocationConnection.PinName,
			GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, SpawnLocation));

		const FConnectedPin RotationConnection = AreaProfileNode->GetConnection(GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneAreaProfile, SpawnRotationOverride));
		TestEqual(FString::Printf(TEXT("%s spawn rotation comes from calc node"), Label), RotationConnection.NodeGuid, CalcTransformNode->GetGuid());
		TestEqual(FString::Printf(TEXT("%s spawn rotation uses calc SpawnRotation output"), Label),
			RotationConnection.PinName,
			GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, SpawnRotation));

		const FRuneCardProfileAreaConfig& Area = AreaProfileNode->Profile->Area;
		TestEqual(FString::Printf(TEXT("%s calc source selector"), Label), CalcTransformNode->Source, EBFTargetSelector::BuffOwner);
		TestEqual(FString::Printf(TEXT("%s calc faces last damage target"), Label), CalcTransformNode->FacingMode, ERuneGroundPathFacingMode::ToLastDamageTarget);
		TestTrue(FString::Printf(TEXT("%s calc centers path on length"), Label), CalcTransformNode->bCenterOnPathLength);
		TestEqual(FString::Printf(TEXT("%s spawn fallback faces last damage target"), Label), Area.FacingMode, ERuneGroundPathFacingMode::ToLastDamageTarget);
		TestEqual(FString::Printf(TEXT("%s targets enemies only"), Label), Area.TargetPolicy, ERuneGroundPathTargetPolicy::EnemiesOnly);
		TestEqual(FString::Printf(TEXT("%s source selector"), Label), Area.Source, EBFTargetSelector::BuffOwner);
		TestNotNull(FString::Printf(TEXT("%s has path decal material"), Label), Area.DecalMaterial.Get());
		TestEqual(FString::Printf(TEXT("%s decal plane rotation aligns visual forward"), Label), Area.DecalPlaneRotationDegrees, 0.0f);
		TestTrue(FString::Printf(TEXT("%s has path Niagara"), Label), Area.NiagaraSystem != nullptr);

		if (bBurn)
		{
			TestEqual(FString::Printf(TEXT("%s burn shape"), Label), Area.Shape, ERuneGroundPathShape::Fan);
			TestEqual(FString::Printf(TEXT("%s burn duration"), Label), Area.Duration, 4.0f);
			TestEqual(FString::Printf(TEXT("%s burn scan interval"), Label), Area.TickInterval, 0.5f);
			TestEqual(FString::Printf(TEXT("%s burn length"), Label), Area.Length, 520.0f);
			TestEqual(FString::Printf(TEXT("%s burn width"), Label), Area.Width, 230.0f);
			TestEqual(FString::Printf(TEXT("%s burn decal projection depth"), Label), Area.DecalProjectionDepth, 18.0f);
			TestEqual(FString::Printf(TEXT("%s burn ground fire instances"), Label), Area.NiagaraInstanceCount, 7);
			TestTrue(FString::Printf(TEXT("%s burn applies once per path target"), Label), Area.bApplyOncePerTarget);
			TestTrue(FString::Printf(TEXT("%s uses burn ground path decal"), Label),
				Area.DecalMaterial != nullptr
				&& Area.DecalMaterial->GetName().Contains(TEXT("GroundPath_Burn_Fan_Decal")));
			TestTrue(FString::Printf(TEXT("%s applies UGE_RuneBurn"), Label), Area.Effect.Get() == UGE_RuneBurn::StaticClass());
			TestEqual(FString::Printf(TEXT("%s burn uses Data.Damage.Burn"), Label), Area.SetByCallerTag1.GetTagName(), FName(TEXT("Data.Damage.Burn")));
			TestEqual(FString::Printf(TEXT("%s burn damage per tick"), Label), Area.SetByCallerValue1, 6.0f);
			TestTrue(FString::Printf(TEXT("%s burn profile owns status GE duration"), Label), AreaProfileNode->Profile->Effect.bOverrideDuration);
			TestEqual(FString::Printf(TEXT("%s burn profile duration matches burn area"), Label), AreaProfileNode->Profile->Effect.Duration, Area.Duration);
			TestTrue(FString::Printf(TEXT("%s burn profile owns status GE period"), Label), AreaProfileNode->Profile->Effect.bOverridePeriod);
			TestEqual(FString::Printf(TEXT("%s burn profile period"), Label), AreaProfileNode->Profile->Effect.Period, 1.0f);
			TestNotNull(FString::Printf(TEXT("%s burn target VFX is profile-owned"), Label), AreaProfileNode->Profile->VFX.NiagaraSystem.Get());
			TestEqual(FString::Printf(TEXT("%s burn target VFX socket"), Label), AreaProfileNode->Profile->VFX.AttachSocketName, FName(TEXT("spine_03")));
		}
		else
		{
			TestEqual(FString::Printf(TEXT("%s poison shape"), Label), Area.Shape, ERuneGroundPathShape::Rectangle);
			TestEqual(FString::Printf(TEXT("%s poison duration"), Label), Area.Duration, 4.5f);
			TestEqual(FString::Printf(TEXT("%s poison tick interval"), Label), Area.TickInterval, 1.0f);
			TestEqual(FString::Printf(TEXT("%s poison length"), Label), Area.Length, 560.0f);
			TestEqual(FString::Printf(TEXT("%s poison width"), Label), Area.Width, 210.0f);
			TestEqual(FString::Printf(TEXT("%s poison decal projection depth"), Label), Area.DecalProjectionDepth, 18.0f);
			TestEqual(FString::Printf(TEXT("%s poison ground VFX instances"), Label), Area.NiagaraInstanceCount, 1);
			TestFalse(FString::Printf(TEXT("%s poison can reapply per scan"), Label), Area.bApplyOncePerTarget);
			TestTrue(FString::Printf(TEXT("%s uses poison ground path decal"), Label),
				Area.DecalMaterial != nullptr
				&& Area.DecalMaterial->GetName().Contains(TEXT("GroundPath_Poison_Decal")));
			TestTrue(FString::Printf(TEXT("%s applies GE_Poison"), Label), Area.Effect != nullptr && Area.Effect->GetName().Contains(TEXT("GE_Poison")));
		}

		return true;
	};

	const bool bBurnValid = ValidateGroundPathFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Burn.FA_Rune512_Moonlight_Reversed_Burn"),
		TEXT("Reversed burn Moonlight"),
		true);
	const bool bPoisonValid = ValidateGroundPathFlow(
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Reversed_Poison.FA_Rune512_Moonlight_Reversed_Poison"),
		TEXT("Reversed poison Moonlight"),
		false);

	return bBurnValid && bPoisonValid;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyDeathPoisonFlowTargetsNearbyCharactersTest,
	"DevKit.CombatDeck.EnemyDeathPoisonFlowTargetsNearbyCharacters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathPoisonFlowTargetsNearbyCharactersTest::RunTest(const FString& Parameters)
{
	const FGameplayTag DeathAnimCompleteTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.DeathAnimComplete"), false);
	TestTrue(TEXT("Death animation complete tag exists"), DeathAnimCompleteTag.IsValid());

	UFlowAsset* Flow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/Playtest_GA/DeathPoison/FA_Rune_DeathPoison.FA_Rune_DeathPoison"));
	TestNotNull(TEXT("Enemy death poison flow exists"), Flow);
	if (!Flow)
	{
		return false;
	}

	UBFNode_WaitGameplayEvent* WaitNode = nullptr;
	UBFNode_ApplyGEInRadius* RadiusNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
	{
		if (!WaitNode)
		{
			WaitNode = Cast<UBFNode_WaitGameplayEvent>(Pair.Value);
		}
		if (!RadiusNode)
		{
			RadiusNode = Cast<UBFNode_ApplyGEInRadius>(Pair.Value);
		}
	}

	TestNotNull(TEXT("Death poison waits for death animation completion"), WaitNode);
	TestNotNull(TEXT("Death poison applies poison in radius"), RadiusNode);
	if (!WaitNode || !RadiusNode)
	{
		return false;
	}

	TestEqual(TEXT("Death poison listens on the dying enemy"), WaitNode->Target, EBFTargetSelector::BuffOwner);
	TestEqual(TEXT("Death poison listens for DeathAnimComplete"), WaitNode->GetRuntimeEventTag(), DeathAnimCompleteTag);
	TestTrue(TEXT("Death poison uses poison splash GE"),
		RadiusNode->Effect != nullptr && RadiusNode->Effect->GetName().Contains(TEXT("GE_PoisonSplash")));
	TestEqual(TEXT("Death poison radius centers on the dying enemy"), RadiusNode->LocationSource, EBFTargetSelector::BuffOwner);
	TestFalse(TEXT("Death poison runtime filter includes nearby players"),
		RadiusNode->ShouldRestrictTargetsToEnemiesForRuntime());

	return true;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckBurnStatusNiagaraNotAutoBoundToTagTest,
	"DevKit.CombatDeck.BurnStatusNiagaraNotAutoBoundToBurningTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckBurnStatusNiagaraNotAutoBoundToTagTest::RunTest(const FString& Parameters)
{
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Burning"), false);
	TestTrue(TEXT("Buff.Status.Burning tag exists"), BurningTag.IsValid());

	UYogAbilitySystemComponent* ASC = NewObject<UYogAbilitySystemComponent>();
	TestNotNull(TEXT("Yog ASC exists for burn status VFX binding test"), ASC);
	if (!ASC)
	{
		return false;
	}

	UNiagaraSystem* BurnSystem = ASC->GetStatusNiagaraSystemForTag(BurningTag);
	TestNull(TEXT("Burning tag does not auto resolve Niagara; burn VFX is FA/Profile-driven"), BurnSystem);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRuneEffectProfileDefaultsTest,
	"DevKit.CombatDeck.RuneEffectProfileDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRuneEffectProfileDefaultsTest::RunTest(const FString& Parameters)
{
	const FName ProfileObjectName = MakeUniqueObjectName(GetTransientPackage(), URuneCardEffectProfileDA::StaticClass(), TEXT("EP_Test_Profile"));
	URuneCardEffectProfileDA* Profile = NewObject<URuneCardEffectProfileDA>(GetTransientPackage(), URuneCardEffectProfileDA::StaticClass(), ProfileObjectName);
	TestNotNull(TEXT("Profile can be constructed"), Profile);
	if (!Profile)
	{
		return false;
	}

	Profile->DebugName = TEXT("TestMoonlightBurn");
	Profile->DamageMode = ERuneCardProfileDamageMode::Fixed;
	Profile->DamageValue = 35.0f;
	Profile->DamageLogType = TEXT("Rune_Profile_Test");
	Profile->Effect.ApplicationCount = 2;
	Profile->Effect.bRemoveEffectOnCleanup = false;
	Profile->Effect.bOverrideDuration = true;
	Profile->Effect.Duration = 5.0f;
	Profile->Effect.bOverridePeriod = true;
	Profile->Effect.Period = 0.75f;
	Profile->Projectile.Speed = 1100.0f;
	Profile->Projectile.MaxDistance = 800.0f;
	Profile->Projectile.ProjectileCount = 3;
	Profile->Projectile.bSpawnProjectilesSequentially = true;
	Profile->Area.Duration = 4.0f;
	Profile->Area.TickInterval = 0.5f;
	Profile->Area.Length = 520.0f;
	Profile->Area.Width = 230.0f;
	Profile->VFX.EffectName = TEXT("Rune.Profile.TestVFX");
	Profile->VFX.Scale = FVector(0.4f, 0.4f, 0.4f);

	TestEqual(TEXT("Trace name uses DebugName"), Profile->GetTraceName(), FName(TEXT("TestMoonlightBurn")));
	TestEqual(TEXT("Damage value is editable on profile"), Profile->DamageValue, 35.0f);
	TestEqual(TEXT("Effect application count is editable on profile"), Profile->Effect.ApplicationCount, 2);
	TestTrue(TEXT("Effect duration override is profile-owned"), Profile->Effect.bOverrideDuration);
	TestEqual(TEXT("Effect duration is editable on profile"), Profile->Effect.Duration, 5.0f);
	TestTrue(TEXT("Effect period override is profile-owned"), Profile->Effect.bOverridePeriod);
	TestEqual(TEXT("Effect period is editable on profile"), Profile->Effect.Period, 0.75f);
	TestEqual(TEXT("Projectile speed is editable on profile"), Profile->Projectile.Speed, 1100.0f);
	TestEqual(TEXT("Projectile count supports sequential moonlight tuning"), Profile->Projectile.ProjectileCount, 3);
	TestTrue(TEXT("Projectile sequential flag is profile-owned"), Profile->Projectile.bSpawnProjectilesSequentially);
	TestEqual(TEXT("Area duration is profile-owned"), Profile->Area.Duration, 4.0f);
	TestEqual(TEXT("Area tick interval is profile-owned"), Profile->Area.TickInterval, 0.5f);
	TestEqual(TEXT("VFX scale is profile-owned"), Profile->VFX.Scale, FVector(0.4f, 0.4f, 0.4f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRuneEffectProfileNodesConstructTest,
	"DevKit.CombatDeck.RuneEffectProfileNodesConstruct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRuneEffectProfileNodesConstructTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Apply Rune Effect Profile node constructs"), NewObject<UBFNode_ApplyRuneEffectProfile>());
	TestNotNull(TEXT("Spawn Rune Projectile Profile node constructs"), NewObject<UBFNode_SpawnRuneProjectileProfile>());
	TestNotNull(TEXT("Spawn Rune Area Profile node constructs"), NewObject<UBFNode_SpawnRuneAreaProfile>());
	TestNotNull(TEXT("Play Rune VFX Profile node constructs"), NewObject<UBFNode_PlayRuneVFXProfile>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckBuffFlowTraceRecordsProfileTest,
	"DevKit.CombatDeck.BuffFlowTraceRecordsProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckBuffFlowTraceRecordsProfileTest::RunTest(const FString& Parameters)
{
	UBuffFlowComponent* BFC = NewObject<UBuffFlowComponent>();
	const FName ProfileObjectName = MakeUniqueObjectName(GetTransientPackage(), URuneCardEffectProfileDA::StaticClass(), TEXT("EP_Trace_Profile"));
	URuneCardEffectProfileDA* Profile = NewObject<URuneCardEffectProfileDA>(GetTransientPackage(), URuneCardEffectProfileDA::StaticClass(), ProfileObjectName);
	TestNotNull(TEXT("BuffFlowComponent constructs"), BFC);
	TestNotNull(TEXT("Profile constructs"), Profile);
	if (!BFC || !Profile)
	{
		return false;
	}

	BFC->RecordTrace(nullptr, Profile, nullptr, EBuffFlowTraceResult::Success, TEXT("Applied"), TEXT("Damage=35 Duration=4"));

	const TArray<FBuffFlowTraceEntry> Entries = BFC->GetTraceEntries();
	TestEqual(TEXT("Trace stores one entry"), Entries.Num(), 1);
	if (Entries.Num() != 1)
	{
		return false;
	}

	TestEqual(TEXT("Trace records profile asset name"), Entries[0].ProfileName, ProfileObjectName);
	TestEqual(TEXT("Trace records result"), Entries[0].Result, EBuffFlowTraceResult::Success);
	TestEqual(TEXT("Trace records message"), Entries[0].Message, FString(TEXT("Applied")));
	TestEqual(TEXT("Trace records values"), Entries[0].Values, FString(TEXT("Damage=35 Duration=4")));

	BFC->ClearTraceEntries();
	TestEqual(TEXT("Trace clears entries"), BFC->GetTraceEntries().Num(), 0);
	return true;
}

#endif
