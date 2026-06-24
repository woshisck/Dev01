#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Component/CombatDeckComponent.h"
#include "Component/BufferComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Component/SacrificeRuneComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_ActiveSkill_ShieldBurst.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Abilities/GA_PlayerAttackCombos.h"
#include "AbilitySystem/Abilities/GA_PlayerDash.h"
#include "AbilitySystem/Abilities/GA_SwitchWeapon.h"
#include "AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
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
#include "BuffFlow/Nodes/BFNode_SpawnBuffFlowProjectile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Animation/AnimNotifyState_PostAtkWindow.h"
#include "Data/AbilityData.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include "System/YogGameInstanceBase.h"
#include "UI/WeaponComboTextUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "NiagaraSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckFormalGameplayTagsConfiguredTest,
	"DevKit.CombatDeck.FormalGameplayTagsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckFormalGameplayTagsConfiguredTest::RunTest(const FString& Parameters)
{
	const TCHAR* RequiredTagNames[] = {
		TEXT("Character.State.Skill.Attack"),
		TEXT("Character.State.Skill.WeaponSkill"),
		TEXT("Character.State.Movement.Dash"),
		TEXT("Character.State.Equipment.SwitchWeapon"),
		TEXT("Character.State.Window.CanCombo"),
		TEXT("Character.State.Window.PostAttackRecovery"),
		TEXT("Buff.Moonlight"),
		TEXT("Buff.AttackUp"),
		TEXT("Buff.Attack"),
		TEXT("Buff.Moonlight"),
		TEXT("Buff.SplashSplit"),
	};

	for (const TCHAR* TagName : RequiredTagNames)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		const FString Message = FString::Printf(TEXT("formal gameplay tag exists: %s"), TagName);
		TestTrue(*Message, Tag.IsValid());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckAdvancesCardsWithoutShufflingTest,
	"DevKit.CombatDeck.AdvancesCardsWithoutConsumingOrShuffling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckAdvancesCardsWithoutShufflingTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetShuffleCooldownDuration(1.0f);
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
	});

	for (int32 i = 0; i < 3; ++i)
	{
		const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
		TestTrue(TEXT("Card resolves before the sequence reaches the end"), Result.bHadCard);
		TestFalse(TEXT("Card resolve does not start shuffle"), Result.bStartedShuffle);
		TestEqual(TEXT("Full combat sequence stays visible while advancing"), Deck->GetRemainingDeckSnapshot().Num(), 4);
		TestEqual(TEXT("Current index advances left to right"), Deck->GetCurrentIndex(), i + 1);
	}

	const FCombatCardResolveResult FinalResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Fourth attack resolves the fourth card"), FinalResult.bHadCard);
	TestFalse(TEXT("Final card no longer starts shuffle"), FinalResult.bStartedShuffle);
	TestEqual(TEXT("Deck remains ready after the final visible card"), Deck->GetDeckState(), EDeckState::Ready);
	TestEqual(TEXT("Full combat sequence remains visible after the final card"), Deck->GetRemainingDeckSnapshot().Num(), 4);
	TestEqual(TEXT("Current index rests just past the last card until combo exit"), Deck->GetCurrentIndex(), 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckComboExitResetsToFirstCardTest,
	"DevKit.CombatDeck.ComboExitResetsToFirstCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckComboExitResetsToFirstCardTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetShuffleCooldownDuration(1.0f);

	FCombatCardConfig FirstCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	FirstCard.DisplayName = FText::FromString(TEXT("First"));
	FCombatCardConfig SecondCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	SecondCard.DisplayName = FText::FromString(TEXT("Second"));
	Deck->SetDeckListForTest({ FirstCard, SecondCard });

	const FCombatCardResolveResult FirstResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("First attack resolves the first card"), FirstResult.bHadCard);
	TestEqual(TEXT("Combo progress advances to the second card"), Deck->GetCurrentIndex(), 1);

	Deck->NotifyComboStateExited();
	TestEqual(TEXT("Combo exit resets combat sequence progress"), Deck->GetCurrentIndex(), 0);
	TestEqual(TEXT("Deck stays ready after combo exit"), Deck->GetDeckState(), EDeckState::Ready);

	const FCombatCardResolveResult RestartResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Next attack after combo exit resolves a card"), RestartResult.bHadCard);
	TestEqual(TEXT("Restarted combo begins from the first card again"),
		RestartResult.ResolvedCard.Config.DisplayName.ToString(),
		FString(TEXT("First")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLegacyActionRequirementsMatchAnyAttackContextTest,
	"DevKit.CombatDeck.LegacyActionRequirementsMatchAnyAttackContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLegacyActionRequirementsMatchAnyAttackContextTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Light },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Heavy },
	});

	const FCombatCardResolveResult FirstResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	const FCombatCardResolveResult SecondResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);

	TestTrue(TEXT("Any attack context still resolves a legacy light card"), FirstResult.bHadCard);
	TestTrue(TEXT("Any attack context matches legacy light requirements"), FirstResult.bActionMatched);
	TestTrue(TEXT("Legacy light card triggers base flow"), FirstResult.bTriggeredBaseFlow);
	TestTrue(TEXT("Any attack context still resolves a legacy heavy card"), SecondResult.bHadCard);
	TestTrue(TEXT("Any attack context matches legacy heavy requirements"), SecondResult.bActionMatched);
	TestTrue(TEXT("Legacy heavy card triggers base flow"), SecondResult.bTriggeredBaseFlow);

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
	NormalContext.ActionType = ECardRequiredAction::Any;
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

	TestTrue(TEXT("Finisher release resolves the held card"), FinisherResult.bHadCard);
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
	FinisherContext.ActionType = ECardRequiredAction::Any;
	FinisherContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	FinisherContext.bIsComboFinisher = true;
	FinisherContext.ReleaseMode = ECombatCardReleaseMode::Finisher;
	FinisherContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult FinisherResult = Deck->ResolveAttackCardWithContext(FinisherContext);

	TestTrue(TEXT("A normal card is visible to the finisher release attempt"), FinisherResult.bHadCard);
	TestFalse(TEXT("Finisher release does not match a normal card"), FinisherResult.bReleaseModeMatched);
	TestFalse(TEXT("Finisher release does not resolve normal card base flow"), FinisherResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Finisher release holds normal cards for normal attacks"), Deck->GetCurrentIndex(), 0);

	FCombatDeckActionContext NormalContext = FinisherContext;
	NormalContext.bIsComboFinisher = false;
	NormalContext.ReleaseMode = ECombatCardReleaseMode::Normal;
	NormalContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult NormalResult = Deck->ResolveAttackCardWithContext(NormalContext);

	TestTrue(TEXT("Normal release resolves the held normal card"), NormalResult.bHadCard);
	TestTrue(TEXT("Normal release matches the held normal card"), NormalResult.bReleaseModeMatched);
	TestTrue(TEXT("Normal release runs base flow"), NormalResult.bTriggeredBaseFlow);
	TestFalse(TEXT("Normal release does not start shuffle"), NormalResult.bStartedShuffle);
	TestEqual(TEXT("Normal release keeps the deck ready"), Deck->GetDeckState(), EDeckState::Ready);
	TestEqual(TEXT("Normal release advances past the held normal card"), Deck->GetCurrentIndex(), 1);

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
	AttackContext.ActionType = ECardRequiredAction::Any;
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

	TestTrue(TEXT("Skill-slot context resolves the held card"), SkillResult.bHadCard);
	TestTrue(TEXT("Skill-slot context matches the held card"), SkillResult.bActionSlotMatched);
	TestTrue(TEXT("Skill-slot context runs base flow"), SkillResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Skill-slot card does not advance the attack sequence"), Deck->GetCurrentIndex(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckSkillSlotCanResolveOnCommitTest,
	"DevKit.CombatDeck.SkillSlotCanResolveOnCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckSkillSlotCanResolveOnCommitTest::RunTest(const FString& Parameters)
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

	TestFalse(TEXT("OnCommit without resolve flag only previews/prepares card flow"), PreviewResult.bHadCard);
	TestEqual(TEXT("OnCommit preview holds the skill card"), Deck->GetCurrentIndex(), 0);

	FCombatDeckActionContext ResolveContext = PreviewContext;
	ResolveContext.bConsumeOnCommit = true;
	ResolveContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult ResolveResult = Deck->ResolveAttackCardWithContext(ResolveContext);

	TestTrue(TEXT("Skill-slot OnCommit can resolve when explicitly requested"), ResolveResult.bHadCard);
	TestTrue(TEXT("Skill-slot OnCommit matches slot"), ResolveResult.bActionSlotMatched);
	TestTrue(TEXT("Skill-slot OnCommit matches catalyst role"), ResolveResult.bFlowRoleMatched);
	TestTrue(TEXT("Skill-slot OnCommit runs base flow"), ResolveResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Resolved skill-slot card does not advance the attack sequence"), Deck->GetCurrentIndex(), 0);

	ResolveContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult RepeatResult = Deck->ResolveAttackCardWithContext(ResolveContext);
	TestTrue(TEXT("Single skill slot can trigger again on the next skill use"), RepeatResult.bHadCard);
	TestEqual(TEXT("Repeated skill slot trigger still leaves attack sequence untouched"), Deck->GetCurrentIndex(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckAttackContextUsesCardAuthoredFlowRoleTest,
	"DevKit.CombatDeck.AttackContextUsesCardAuthoredFlowRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckAttackContextUsesCardAuthoredFlowRoleTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig CatalystAttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	CatalystAttackCard.RequiredActionSlot = ECombatDeckActionSlot::Attack;
	CatalystAttackCard.RequiredFlowRole = ECombatDeckFlowRole::Catalyst;
	CatalystAttackCard.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	Deck->SetDeckListForTest({ CatalystAttackCard });

	FCombatDeckActionContext AttackContext;
	AttackContext.ActionType = ECardRequiredAction::Any;
	AttackContext.ActionSlot = ECombatDeckActionSlot::Attack;
	AttackContext.FlowRole = ECombatDeckFlowRole::Any;
	AttackContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult Result = Deck->ResolveAttackCardWithContext(AttackContext);

	TestTrue(TEXT("Attack context with Any action triggers attack-slot cards"), Result.bHadCard);
	TestTrue(TEXT("Attack context matches Any action requirements"), Result.bActionMatched);
	TestTrue(TEXT("Card-authored flow role matches when attack context allows card roles"), Result.bFlowRoleMatched);
	TestEqual(TEXT("Resolved context stores the card-authored catalyst role"), Result.ActionContext.FlowRole, ECombatDeckFlowRole::Catalyst);

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
	CatalystContext.ActionType = ECardRequiredAction::Any;
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

	TestTrue(TEXT("Finisher role resolves the held card"), FinisherResult.bHadCard);
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

	const FCombatCardResolveResult Attack = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestFalse(TEXT("Deprecated finisher card is not inserted into the deck"), Attack.bHadCard);

	const FCombatCardResolveResult ComboFinisher = Deck->ResolveAttackCard(ECardRequiredAction::Any, true, false);
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

	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	AttackCard.DisplayName = FText::FromString(TEXT("Attack"));

	Deck->SetDeckListForTest({ FinisherCard, AttackCard });

	const TArray<FCombatCardInstance> InitialCards = Deck->GetDeckSnapshot();
	TestEqual(TEXT("Deprecated finisher is filtered out of the active deck"), InitialCards.Num(), 1);
	if (InitialCards.Num() == 1)
	{
		TestEqual(TEXT("Attack card remains after finisher filtering"), InitialCards[0].Config.CardType, ECombatCardType::Attack);
	}

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Attack card can still be resolved"), AttackResult.bHadCard);
	TestFalse(TEXT("Resolving the remaining attack card does not start shuffle"), AttackResult.bStartedShuffle);
	TestEqual(TEXT("Deck remains ready after resolving the attack card"), Deck->GetDeckState(), EDeckState::Ready);
	TestFalse(TEXT("Deprecated finisher card never enters suppression tracking"),
		Deck->IsCardSuppressedFromActiveSequenceForTest(FGuid::NewGuid()));

	const TArray<FCombatCardInstance> VisibleCards = Deck->GetDeckSnapshot();
	TestEqual(TEXT("Visible sequence keeps only the non-finisher card"), VisibleCards.Num(), 1);
	if (VisibleCards.Num() == 1)
	{
		TestEqual(TEXT("Attack card stays in the visible deck"), VisibleCards[0].Config.CardType, ECombatCardType::Attack);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckSourceRestoreTest,
	"DevKit.CombatDeck.SourceAssetsCanRestoreDeckOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckSourceRestoreTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* FirstRune = NewObject<URuneDataAsset>();
	FirstRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };

	URuneDataAsset* SecondRune = NewObject<URuneDataAsset>();
	SecondRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };

	UCombatDeckComponent* SourceDeck = NewObject<UCombatDeckComponent>();
	SourceDeck->AddCardFromRuneReward(FirstRune);
	SourceDeck->AddCardFromRuneReward(SecondRune);

	TArray<URuneDataAsset*> SavedAssets = SourceDeck->GetDeckSourceAssets();

	UCombatDeckComponent* RestoredDeck = NewObject<UCombatDeckComponent>();
	RestoredDeck->LoadDeckFromSourceAssets(SavedAssets, 1.0f, 0);

	TestEqual(TEXT("Restored active sequence keeps source asset order"), RestoredDeck->GetDeckSnapshot().Num(), 2);

	const FCombatCardResolveResult FirstResult = RestoredDeck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("First restored card matches the current attack action"), FirstResult.bActionMatched);

	const FCombatCardResolveResult SecondResult = RestoredDeck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Second restored card matches the current attack action"), SecondResult.bActionMatched);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRewardCardAutoReloadsIntoVisibleSequenceTest,
	"DevKit.CombatDeck.RewardCardAutoReloadsIntoVisibleSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRewardCardAutoReloadsIntoVisibleSequenceTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* FirstRune = NewObject<URuneDataAsset>();
	FirstRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };

	URuneDataAsset* SecondRune = NewObject<URuneDataAsset>();
	SecondRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };

	URuneDataAsset* RewardRune = NewObject<URuneDataAsset>();
	RewardRune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->LoadDeckFromExactSourceAssets({ FirstRune, SecondRune }, 1.0f, 2);
	TestEqual(TEXT("Initial active sequence is capped at two cards"), Deck->GetRemainingDeckSnapshot().Num(), 2);

	TestTrue(TEXT("Reward rune enters combat deck"), Deck->AddCardFromRuneReward(RewardRune));
	TestEqual(TEXT("Reward pickup keeps deck ready instead of starting a reload shuffle"), Deck->GetDeckState(), EDeckState::Ready);

	const TArray<FCombatCardInstance> VisibleCards = Deck->GetRemainingDeckSnapshot();
	bool bRewardVisible = false;
	for (const FCombatCardInstance& Card : VisibleCards)
	{
		bRewardVisible |= Card.SourceData == RewardRune;
	}

	TestTrue(TEXT("Reward card is visible after the immediate sequence refresh"), bRewardVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRemainingSnapshotTest,
	"DevKit.CombatDeck.VisibleSnapshotKeepsResolvedCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRemainingSnapshotTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
	});

	TestEqual(TEXT("Visible snapshot starts with every active card"), Deck->GetRemainingDeckSnapshot().Num(), 2);

	Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);

	const TArray<FCombatCardInstance> RemainingSnapshot = Deck->GetRemainingDeckSnapshot();
	TestEqual(TEXT("Resolved cards stay in the visible snapshot"), RemainingSnapshot.Num(), 2);
	if (RemainingSnapshot.IsValidIndex(0))
	{
		TestEqual(TEXT("Visible snapshot keeps fixed left-to-right order"), RemainingSnapshot[0].Config.RequiredAction, ECardRequiredAction::Any);
	}
	TestEqual(TEXT("Sequence pointer advances without removing the first card"), Deck->GetCurrentIndex(), 1);

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

	TestFalse(TEXT("Consuming without saved action-window tags reports false"), ASC->ConsumeDashSave());

	const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
	FGameplayTagContainer SavedTags;
	SavedTags.AddTag(CanComboTag);

	ASC->ApplyDashSave(SavedTags);
	TestEqual(TEXT("DashSave tag is applied before consumption"), ASC->GetTagCount(CanComboTag), 1);

	TestTrue(TEXT("Consuming saved action-window tags reports true"), ASC->ConsumeDashSave());
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

	const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.HitReact"));
	ASC->AddLooseGameplayTag(HitReactTag);

	TestFalse(TEXT("HitReact blocks player/enemy movement control even without StateConflict data config"), Character->bMovable);
	if (const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		TestTrue(TEXT("HitReact keeps movement mode available for montage root motion"), MoveComp->MovementMode != MOVE_None);
	}

	Character->Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeAbilityHasActionAndDeathGuardsTest,
	"DevKit.CombatDeck.MeleeAbilityHasActionAndDeathGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeleeAbilityHasActionAndDeathGuardsTest::RunTest(const FString& Parameters)
{
	UGA_MeleeAttack* Ability = NewObject<UGA_MeleeAttack>();
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead"));

	TestTrue(TEXT("Generic melee ability carries the attack action tag"),
		Ability->GetAbilityTags().HasTagExact(AttackTag));
	TestTrue(TEXT("Generic melee ability owns the attack action tag while active"),
		Ability->GetActivationOwnedTags().HasTagExact(AttackTag));
	TestTrue(TEXT("Generic melee ability is blocked while dead"),
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
	const FGameplayTag DashInvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.DashInvincible"));
	const FGameplayTag ActionTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast"));
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead"));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwitchWeaponAbilityHasDefaultTagsAndGuardsTest,
	"DevKit.CombatDeck.SwitchWeaponAbilityHasDefaultTagsAndGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSwitchWeaponAbilityHasDefaultTagsAndGuardsTest::RunTest(const FString& Parameters)
{
	UGA_SwitchWeapon* Ability = NewObject<UGA_SwitchWeapon>();
	const FGameplayTag SwitchWeaponTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SwitchWeapon"));
	const FGameplayTag ActionTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast"));
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead"));

	TestTrue(TEXT("SwitchWeapon ability carries the switch action tag"),
		Ability->GetAbilityTags().HasTagExact(SwitchWeaponTag));
	TestTrue(TEXT("SwitchWeapon ability owns the switch action tag while active"),
		Ability->GetActivationOwnedTags().HasTagExact(SwitchWeaponTag));
	TestTrue(TEXT("SwitchWeapon cancels current player action abilities"),
		Ability->GetCancelAbilitiesWithTag().HasTagExact(ActionTag));
	TestTrue(TEXT("SwitchWeapon cannot activate while dead"),
		Ability->GetActivationBlockedTags().HasTagExact(DeadTag));
	TestTrue(TEXT("SwitchWeapon cannot retrigger while a switch montage is active"),
		Ability->GetActivationBlockedTags().HasTagExact(SwitchWeaponTag));

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

	UWeaponDefinition* WeaponA = NewObject<UWeaponDefinition>(Player);
	UWeaponDefinition* WeaponB = NewObject<UWeaponDefinition>(Player);
	UAbilityData* AbilityDataA = NewObject<UAbilityData>(WeaponA);
	UAbilityData* AbilityDataB = NewObject<UAbilityData>(WeaponB);
	WeaponA->AttackAbilityData = AbilityDataA;
	WeaponB->AttackAbilityData = AbilityDataB;
	UCharacterData* RuntimeCharacterData = NewObject<UCharacterData>(Player);
	RuntimeCharacterData->AbilityData = NewObject<UAbilityData>(RuntimeCharacterData);
	Player->GetCharacterDataComponent()->SetCharacterData(RuntimeCharacterData);

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

	Player->ApplyAbilityDataFromWeapon(WeaponA);
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
	UCharacterData* CharacterData = Player->GetCharacterDataComponent()
		? Player->GetCharacterDataComponent()->GetCharacterData()
		: nullptr;
	TestNotNull(TEXT("Player keeps runtime character data after switch"), CharacterData);
	TestTrue(TEXT("Promoted weapon ability data is merged after switch"),
		CharacterData && CharacterData->AbilityData && CharacterData->AbilityData != AbilityDataA);

	Player->EquippedWeaponInstance = nullptr;
	Player->InactiveWeaponInstance = nullptr;
	InstanceA->Destroy();
	InstanceB->Destroy();
	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPostAttackWindowAppliesRecoveryTagTest,
	"DevKit.CombatDeck.PostAttackWindowAppliesRecoveryTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPostAttackWindowAppliesRecoveryTagTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for post-attack window test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for post-attack window test"), Player);
	if (!Player)
	{
		return false;
	}

	UYogAbilitySystemComponent* ASC = Player->GetASC();
	TestNotNull(TEXT("Player ability system exists"), ASC);
	if (!ASC)
	{
		Player->Destroy();
		return false;
	}
	ASC->InitAbilityActorInfo(Player, Player);

	const FGameplayTag RecoveryTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.PostAttackRecovery"), false);
	TestTrue(TEXT("Post-attack recovery tag is registered"), RecoveryTag.IsValid());

	UAnimNotifyState_PostAtkWindow* Notify = NewObject<UAnimNotifyState_PostAtkWindow>(Player);
	USkeletalMeshComponent* Mesh = Player->GetMesh();
	FAnimNotifyEventReference EventReference;

	Notify->NotifyBegin(Mesh, nullptr, 0.25f, EventReference);
	TestTrue(TEXT("Post-attack notify begin adds recovery window tag"),
		RecoveryTag.IsValid() && ASC->GetTagCount(RecoveryTag) > 0);

	Notify->NotifyEnd(Mesh, nullptr, EventReference);
	TestEqual(TEXT("Post-attack notify end clears recovery window tag"),
		RecoveryTag.IsValid() ? ASC->GetTagCount(RecoveryTag) : 0, 0);

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSwitchWeaponRecoveryCancelClearsActiveSkillCooldownTest,
	"DevKit.CombatDeck.PlayerSwitchWeaponRecoveryCancelClearsActiveSkillCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerSwitchWeaponRecoveryCancelClearsActiveSkillCooldownTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for recovery cancel switch test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for recovery cancel switch test"), Player);
	if (!Player)
	{
		return false;
	}

	UYogAbilitySystemComponent* ASC = Player->GetASC();
	TestNotNull(TEXT("Player ability system exists"), ASC);
	if (!ASC)
	{
		Player->Destroy();
		return false;
	}
	ASC->InitAbilityActorInfo(Player, Player);

	UWeaponDefinition* WeaponA = NewObject<UWeaponDefinition>(Player);
	UWeaponDefinition* WeaponB = NewObject<UWeaponDefinition>(Player);
	Player->EquippedWeaponDef = WeaponA;
	Player->InactiveWeaponDef = WeaponB;

	UActiveSkillDataAsset* Skill = NewObject<UActiveSkillDataAsset>(Player);
	Skill->Config.SkillId = TEXT("RecoveryCancelSkill");
	Skill->Config.AbilityClass = UGA_ActiveSkill_ShieldBurst::StaticClass();
	Skill->Config.Cooldown = 10.0f;
	Player->ActiveSkillComponent->SetSkillLoadout({ Skill });
	Player->ActiveSkillComponent->SetUnlockedSlotCount(1);
	TestTrue(TEXT("Active skill can be used before recovery cancel switch"),
		Player->ActiveSkillComponent->UseActiveSkill());

	TArray<FActiveSkillSlotView> BeforeSwitchSlots = Player->ActiveSkillComponent->GetSlotViews();
	TestTrue(TEXT("Active skill is cooling down before recovery cancel switch"),
		BeforeSwitchSlots.IsValidIndex(0) && BeforeSwitchSlots[0].CooldownRemaining > 0.0f);

	const FGameplayTag RecoveryTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.PostAttackRecovery"), false);
	const FGameplayTag BonusTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.RecoveryCancelBonus"), false);
	TestTrue(TEXT("Post-attack recovery tag is registered"), RecoveryTag.IsValid());
	TestTrue(TEXT("Recovery-cancel bonus tag is registered"), BonusTag.IsValid());
	if (RecoveryTag.IsValid())
	{
		ASC->AddLooseGameplayTag(RecoveryTag);
	}

	Player->SwitchWeapon();

	TArray<FActiveSkillSlotView> AfterSwitchSlots = Player->ActiveSkillComponent->GetSlotViews();
	TestTrue(TEXT("Recovery-window weapon switch clears active skill cooldown"),
		AfterSwitchSlots.IsValidIndex(0) && FMath::IsNearlyZero(AfterSwitchSlots[0].CooldownRemaining));
	TestTrue(TEXT("Recovery-window weapon switch applies recovery cancel bonus tag"),
		BonusTag.IsValid() && ASC->GetTagCount(BonusTag) > 0);
	TestEqual(TEXT("Recovery-window weapon switch still promotes inactive weapon"),
		Player->EquippedWeaponDef.Get(), WeaponB);

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerActiveSkillSharedCooldownTagTest,
	"DevKit.CombatDeck.PlayerActiveSkillSharedCooldownTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerActiveSkillSharedCooldownTagTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for shared skill cooldown test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for shared skill cooldown test"), Player);
	if (!Player)
	{
		return false;
	}

	UYogAbilitySystemComponent* ASC = Player->GetASC();
	TestNotNull(TEXT("Player ability system exists"), ASC);
	if (!ASC)
	{
		Player->Destroy();
		return false;
	}
	ASC->InitAbilityActorInfo(Player, Player);

	const FGameplayTag SharedCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Cooldown.SkillShared"), false);
	TestTrue(TEXT("Shared skill cooldown tag is registered"), SharedCooldownTag.IsValid());

	Player->ActiveSkillComponent->StartSharedSkillCooldown(4.0f);
	TestTrue(TEXT("Shared skill cooldown remaining is tracked on active skill component"),
		Player->ActiveSkillComponent->GetSharedSkillCooldownRemaining() > 0.0f);
	TestTrue(TEXT("Shared skill cooldown tag is applied to ASC"),
		SharedCooldownTag.IsValid() && ASC->GetTagCount(SharedCooldownTag) > 0);

	Player->ActiveSkillComponent->ClearCooldowns();
	TestEqual(TEXT("Shared skill cooldown is cleared"), Player->ActiveSkillComponent->GetSharedSkillCooldownRemaining(), 0.0f);
	TestEqual(TEXT("Shared skill cooldown tag is cleared"),
		SharedCooldownTag.IsValid() ? ASC->GetTagCount(SharedCooldownTag) : 0, 0);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerRunStateCapturesIndependentWeaponDecksTest,
	"DevKit.CombatDeck.PlayerRunStateCapturesIndependentWeaponDecks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerRunStateCapturesIndependentWeaponDecksTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for run-state weapon deck capture test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for run-state weapon deck capture test"), Player);
	if (!Player)
	{
		return false;
	}

	auto MakeCardRune = [](UObject* Outer, const TCHAR* Name, ECombatDeckActionSlot Slot) -> URuneDataAsset*
	{
		URuneDataAsset* Rune = NewObject<URuneDataAsset>(Outer, FName(Name));
		Rune->RuneInfo.CombatCard = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
		Rune->RuneInfo.CombatCard.DisplayName = FText::FromString(Name);
		Rune->RuneInfo.CombatCard.RequiredActionSlot = Slot;
		return Rune;
	};

	URuneDataAsset* WeaponAAttack = MakeCardRune(Player, TEXT("RunStateWeaponAAttack"), ECombatDeckActionSlot::Attack);
	URuneDataAsset* WeaponASkill = MakeCardRune(Player, TEXT("RunStateWeaponASkill"), ECombatDeckActionSlot::Skill);
	URuneDataAsset* WeaponBAttack = MakeCardRune(Player, TEXT("RunStateWeaponBAttack"), ECombatDeckActionSlot::Attack);
	URuneDataAsset* WeaponBDash = MakeCardRune(Player, TEXT("RunStateWeaponBDash"), ECombatDeckActionSlot::Dash);

	UWeaponDefinition* WeaponA = NewObject<UWeaponDefinition>(Player);
	UWeaponDefinition* WeaponB = NewObject<UWeaponDefinition>(Player);
	WeaponA->InitialCombatDeck = { WeaponAAttack, WeaponASkill };
	WeaponB->InitialCombatDeck = { WeaponBAttack, WeaponBDash };
	WeaponA->ShuffleCooldownDuration = 1.5f;
	WeaponB->ShuffleCooldownDuration = 2.5f;
	WeaponA->MaxActiveSequenceSize = 1;
	WeaponB->MaxActiveSequenceSize = 2;

	Player->EquippedWeaponDef = WeaponA;
	Player->InactiveWeaponDef = WeaponB;
	Player->CombatDeckComponent->LoadDeckFromWeapon(WeaponA);
	Player->InitializeInactiveWeaponDeckStateFromDefinition();

	FRunState SavedState;
	Player->CaptureCombatLoadoutForRunState(SavedState);

	TestEqual(TEXT("RunState stores active weapon definition"), SavedState.EquippedWeaponDef.Get(), WeaponA);
	TestEqual(TEXT("RunState stores inactive weapon definition"), SavedState.InactiveWeaponDef.Get(), WeaponB);
	TestTrue(TEXT("RunState active deck includes attack card"), SavedState.CombatDeckCards.Contains(WeaponAAttack));
	TestTrue(TEXT("RunState active deck includes skill single-slot card"), SavedState.CombatDeckCards.Contains(WeaponASkill));
	TestTrue(TEXT("RunState inactive deck includes attack card"), SavedState.InactiveCombatDeckCards.Contains(WeaponBAttack));
	TestTrue(TEXT("RunState inactive deck includes dash single-slot card"), SavedState.InactiveCombatDeckCards.Contains(WeaponBDash));
	TestEqual(TEXT("RunState stores active shuffle cooldown"), SavedState.CombatDeckShuffleCooldownDuration, 1.5f);
	TestEqual(TEXT("RunState stores inactive shuffle cooldown"), SavedState.InactiveCombatDeckShuffleCooldownDuration, 2.5f);
	TestEqual(TEXT("RunState stores active max active sequence size"), SavedState.CombatDeckMaxActiveSequenceSize, 1);
	TestEqual(TEXT("RunState stores inactive max active sequence size"), SavedState.InactiveCombatDeckMaxActiveSequenceSize, 2);

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerRestoreRunStateRestoresInactiveWeaponDeckTest,
	"DevKit.CombatDeck.PlayerRestoreRunStateRestoresInactiveWeaponDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerRestoreRunStateRestoresInactiveWeaponDeckTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for inactive weapon run-state restore test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for inactive weapon run-state restore test"), Player);
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

	URuneDataAsset* WeaponACard = MakeAttackRune(Player, TEXT("RestoreWeaponACard"));
	URuneDataAsset* WeaponAReward = MakeAttackRune(Player, TEXT("RestoreWeaponAReward"));
	URuneDataAsset* WeaponBCard = MakeAttackRune(Player, TEXT("RestoreWeaponBCard"));
	URuneDataAsset* WeaponBReward = MakeAttackRune(Player, TEXT("RestoreWeaponBReward"));

	UWeaponDefinition* WeaponA = NewObject<UWeaponDefinition>(Player);
	UWeaponDefinition* WeaponB = NewObject<UWeaponDefinition>(Player);
	WeaponA->InitialCombatDeck = { WeaponACard };
	WeaponB->InitialCombatDeck = { WeaponBCard };

	FRunState State;
	State.bIsValid = true;
	State.EquippedWeaponDef = WeaponA;
	State.InactiveWeaponDef = WeaponB;
	State.CombatDeckCards = { WeaponACard, WeaponAReward };
	State.InactiveCombatDeckCards = { WeaponBCard, WeaponBReward };
	State.CombatDeckShuffleCooldownDuration = 0.0f;
	State.InactiveCombatDeckShuffleCooldownDuration = 0.0f;

	Player->RestoreRunState(State);

	TestEqual(TEXT("Restore keeps active weapon definition"), Player->EquippedWeaponDef.Get(), WeaponA);
	TestEqual(TEXT("Restore keeps inactive weapon definition"), Player->InactiveWeaponDef.Get(), WeaponB);
	TestTrue(TEXT("Restored active deck contains active reward"), Player->CombatDeckComponent->GetDeckSourceAssets().Contains(WeaponAReward));
	TestFalse(TEXT("Restored active deck does not contain inactive reward"), Player->CombatDeckComponent->GetDeckSourceAssets().Contains(WeaponBReward));

	Player->SwitchWeapon();

	TestEqual(TEXT("Switch after restore promotes inactive weapon definition"), Player->EquippedWeaponDef.Get(), WeaponB);
	TestTrue(TEXT("Switch after restore loads inactive reward deck"), Player->CombatDeckComponent->GetDeckSourceAssets().Contains(WeaponBReward));
	TestFalse(TEXT("Switch after restore does not leak active reward into inactive deck"), Player->CombatDeckComponent->GetDeckSourceAssets().Contains(WeaponAReward));

	Player->Destroy();
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

	EInputCommandType ConsumedType = EInputCommandType::Attack;
	const float SinceTime = World->GetTimeSeconds() - 1.0f;
	Buffer->RecordAttack();
	Buffer->RecordWeaponSkill();
	TestTrue(TEXT("Attack-only input is consumed"), Buffer->ConsumeLatestAttackInputSince(SinceTime, ConsumedType));
	TestEqual(TEXT("Attack-only consume skips WeaponSkill"), ConsumedType, EInputCommandType::Attack);

	Buffer->ClearBuffer();
	Buffer->RecordAttack();
	Buffer->RecordSkill();
	TestTrue(TEXT("Latest action input is consumed"), Buffer->ConsumeLatestActionInputSince(SinceTime, ConsumedType));
	TestEqual(TEXT("Skill wins when it was recorded after Attack"), ConsumedType, EInputCommandType::Skill);

	Buffer->ClearBuffer();
	Buffer->RecordSkill();
	Buffer->RecordWeaponSkill();
	TestTrue(TEXT("Latest action input is consumed after order swap"), Buffer->ConsumeLatestActionInputSince(SinceTime, ConsumedType));
	TestEqual(TEXT("WeaponSkill wins when it was recorded after Skill"), ConsumedType, EInputCommandType::WeaponSkill);

	Buffer->ClearBuffer();
	Buffer->RecordSpecial();
	TestTrue(TEXT("Deprecated RecordSpecial is consumed as Skill"), Buffer->ConsumeLatestActionInputSince(SinceTime, ConsumedType));
	TestEqual(TEXT("Deprecated Special alias records Skill"), ConsumedType, EInputCommandType::Skill);

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

	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo3"));
	const FGameplayTag BranchTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAbilityMontageDataSeedsExplicitActionComboTagsTest,
	"DevKit.CombatDeck.PlayerAbilityMontageDataSeedsExplicitActionComboTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerAbilityMontageDataSeedsExplicitActionComboTagsTest::RunTest(const FString& Parameters)
{
	UPlayerAbilityMontageData* AbilityData = NewObject<UPlayerAbilityMontageData>();

	static const TCHAR* ActionNames[] = {
		TEXT("Skill.Attack"),
		TEXT("Skill.WeaponSkill"),
		TEXT("Movement.Dash"),
	};

	for (const TCHAR* ActionName : ActionNames)
	{
		for (int32 ComboIndex = 1; ComboIndex <= 4; ++ComboIndex)
		{
			const FString TagName = FString::Printf(
				TEXT("Character.State.%s.Combo%d"),
				ActionName,
				ComboIndex);
			const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(FName(*TagName));
			TestTrue(
				FString::Printf(TEXT("Player ability montage data has %s"), *TagName),
				AbilityData->MontageMap.Contains(ComboTag));
		}
	}

	TestTrue(TEXT("Player ability montage data has active skill keys"),
		AbilityData->MontageMap.Contains(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Active.Skill1")))
		&& AbilityData->MontageMap.Contains(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Active.Skill2"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSplitAbilityMontageDataSeedsScopedComboTagsTest,
	"DevKit.CombatDeck.SplitAbilityMontageDataSeedsScopedComboTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSplitAbilityMontageDataSeedsScopedComboTagsTest::RunTest(const FString& Parameters)
{
	UWeaponAttackAbilityMontageData* WeaponAttackData = NewObject<UWeaponAttackAbilityMontageData>();
	UWeaponSkillAbilityMontageData* WeaponSkillData = NewObject<UWeaponSkillAbilityMontageData>();
	USpecialAbilityMontageData* SpecialData = NewObject<USpecialAbilityMontageData>();
	UWeaponPassiveAbilityMontageData* PassiveData = NewObject<UWeaponPassiveAbilityMontageData>();

	auto HasKey = [](const UAbilityData* AbilityData, const TCHAR* TagName)
	{
		return AbilityData && AbilityData->MontageMap.Contains(FGameplayTag::RequestGameplayTag(FName(TagName)));
	};
	auto HasPassiveKey = [](const UAbilityData* AbilityData, const TCHAR* TagName)
	{
		return AbilityData && AbilityData->PassiveMap.Contains(FGameplayTag::RequestGameplayTag(FName(TagName)));
	};

	TestTrue(TEXT("WeaponAttack data has Attack combo keys"),
		HasKey(WeaponAttackData, TEXT("Character.State.Skill.Attack.Combo1"))
		&& HasKey(WeaponAttackData, TEXT("Character.State.Skill.Attack.Combo4")));
	TestTrue(TEXT("WeaponAttack data has Dash combo keys"),
		HasKey(WeaponAttackData, TEXT("Character.State.Movement.Dash.Combo1"))
		&& HasKey(WeaponAttackData, TEXT("Character.State.Movement.Dash.Combo4")));
	TestTrue(TEXT("WeaponAttack data has SwitchWeapon key"),
		HasKey(WeaponAttackData, TEXT("Character.State.Equipment.SwitchWeapon")));
	TestFalse(TEXT("WeaponAttack data does not seed WeaponSkill combo keys"),
		HasKey(WeaponAttackData, TEXT("Character.State.Skill.WeaponSkill.Combo1")));
	TestTrue(TEXT("WeaponAttack data has no passive rows"),
		WeaponAttackData->PassiveMap.IsEmpty());

	TestTrue(TEXT("WeaponSkill data has WeaponSkill combo keys"),
		HasKey(WeaponSkillData, TEXT("Character.State.Skill.WeaponSkill.Combo1"))
		&& HasKey(WeaponSkillData, TEXT("Character.State.Skill.WeaponSkill.Combo4")));
	TestFalse(TEXT("WeaponSkill data does not seed Attack combo keys"),
		HasKey(WeaponSkillData, TEXT("Character.State.Skill.Attack.Combo1")));
	TestTrue(TEXT("WeaponSkill data has no passive rows"),
		WeaponSkillData->PassiveMap.IsEmpty());

	TestFalse(TEXT("Special data no longer seeds deprecated Special combo keys"),
		HasKey(SpecialData, TEXT("PlayerState.AbilityCast.Special.Combo1"))
		|| HasKey(SpecialData, TEXT("PlayerState.AbilityCast.Special.Combo4")));
	TestFalse(TEXT("Special data does not seed Dash combo keys"),
		HasKey(SpecialData, TEXT("Character.State.Movement.Dash.Combo1")));
	TestTrue(TEXT("Special data has no passive rows"),
		SpecialData->PassiveMap.IsEmpty());
	TestTrue(TEXT("WeaponPassive data has hit/death passive keys"),
		HasPassiveKey(PassiveData, TEXT("Action.HitReact.Front"))
		&& HasPassiveKey(PassiveData, TEXT("Action.HitReact.Back"))
		&& HasPassiveKey(PassiveData, TEXT("Action.HitReact.Blocked"))
		&& HasPassiveKey(PassiveData, TEXT("Action.HitReact.Parried"))
		&& HasPassiveKey(PassiveData, TEXT("Action.Dead")));
	TestFalse(TEXT("WeaponPassive data does not seed Attack combo keys"),
		HasKey(PassiveData, TEXT("Character.State.Skill.Attack.Combo1")));
	TestTrue(TEXT("WeaponPassive data has no montage config rows"),
		PassiveData->MontageConfigMap.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerWeaponPassiveAbilityDataOverridesBaseReactionTest,
	"DevKit.CombatDeck.PlayerWeaponPassiveAbilityDataOverridesBaseReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerWeaponPassiveAbilityDataOverridesBaseReactionTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for weapon passive ability data test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned for weapon passive ability data test"), Player);
	if (!Player)
	{
		return false;
	}

	UAbilityData* BaseAbilityData = NewObject<UAbilityData>(Player);
	UCharacterData* RuntimeCharacterData = NewObject<UCharacterData>(Player);
	RuntimeCharacterData->AbilityData = BaseAbilityData;
	Player->GetCharacterDataComponent()->SetCharacterData(RuntimeCharacterData);

	UWeaponDefinition* Weapon = NewObject<UWeaponDefinition>(Player);
	UWeaponPassiveAbilityMontageData* WeaponPassiveData = NewObject<UWeaponPassiveAbilityMontageData>(Weapon);
	Weapon->PassiveAbilityData = WeaponPassiveData;

	const FGameplayTag HitReactFrontTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Front"));
	const FGameplayTag HitReactBackTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Back"));

	UAnimMontage* BaseFrontMontage = NewObject<UAnimMontage>(BaseAbilityData);
	UAnimMontage* BaseBackMontage = NewObject<UAnimMontage>(BaseAbilityData);
	UAnimMontage* WeaponFrontMontage = NewObject<UAnimMontage>(WeaponPassiveData);

	FPassiveActionData BaseFrontReaction;
	BaseFrontReaction.Montage = BaseFrontMontage;
	BaseAbilityData->PassiveMap.Add(HitReactFrontTag, BaseFrontReaction);

	FPassiveActionData BaseBackReaction;
	BaseBackReaction.Montage = BaseBackMontage;
	BaseAbilityData->PassiveMap.Add(HitReactBackTag, BaseBackReaction);

	FPassiveActionData WeaponFrontReaction;
	WeaponFrontReaction.Montage = WeaponFrontMontage;
	WeaponPassiveData->PassiveMap.Add(HitReactFrontTag, WeaponFrontReaction);

	Player->ApplyAbilityDataFromWeapon(Weapon);

	UCharacterData* CharacterData = Player->GetCharacterDataComponent()->GetCharacterData();
	TestNotNull(TEXT("Player has runtime character data"), CharacterData);
	TestNotNull(TEXT("Player has merged runtime ability data"), CharacterData ? CharacterData->AbilityData.Get() : nullptr);

	if (CharacterData && CharacterData->AbilityData)
	{
		TestEqual(TEXT("Weapon passive reaction overrides base hit react front"),
			CharacterData->AbilityData->GetPassiveAbility(HitReactFrontTag).Montage.Get(), WeaponFrontMontage);
		TestEqual(TEXT("Base passive reaction remains as fallback when weapon has no row"),
			CharacterData->AbilityData->GetPassiveAbility(HitReactBackTag).Montage.Get(), BaseBackMontage);
	}

	Player->Destroy();
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

	const FGameplayTag BranchTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponComboHintTextUnlimitedLinesTest,
	"DevKit.CombatDeck.WeaponComboHintTextUnlimitedLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponComboHintTextUnlimitedLinesTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Weapon = NewObject<UWeaponDefinition>();
	UAbilityData* AttackData = NewObject<UAbilityData>(Weapon);
	UAbilityData* WeaponSkillData = NewObject<UAbilityData>(Weapon);
	Weapon->AttackAbilityData = AttackData;
	Weapon->WeaponSkillAbilityData = WeaponSkillData;

	auto AddMontageKey = [](UAbilityData* AbilityData, const TCHAR* TagName)
	{
		const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TagName));
		AbilityData->MontageMap.Add(AbilityTag, NewObject<UAnimMontage>(AbilityData));
	};

	AddMontageKey(AttackData, TEXT("Character.State.Skill.Attack.Combo1"));
	AddMontageKey(AttackData, TEXT("Character.State.Skill.Attack.Combo2"));
	AddMontageKey(AttackData, TEXT("Character.State.Movement.Dash.Combo1"));
	AddMontageKey(WeaponSkillData, TEXT("Character.State.Skill.WeaponSkill.Combo1"));
	AddMontageKey(WeaponSkillData, TEXT("Character.State.Skill.WeaponSkill.Combo2"));

	const FString LimitedText = WeaponComboTextUtils::BuildComboHintText(Weapon, 1, true).ToString();
	TestTrue(TEXT("Explicit line limit includes configured attack combo"), LimitedText.Contains(TEXT("Attack")));
	TestFalse(TEXT("Explicit line limit truncates later ability-data lines"), LimitedText.Contains(TEXT("WeaponSkill")));

	const FString UnlimitedText = WeaponComboTextUtils::BuildComboHintText(Weapon, 0, true).ToString();
	TestTrue(TEXT("Zero line limit shows weapon skill combo line"), UnlimitedText.Contains(TEXT("WeaponSkill")));
	TestFalse(TEXT("Zero line limit omits deprecated special combo line"), UnlimitedText.Contains(TEXT("Special")));

	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckContextDedupesCommitAndHitTest,
	"DevKit.CombatDeck.ContextUsesOneCardPerAttackHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckContextDedupesCommitAndHitTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
		FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any },
	});

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Any;
	Context.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	Context.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult CommitResult = Deck->ResolveAttackCardWithContext(Context);
	TestFalse(TEXT("OnCommit preview does not advance the visible card sequence"), CommitResult.bHadCard);
	TestEqual(TEXT("Sequence still points at the first card after OnCommit preview"), Deck->GetCurrentIndex(), 0);

	Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	const FCombatCardResolveResult HitResult = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("OnHit resolves one card for the attack"), HitResult.bHadCard);
	TestEqual(TEXT("Sequence advances once for the attack hit"), Deck->GetCurrentIndex(), 1);

	const FCombatCardResolveResult DuplicateHitResult = Deck->ResolveAttackCardWithContext(Context);
	TestFalse(TEXT("Second OnHit for the same attack guid does not resolve another card"), DuplicateHitResult.bHadCard);
	TestEqual(TEXT("Visible card sequence keeps every card after a resolve"), Deck->GetRemainingDeckSnapshot().Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckOnHitCardIgnoresCommitTest,
	"DevKit.CombatDeck.OnHitCardResolvesOnlyOnHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckOnHitCardIgnoresCommitTest::RunTest(const FString& Parameters)
{
	FCombatCardConfig OnHitCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	OnHitCard.TriggerTiming = ECombatCardTriggerTiming::OnHit;

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({ OnHitCard });

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Any;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	Context.TriggerTiming = ECombatCardTriggerTiming::OnCommit;

	const FCombatCardResolveResult CommitResult = Deck->ResolveAttackCardWithContext(Context);
	TestFalse(TEXT("OnHit card is not resolved by OnCommit"), CommitResult.bHadCard);
	TestEqual(TEXT("Card remains available after ignored OnCommit"), Deck->GetRemainingDeckSnapshot().Num(), 1);

	Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	const FCombatCardResolveResult HitResult = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("OnHit card is resolved by OnHit"), HitResult.bHadCard);
	TestTrue(TEXT("OnHit card triggers base release"), HitResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Visible card sequence keeps the resolved OnHit card"), Deck->GetRemainingDeckSnapshot().Num(), 1);

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
	Context.ActionType = ECardRequiredAction::Any;
	Context.ComboIndex = 3;
	Context.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult Result = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("Default card resolves"), Result.bHadCard);
	TestEqual(TEXT("Default card does not receive combo scaling"), Result.AppliedMultiplier, 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckDeprecatedComboEffectScalingIgnoredTest,
	"DevKit.CombatDeck.DeprecatedComboEffectScalingIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckDeprecatedComboEffectScalingIgnoredTest::RunTest(const FString& Parameters)
{
	FCombatCardConfig AttackCard{ ECombatCardType::Attack, ECardRequiredAction::Any };
	AttackCard.bUseComboEffectScaling = true;
	AttackCard.ComboScalarPerIndex = 0.25f;
	AttackCard.MaxComboScalar = 0.5f;

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();
	Deck->SetDeckListForTest({ AttackCard, AttackCard, AttackCard });

	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Any;

	Context.ComboIndex = 1;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult FirstResult = Deck->ResolveAttackCardWithContext(Context);
	TestEqual(TEXT("Deprecated combo scaling keeps Combo1 unchanged"), FirstResult.AppliedMultiplier, 1.0f);

	Context.ComboIndex = 2;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult SecondResult = Deck->ResolveAttackCardWithContext(Context);
	TestEqual(TEXT("Deprecated combo scaling is ignored for Combo2"), SecondResult.AppliedMultiplier, 1.0f);

	Context.ComboIndex = 4;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult CappedResult = Deck->ResolveAttackCardWithContext(Context);
	TestEqual(TEXT("Deprecated combo scaling is ignored at cap"), CappedResult.AppliedMultiplier, 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkMultiplierIgnoresDeprecatedComboScalingTest,
	"DevKit.CombatDeck.LinkMultiplierIgnoresDeprecatedComboScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkMultiplierIgnoresDeprecatedComboScalingTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Attack"));

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
	Context.ActionType = ECardRequiredAction::Any;
	Context.ComboIndex = 1;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	Deck->ResolveAttackCardWithContext(Context);

	Context.ComboIndex = 3;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult LinkResult = Deck->ResolveAttackCardWithContext(Context);
	TestTrue(TEXT("Forward link triggers"), LinkResult.bTriggeredForwardLink);
	TestEqual(TEXT("Link multiplier ignores deprecated combo multiplier"), LinkResult.AppliedMultiplier, 1.5f);

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
	ResolveResult.ResolvedCard = AttackCard;
	ResolveResult.ConsumedCard = AttackCard;
	ResolveResult.AppliedMultiplier = 1.5f;

	UBuffFlowComponent* BuffFlowComponent = NewObject<UBuffFlowComponent>();
	BuffFlowComponent->StartCombatCardFlow(nullptr, AttackCard, ActionContext, ResolveResult, nullptr, true);

	TestTrue(TEXT("Combat-card context is marked available"), BuffFlowComponent->HasCombatCardEffectContext());
	const FCombatCardEffectContext& StoredContext = BuffFlowComponent->GetLastCombatCardEffectContext();
	TestEqual(TEXT("Stored combo index"), StoredContext.ComboIndex, 3);
	TestEqual(TEXT("Stored combo bonus stacks are deprecated and disabled"), StoredContext.ComboBonusStacks, 0);
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

	TestTrue(TEXT("Active skills resolve the player skill slot by default"),
		ActiveSkill->Config.bResolveCombatDeckOnUse);
	TestEqual(TEXT("Active skill combat deck slot is skill"), ActiveSkill->Config.CombatDeckActionSlot, ECombatDeckActionSlot::Skill);
	TestEqual(TEXT("Active skill default flow role is catalyst"), ActiveSkill->Config.CombatDeckFlowRole, ECombatDeckFlowRole::Catalyst);
	TestEqual(TEXT("Active skill default card timing is OnCommit"), ActiveSkill->Config.CombatDeckTriggerTiming, ECombatCardTriggerTiming::OnCommit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerActionAbilitiesExposeCombatDeckIntentTest,
	"DevKit.CombatDeck.PlayerActionAbilitiesExposeCombatDeckIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerActionAbilitiesExposeCombatDeckIntentTest::RunTest(const FString& Parameters)
{
	UGA_MeleeAttack* Attack = NewObject<UGA_MeleeAttack>();
	TestEqual(TEXT("Melee attack no longer classifies cards as light/heavy"),
		Attack->GetCombatDeckActionType(), ECardRequiredAction::Any);
	TestEqual(TEXT("Melee attack lets attack cards author starter/catalyst roles"),
		Attack->GetCombatDeckFlowRole(), ECombatDeckFlowRole::Any);
	Attack->AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo4"), false));
	TestFalse(TEXT("Melee attack combo tags no longer promote combat deck cards to finishers"),
		Attack->IsCombatDeckComboFinisher());
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"), false);
	TestTrue(TEXT("Attack combo 1 remains the broad attack entry point"),
		NewObject<UGA_PlayerAttack_Combo1>()->AbilityTags.HasTagExact(AttackTag));
	TestFalse(TEXT("Attack combo 2 no longer responds to broad attack input"),
		NewObject<UGA_PlayerAttack_Combo2>()->AbilityTags.HasTagExact(AttackTag));

	UGA_WeaponSkill* WeaponSkill = NewObject<UGA_WeaponSkill>();
	TestTrue(TEXT("Weapon skill resolves combat deck cards"),
		WeaponSkill->ShouldResolveCombatDeck());
	TestEqual(TEXT("Weapon skill uses the weapon skill card slot"),
		WeaponSkill->GetCombatDeckActionSlot(), ECombatDeckActionSlot::WeaponSkill);
	TestEqual(TEXT("Weapon skill is the default detonate/finisher action"),
		WeaponSkill->GetCombatDeckFlowRole(), ECombatDeckFlowRole::Finisher);
	TestEqual(TEXT("Weapon skill can resolve on-commit cards for non-hit skills"),
		WeaponSkill->GetCombatDeckCommitTiming(), ECombatCardTriggerTiming::OnCommit);
	const FGameplayTag SharedCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Cooldown.SkillShared"), false);
	TestTrue(TEXT("Weapon skill is blocked by the shared Skill/WeaponSkill cooldown"),
		SharedCooldownTag.IsValid() && WeaponSkill->GetActivationBlockedTags().HasTagExact(SharedCooldownTag));
	const FGameplayTag WeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"), false);
	TestTrue(TEXT("Weapon skill combo 1 remains the broad weapon skill entry point"),
		NewObject<UGA_WeaponSkill_Combo1>()->AbilityTags.HasTagExact(WeaponSkillTag));
	TestFalse(TEXT("Weapon skill combo 2 no longer responds to broad weapon skill input"),
		NewObject<UGA_WeaponSkill_Combo2>()->AbilityTags.HasTagExact(WeaponSkillTag));

	const TArray<FGameplayTag> MusketWeaponSkillReleaseTags = UGA_Musket_HeavyAttack::GetReleaseEventTags();
	const FGameplayTag WeaponSkillReleaseTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.Release"), false);
	const FGameplayTag DeprecatedHeavyReleaseTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.Musket.HeavyRelease"), false);
	TestEqual(TEXT("Musket weapon skill listens to exactly one release event"), MusketWeaponSkillReleaseTags.Num(), 1);
	TestTrue(TEXT("Musket weapon skill release uses the broad WeaponSkill release event"),
		WeaponSkillReleaseTag.IsValid() && MusketWeaponSkillReleaseTags.Contains(WeaponSkillReleaseTag));
	TestFalse(TEXT("Musket weapon skill no longer listens to deprecated heavy-release events"),
		DeprecatedHeavyReleaseTag.IsValid() && MusketWeaponSkillReleaseTags.Contains(DeprecatedHeavyReleaseTag));

	UGA_PlayerDash* Dash = NewObject<UGA_PlayerDash>();
	TestTrue(TEXT("Dash resolves combat deck cards"),
		Dash->ShouldResolveCombatDeck());
	TestEqual(TEXT("Dash uses the dash card slot"),
		Dash->GetCombatDeckActionSlot(), ECombatDeckActionSlot::Dash);
	TestEqual(TEXT("Dash is a catalyst/cancel action"),
		Dash->GetCombatDeckFlowRole(), ECombatDeckFlowRole::Catalyst);
	TestEqual(TEXT("Dash resolves its single-slot card on successful dash commit"),
		Dash->GetCombatDeckTriggerTiming(), ECombatCardTriggerTiming::OnCommit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckLinkReadPreviousTest,
	"DevKit.CombatDeck.LinkReadPreviousUsesLastResolvedCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckLinkReadPreviousTest::RunTest(const FString& Parameters)
{
	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig First{ ECombatCardType::Attack, ECardRequiredAction::Any };
	FCombatCardConfig ReadPrevious{ ECombatCardType::Link, ECardRequiredAction::Any };
	ReadPrevious.LinkMode = ECardLinkMode::ReadPrevious;

	Deck->SetDeckListForTest({ First, ReadPrevious });

	const FCombatCardResolveResult FirstResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("First card resolves normally"), FirstResult.bHadCard);

	const FCombatCardResolveResult LinkResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
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

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Attack card resolves before Moonlight"), AttackResult.bHadCard);

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
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
	MoonlightContext.ActionType = ECardRequiredAction::Any;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Moonlight is resolved first"), MoonlightResult.bHadCard);
	TestTrue(TEXT("Moonlight opens a pending backward link"), MoonlightResult.bPendingBackwardLink);

	FCombatDeckActionContext AttackContext = MoonlightContext;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(AttackContext);
	TestTrue(TEXT("Next attack card is resolved"), AttackResult.bHadCard);
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
	MoonlightCard.Config.CardIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Moonlight"), false);

	FCombatCardInstance TargetCard;
	TargetCard.Config = FCombatCardConfig{ ECombatCardType::Attack, ECardRequiredAction::Any };
	TargetCard.Config.CardIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.AttackUp"), false);

	FCombatCardResolveResult Result;
	Result.ResolvedCard = TargetCard;
	Result.ConsumedCard = TargetCard;
	Result.LinkedSourceCard = MoonlightCard;
	Result.LinkedTargetCard = TargetCard;
	Result.bTriggeredBackwardLink = true;

	const FCombatCardInstance ReplaySourceCard = USacrificeRuneComponent::ResolveShadowReplaySourceCardForTest(Result);
	TestEqual(TEXT("Backward Moonlight replay uses the linked source card"), ReplaySourceCard.Config.CardIdTag, MoonlightCard.Config.CardIdTag);
	TestNotEqual(TEXT("Backward Moonlight replay does not use the resolved target card"), ReplaySourceCard.Config.CardIdTag, TargetCard.Config.CardIdTag);

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
	MoonlightContext.ActionType = ECardRequiredAction::Any;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bConsumeOnCommit = true;
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

	Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Both-direction link triggers forward when previous attack matches"), MoonlightResult.bTriggeredForwardLink);
	TestFalse(TEXT("Forward match does not also open a backward pending link"), MoonlightResult.bPendingBackwardLink);

	const FCombatCardResolveResult FinalAttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
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
		const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
		TestTrue(FString::Printf(TEXT("Moonlight %d is resolved"), Index + 1), Result.bHadCard);
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

	const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Normal card is resolved"), Result.bHadCard);
	TestTrue(TEXT("Normal card triggers BaseFlow"), Result.bTriggeredBaseFlow);
	TestFalse(TEXT("Normal card does not trigger link"), Result.bTriggeredLink);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeForwardUsesEffectTagsTest,
	"DevKit.CombatDeck.RecipeForwardUsesEffectTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeForwardUsesEffectTagsTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Attack"));
	const FGameplayTag MoonlightIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Moonlight"));

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

	Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);

	TestTrue(TEXT("Moonlight forward recipe is triggered by previous Buff.Attack"), MoonlightResult.bTriggeredForwardLink);
	TestFalse(TEXT("Forward recipe does not release base flow"), MoonlightResult.bTriggeredBaseFlow);
	TestEqual(TEXT("Forward recipe multiplier is reported"), MoonlightResult.AppliedMultiplier, 1.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeForwardAcceptsLegacyNeighborIdTest,
	"DevKit.CombatDeck.RecipeForwardAcceptsLegacyNeighborId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeForwardAcceptsLegacyNeighborIdTest::RunTest(const FString& Parameters)
{
	const FGameplayTag LegacyAttackUpIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.AttackUp"), false);
	const FGameplayTag AttackUpIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.AttackUp"), false);
	TestTrue(TEXT("Legacy Card.ID.AttackUp tag exists for migration compatibility"), LegacyAttackUpIdTag.IsValid());
	TestTrue(TEXT("Formal Buff.AttackUp tag exists"), AttackUpIdTag.IsValid());
	if (!LegacyAttackUpIdTag.IsValid() || !AttackUpIdTag.IsValid())
	{
		return false;
	}

	UCombatDeckComponent* Deck = NewObject<UCombatDeckComponent>();

	FCombatCardConfig AttackUpCard{ ECombatCardType::Normal, ECardRequiredAction::Any };
	AttackUpCard.CardIdTag = LegacyAttackUpIdTag;

	FCombatCardConfig MoonlightCard{ ECombatCardType::Link, ECardRequiredAction::Any };
	MoonlightCard.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;

	FCombatCardLinkRecipe Recipe;
	Recipe.Direction = ECombatCardLinkOrientation::Forward;
	Recipe.LinkFlow = NewObject<UFlowAsset>();
	Recipe.Condition.RequiredNeighborIdTags.AddTag(AttackUpIdTag);
	MoonlightCard.LinkRecipes.Add(Recipe);

	Deck->SetDeckListForTest({ AttackUpCard, MoonlightCard });

	Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);

	TestTrue(TEXT("Forward recipe treats legacy Card.ID neighbor as formal Buff"), MoonlightResult.bTriggeredForwardLink);
	TestFalse(TEXT("Forward recipe does not release base flow after matched recipe"), MoonlightResult.bTriggeredBaseFlow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedUsesEffectTagsTest,
	"DevKit.CombatDeck.RecipeReversedUsesEffectTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedUsesEffectTagsTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Attack"));

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
	MoonlightContext.ActionType = ECardRequiredAction::Any;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bConsumeOnCommit = true;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link"), MoonlightResult.bPendingBackwardLink);
	TestFalse(TEXT("Reversed Moonlight does not release base flow while pending"), MoonlightResult.bTriggeredBaseFlow);

	FCombatDeckActionContext AttackContext = MoonlightContext;
	AttackContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(AttackContext);
	TestTrue(TEXT("Next Buff.Attack triggers reversed Moonlight recipe"), AttackResult.bTriggeredBackwardLink);
	TestEqual(TEXT("Reversed recipe multiplier is reported"), AttackResult.AppliedMultiplier, 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedCanStartFromFirstAttackTest,
	"DevKit.CombatDeck.RecipeReversedCanStartFromFirstAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedCanStartFromFirstAttackTest::RunTest(const FString& Parameters)
{
	const FGameplayTag SplashSplitEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.SplashSplit"));

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
	MoonlightContext.ActionType = ECardRequiredAction::Any;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bConsumeOnCommit = true;
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
	const FGameplayTag BurnEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Fire"));
	const FGameplayTag PoisonEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Poison"));
	const FGameplayTag ShieldEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Shield"));

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

	ForwardDeck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	const FCombatCardResolveResult ForwardResult = ForwardDeck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Forward Moonlight recipe triggers on the matching poison card"), ForwardResult.bTriggeredForwardLink);
	TestEqual(TEXT("Forward Moonlight selects the poison recipe multiplier"), ForwardResult.AppliedMultiplier, 1.4f);

	FCombatCardConfig MoonlightReversed = MoonlightForward;
	MoonlightReversed.DefaultLinkOrientation = ECombatCardLinkOrientation::Reversed;

	UCombatDeckComponent* ReversedDeck = NewObject<UCombatDeckComponent>();
	ReversedDeck->SetDeckListForTest({ MoonlightReversed, ShieldCard });

	FCombatDeckActionContext ReversedMoonlightContext;
	ReversedMoonlightContext.ActionType = ECardRequiredAction::Any;
	ReversedMoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	ReversedMoonlightContext.bConsumeOnCommit = true;
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
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Attack"));

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
	MoonlightContext.ActionType = ECardRequiredAction::Any;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	MoonlightContext.bConsumeOnCommit = true;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link"), MoonlightResult.bPendingBackwardLink);

	Deck->NotifyComboStateExited();

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	TestTrue(TEXT("Next attack restarts from the first card"), AttackResult.bHadCard);
	TestEqual(TEXT("Combo exit returns the sequence to Moonlight"), AttackResult.ResolvedCard.Config.CardType, ECombatCardType::Link);
	TestFalse(TEXT("Pending reversed link is cleared after combo exit"), AttackResult.bTriggeredBackwardLink);
	TestTrue(TEXT("Restarted first card opens its own pending reversed link"), AttackResult.bPendingBackwardLink);
	TestEqual(TEXT("Restarted combo advances to the second card after using Moonlight"), Deck->GetCurrentIndex(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckRecipeReversedRequiresComboContinuationTest,
	"DevKit.CombatDeck.RecipeReversedRequiresComboContinuation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckRecipeReversedRequiresComboContinuationTest::RunTest(const FString& Parameters)
{
	const FGameplayTag AttackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Attack"));

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
	MoonlightContext.ActionType = ECardRequiredAction::Any;
	MoonlightContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	MoonlightContext.bComboContinued = true;
	MoonlightContext.AttackInstanceGuid = FGuid::NewGuid();

	const FCombatCardResolveResult MoonlightResult = Deck->ResolveAttackCardWithContext(MoonlightContext);
	TestTrue(TEXT("Reversed Moonlight opens a pending recipe link from card sequence"), MoonlightResult.bPendingBackwardLink);

	FCombatDeckActionContext RestartedAttackContext = MoonlightContext;
	RestartedAttackContext.AttackInstanceGuid = FGuid::NewGuid();
	RestartedAttackContext.bComboContinued = false;

	const FCombatCardResolveResult AttackResult = Deck->ResolveAttackCardWithContext(RestartedAttackContext);
	TestTrue(TEXT("Restarted attack starts again from the first card"), AttackResult.bHadCard);
	TestEqual(TEXT("Non-continuation resets the sequence to Moonlight"), AttackResult.ResolvedCard.Config.CardType, ECombatCardType::Link);
	TestFalse(TEXT("Pending reversed link is cleared when the next attack is not a combo continuation"), AttackResult.bTriggeredBackwardLink);
	TestTrue(TEXT("Restarted first card opens a new pending reversed link"), AttackResult.bPendingBackwardLink);
	TestEqual(TEXT("Restarted combo advances to the second card after using Moonlight"), Deck->GetCurrentIndex(), 1);

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
	const FGameplayTag MoonlightEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Moonlight"));

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

	Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);
	const FCombatCardResolveResult Result = Deck->ResolveAttackCard(ECardRequiredAction::Any, false, false);

	TestTrue(TEXT("Second link card is resolved"), Result.bHadCard);
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
		TEXT("Buff.Fire"),
		TEXT("Buff.Poison"),
		TEXT("Buff.Shield"),
		TEXT("Buff.Pierce"),
		TEXT("Buff.Attack"),
		TEXT("Buff.ReduceDamage"),
	};
	TArray<FName> ReversedEffectTagNames = ForwardEffectTagNames;
	ReversedEffectTagNames.Add(TEXT("Buff.SplashSplit"));

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
		ForwardNeighborContext.ActionType = ECardRequiredAction::Any;
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
		ReversedMoonlightContext.ActionType = ECardRequiredAction::Any;
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
		ReversedMoonlightContext.ActionType = ECardRequiredAction::Any;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedCardOrderAssetsConfiguredTest,
	"DevKit.CombatDeck.GeneratedCardOrderAssetsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedCardOrderAssetsConfiguredTest::RunTest(const FString& Parameters)
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
	UCombatDeckComponent* RuntimeDeck = NewObject<UCombatDeckComponent>();
	RuntimeDeck->SetDeckListForTest({ AttackCard });
	FCombatDeckActionContext DeprecatedComboContext;
	DeprecatedComboContext.ActionSlot = AttackCard.RequiredActionSlot;
	DeprecatedComboContext.FlowRole = AttackCard.RequiredFlowRole == ECombatDeckFlowRole::Any
		? ECombatDeckFlowRole::Starter
		: AttackCard.RequiredFlowRole;
	DeprecatedComboContext.TriggerTiming = AttackCard.TriggerTiming;
	DeprecatedComboContext.bConsumeOnCommit = DeprecatedComboContext.TriggerTiming == ECombatCardTriggerTiming::OnCommit;
	DeprecatedComboContext.ComboIndex = 4;
	DeprecatedComboContext.AttackInstanceGuid = FGuid::NewGuid();
	const FCombatCardResolveResult DeprecatedComboResult = RuntimeDeck->ResolveAttackCardWithContext(DeprecatedComboContext);
	TestTrue(TEXT("Generated attack card still resolves"), DeprecatedComboResult.bHadCard);
	TestEqual(TEXT("Generated attack card ignores deprecated combo scaling at runtime"), DeprecatedComboResult.AppliedMultiplier, 1.0f);

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
	if (ApplyAttributeNodeCount == 0)
	{
		TestTrue(TEXT("Attack card references generated attack base flow"), AttackCard.BaseFlow == AttackFlow);
	}

	UFlowAsset* MoonlightBaseFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Moonlight_Base.FA_Rune512_Moonlight_Base"));
	TestNotNull(TEXT("Generated moonlight base flow exists"), MoonlightBaseFlow);
	if (!MoonlightBaseFlow)
	{
		return false;
	}

	UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
	UBFNode_SpawnRuneProjectileProfile* ProjectileProfileNode = nullptr;
	UBFNode_SpawnBuffFlowProjectile* BuffFlowProjectileNode = nullptr;
	for (const TPair<FGuid, UFlowNode*>& Pair : MoonlightBaseFlow->GetNodes())
	{
		if (!SlashNode)
		{
			SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
		}
		if (!ProjectileProfileNode)
		{
			ProjectileProfileNode = Cast<UBFNode_SpawnRuneProjectileProfile>(Pair.Value);
		}
		if (!BuffFlowProjectileNode)
		{
			BuffFlowProjectileNode = Cast<UBFNode_SpawnBuffFlowProjectile>(Pair.Value);
		}
	}

	TestTrue(TEXT("Moonlight base flow has projectile node"), SlashNode || ProjectileProfileNode || BuffFlowProjectileNode);
	if (!SlashNode && !ProjectileProfileNode && !BuffFlowProjectileNode)
	{
		return false;
	}

	if (SlashNode)
	{
		TestEqual(TEXT("Moonlight base projectile count"), SlashNode->ProjectileCount, 1);
		TestEqual(TEXT("Moonlight base keeps one path"), SlashNode->ProjectileConeAngleDegrees, 0.f);
	}
	else if (ProjectileProfileNode && ProjectileProfileNode->Profile)
	{
		TestEqual(TEXT("Moonlight base profile projectile count"), ProjectileProfileNode->Profile->Projectile.ProjectileCount, 1);
		TestEqual(TEXT("Moonlight base profile keeps one path"), ProjectileProfileNode->Profile->Projectile.ProjectileConeAngleDegrees, 0.f);
	}
	else if (BuffFlowProjectileNode)
	{
		TestEqual(TEXT("Moonlight base buff-flow projectile count"), BuffFlowProjectileNode->ProjectileCount.Value, 1);
	}

	auto ValidateForwardMoonlightComboProjectiles = [this](const TCHAR* FlowPath, const TCHAR* Label, const bool bShouldUseComboProjectiles) -> bool
	{
		(void)bShouldUseComboProjectiles;
		UFlowAsset* Flow = LoadObject<UFlowAsset>(nullptr, FlowPath);
		TestNotNull(FString::Printf(TEXT("%s flow exists"), Label), Flow);
		if (!Flow)
		{
			return false;
		}

		UBFNode_SpawnSlashWaveProjectile* FlowSlashNode = nullptr;
		UBFNode_SpawnRuneProjectileProfile* FlowProfileNode = nullptr;
		UBFNode_SpawnBuffFlowProjectile* FlowBuffProjectileNode = nullptr;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (!FlowSlashNode)
			{
				FlowSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			}
			if (!FlowProfileNode)
			{
				FlowProfileNode = Cast<UBFNode_SpawnRuneProjectileProfile>(Pair.Value);
			}
			if (!FlowBuffProjectileNode)
			{
				FlowBuffProjectileNode = Cast<UBFNode_SpawnBuffFlowProjectile>(Pair.Value);
			}
		}

		TestTrue(FString::Printf(TEXT("%s has projectile node"), Label), FlowSlashNode || FlowProfileNode || FlowBuffProjectileNode);
		if (!FlowSlashNode && !FlowProfileNode && !FlowBuffProjectileNode)
		{
			return false;
		}

		const FString ProjectileCountMessage = FString::Printf(TEXT("%s has at least one authored projectile"), Label);
		if (FlowSlashNode)
		{
			TestTrue(*ProjectileCountMessage, FlowSlashNode->ProjectileCount >= 1);
		}
		else if (FlowProfileNode && FlowProfileNode->Profile)
		{
			TestTrue(*ProjectileCountMessage, FlowProfileNode->Profile->Projectile.ProjectileCount >= 1);
		}
		else if (FlowBuffProjectileNode)
		{
			TestTrue(*ProjectileCountMessage, FlowBuffProjectileNode->ProjectileCount.Value >= 1);
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

	const FGameplayTag SplashEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Splash"), false);
	const FGameplayTag SplitEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Split"), false);
	const FGameplayTag SplashSplitIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.SplashSplit"), false);
	const FGameplayTag SplashSplitEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.SplashSplit"), false);
	TestTrue(TEXT("Buff.Splash tag exists"), SplashEffectTag.IsValid());
	TestTrue(TEXT("Buff.Split tag exists"), SplitEffectTag.IsValid());
	TestTrue(TEXT("Buff.SplashSplit tag exists"), SplashSplitIdTag.IsValid());
	TestTrue(TEXT("Buff.SplashSplit tag exists"), SplashSplitEffectTag.IsValid());

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
		{ TEXT("Burn"), TEXT("Buff.Fire"), TEXT("Buff.Fire") },
		{ TEXT("Poison"), TEXT("Buff.Poison"), TEXT("Buff.Poison") },
		{ TEXT("Bleed"), TEXT("Buff.Bleed"), TEXT("Buff.Bleed") },
		{ TEXT("Rend"), TEXT("Buff.Rend"), TEXT("Buff.Rend") },
		{ TEXT("Wound"), TEXT("Buff.Wound"), TEXT("Buff.Wound") },
		{ TEXT("Knockback"), TEXT("Buff.Knockback"), TEXT("Buff.Knockback") },
		{ TEXT("Fear"), TEXT("Buff.Fear"), TEXT("Buff.Fear") },
		{ TEXT("Freeze"), TEXT("Buff.Freeze"), TEXT("Buff.Freeze") },
		{ TEXT("Stun"), TEXT("Buff.Stun"), TEXT("Buff.Stun") },
		{ TEXT("Curse"), TEXT("Buff.Curse"), TEXT("Buff.Curse") },
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckGeneratedWeaponSkillFinisherCardBonusConfiguredTest,
	"DevKit.CombatDeck.GeneratedWeaponSkillFinisherCardBonusConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckGeneratedWeaponSkillFinisherCardBonusConfiguredTest::RunTest(const FString& Parameters)
{
	bool bAllValid = true;

	URuneDataAsset* HeavyDA = LoadObject<URuneDataAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_WeaponSkillFinisher.DA_Rune512_WeaponSkillFinisher"));
	if (!HeavyDA)
	{
		HeavyDA = LoadObject<URuneDataAsset>(
			nullptr,
			TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Heavy.DA_Rune512_Heavy"));
	}
	bAllValid &= TestNotNull(TEXT("Generated WeaponSkill finisher DA exists or legacy Heavy DA is still loadable"), HeavyDA);
	if (!HeavyDA)
	{
		return false;
	}

	const FGameplayTag HeavyIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.WeaponSkillFinisher"), false);
	const FGameplayTag HeavyEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Detonate"), false);
	const FGameplayTag WeaponSkillFinisherIdTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.WeaponSkillFinisher"), false);
	const FGameplayTag DetonateEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Detonate"), false);
	const FGameplayTag KnockbackEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Knockback"), false);
	const FGameplayTag KnockbackActionTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
	const FCombatCardConfig& HeavyCard = HeavyDA->RuneInfo.CombatCard;
	bAllValid &= TestTrue(TEXT("Buff.WeaponSkillFinisher tag exists"), HeavyIdTag.IsValid());
	bAllValid &= TestTrue(TEXT("Buff.Detonate tag exists"), HeavyEffectTag.IsValid());
	bAllValid &= TestTrue(TEXT("Buff.WeaponSkillFinisher tag exists"), WeaponSkillFinisherIdTag.IsValid());
	bAllValid &= TestTrue(TEXT("Buff.Detonate tag exists"), DetonateEffectTag.IsValid());
	bAllValid &= TestTrue(TEXT("Buff.Knockback tag exists"), KnockbackEffectTag.IsValid());
	bAllValid &= TestTrue(TEXT("Action.Knockback tag exists"), KnockbackActionTag.IsValid());
	bAllValid &= TestEqual(TEXT("Heavy card is a normal card"), HeavyCard.CardType, ECombatCardType::Normal);
	bAllValid &= TestEqual(TEXT("Heavy card is rare"), HeavyDA->RuneInfo.RuneConfig.Rarity, ERuneRarity::Rare);
	bAllValid &= TestEqual(TEXT("Heavy card keeps Any legacy action"), HeavyCard.RequiredAction, ECardRequiredAction::Any);
	bAllValid &= TestEqual(TEXT("Heavy card is routed to the weapon skill slot"), HeavyCard.RequiredActionSlot, ECombatDeckActionSlot::WeaponSkill);
	bAllValid &= TestEqual(TEXT("Heavy card resolves as a finisher role"), HeavyCard.RequiredFlowRole, ECombatDeckFlowRole::Finisher);
	bAllValid &= TestEqual(TEXT("Heavy card triggers on hit"), HeavyCard.TriggerTiming, ECombatCardTriggerTiming::OnHit);
	bAllValid &= TestTrue(TEXT("WeaponSkill finisher card id configured or legacy Heavy id retained"),
		HeavyCard.CardIdTag == WeaponSkillFinisherIdTag || HeavyCard.CardIdTag == HeavyIdTag);
	bAllValid &= TestTrue(TEXT("WeaponSkill finisher card has Detonate effect or legacy Heavy effect"),
		HeavyCard.CardEffectTags.HasTagExact(DetonateEffectTag) || HeavyCard.CardEffectTags.HasTagExact(HeavyEffectTag));
	bAllValid &= TestTrue(TEXT("Heavy card has Knockback effect tag"), HeavyCard.CardEffectTags.HasTagExact(KnockbackEffectTag));
	const FString HeavyDescription = HeavyDA->RuneInfo.RuneConfig.RuneDescription.ToString();
	bAllValid &= TestFalse(TEXT("Heavy description is configured"), HeavyDescription.IsEmpty());
	bAllValid &= TestTrue(TEXT("Heavy description explains weapon skill finisher use"),
		HeavyDescription.Contains(TEXT("WeaponSkill")) || HeavyDescription.Contains(TEXT("鎴樻")) || HeavyDescription.Contains(TEXT("finisher")));
	bAllValid &= TestTrue(TEXT("Heavy card data declares bonus damage and knockback intent"),
		(HeavyCard.CardEffectTags.HasTagExact(DetonateEffectTag) || HeavyCard.CardEffectTags.HasTagExact(HeavyEffectTag))
		&& HeavyCard.CardEffectTags.HasTagExact(KnockbackEffectTag));

	UFlowAsset* HeavyFlow = LoadObject<UFlowAsset>(
		nullptr,
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_WeaponSkillFinisher_Base.FA_Rune512_WeaponSkillFinisher_Base"));
	if (!HeavyFlow)
	{
		HeavyFlow = LoadObject<UFlowAsset>(
			nullptr,
			TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_Heavy_Base.FA_Rune512_Heavy_Base"));
	}
	bAllValid &= TestNotNull(TEXT("Generated WeaponSkill finisher base flow exists or legacy Heavy base flow is still loadable"), HeavyFlow);
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
			if (Branch->RequiredActionSlot == ECombatDeckActionSlot::WeaponSkill
				&& Branch->RequiredFlowRole == ECombatDeckFlowRole::Finisher)
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

	bAllValid &= TestNotNull(TEXT("Heavy flow has base extra damage for weapon skill finishers"), BaseDamageNode);
	bAllValid &= TestNotNull(TEXT("Heavy flow has a weapon-skill finisher context branch"), HeavyBranch);
	if (HeavyBranch)
	{
		bAllValid &= TestEqual(TEXT("Heavy branch ignores deprecated light/heavy action"), HeavyBranch->RequiredAction, ECardRequiredAction::Any);
		bAllValid &= TestTrue(TEXT("WeaponSkill finisher branch matches formal or legacy card id"),
			HeavyBranch->RequiredSourceCardIdTags.HasTagExact(WeaponSkillFinisherIdTag)
			|| HeavyBranch->RequiredSourceCardIdTags.HasTagExact(HeavyIdTag));
	}
	bAllValid &= TestNotNull(TEXT("Heavy flow has a large extra damage node for coordinated finishers"), ExtraDamageNode);
	bAllValid &= TestNotNull(TEXT("Heavy flow has a large bonus knockback node for coordinated finishers"), BonusKnockbackNode);

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
		int32 ProfileProjectileNodeCount = 0;
		int32 BuffFlowProjectileNodeCount = 0;
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
			else if (const UBFNode_SpawnRuneProjectileProfile* ProfileNode = Cast<UBFNode_SpawnRuneProjectileProfile>(Pair.Value))
			{
				++ProfileProjectileNodeCount;
				TestNotNull(FString::Printf(TEXT("%s projectile profile is assigned"), Label), ProfileNode->Profile.Get());
				if (ProfileNode->Profile)
				{
					TestTrue(FString::Printf(TEXT("%s profile has authored projectile"), Label), ProfileNode->Profile->Projectile.ProjectileCount >= 1);
					TestNull(FString::Printf(TEXT("%s profile projectile visual Niagara is cleared"), Label), ProfileNode->Profile->Projectile.ProjectileVisualNiagaraSystem.Get());
					TestFalse(FString::Printf(TEXT("%s profile keeps default projectile visuals visible"), Label), ProfileNode->Profile->Projectile.bHideDefaultProjectileVisuals);
					TestNull(FString::Printf(TEXT("%s profile hit Niagara is cleared"), Label), ProfileNode->Profile->Projectile.HitNiagaraSystem.Get());
					TestNull(FString::Printf(TEXT("%s profile expire Niagara is cleared"), Label), ProfileNode->Profile->Projectile.ExpireNiagaraSystem.Get());
				}
			}
			else if (const UBFNode_SpawnBuffFlowProjectile* BuffProjectileNode = Cast<UBFNode_SpawnBuffFlowProjectile>(Pair.Value))
			{
				++BuffFlowProjectileNodeCount;
				TestTrue(FString::Printf(TEXT("%s buff-flow projectile has authored projectile"), Label), BuffProjectileNode->ProjectileCount.Value >= 1);
			}
		}

		TestTrue(FString::Printf(TEXT("%s has projectile node"), Label),
			(SlashNodeCount + ProfileProjectileNodeCount + BuffFlowProjectileNodeCount) > 0);
		return (SlashNodeCount + ProfileProjectileNodeCount + BuffFlowProjectileNodeCount) > 0;
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
	const FGameplayTag BurnHitTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Moonlight.BurnHit"), false);
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
	const FGameplayTag PoisonHitTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Moonlight.PoisonHit"), false);
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
		UBFNode_SpawnBuffFlowProjectile* BuffProjectileNode = nullptr;
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
			if (!BuffProjectileNode)
			{
				BuffProjectileNode = Cast<UBFNode_SpawnBuffFlowProjectile>(Pair.Value);
			}
			if (UBFNode_WaitGameplayEvent* CandidateWaitNode = Cast<UBFNode_WaitGameplayEvent>(Pair.Value))
			{
				if (!WaitNode || CandidateWaitNode->EventTag == PoisonHitTag)
				{
					WaitNode = CandidateWaitNode;
				}
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
			if (UBFNode_ApplyEffect* ApplyNode = Cast<UBFNode_ApplyEffect>(Pair.Value))
			{
				if (!PrimaryPoisonNode || (ApplyNode->Effect && ApplyNode->Effect->GetName().Contains(TEXT("GE_Poison"))))
				{
					PrimaryPoisonNode = ApplyNode;
				}
			}
			if (!RadiusPoisonNode)
			{
				RadiusPoisonNode = Cast<UBFNode_ApplyGEInRadius>(Pair.Value);
			}
		}

		TestTrue(FString::Printf(TEXT("%s has projectile event source"), Label), SlashNode || BuffProjectileNode);
		TestNotNull(FString::Printf(TEXT("%s has wait gameplay event node"), Label), WaitNode);
		TestNotNull(FString::Printf(TEXT("%s has poison hit Niagara node"), Label), PoisonHitVfxNode);
		TestNotNull(FString::Printf(TEXT("%s has poison spread Niagara node"), Label), PoisonSpreadVfxNode);
		TestEqual(FString::Printf(TEXT("%s has no active flipbook nodes"), Label), ActiveFlipbookCount, 0);
		TestNotNull(FString::Printf(TEXT("%s has primary poison node"), Label), PrimaryPoisonNode);
		TestNotNull(FString::Printf(TEXT("%s has radius poison node"), Label), RadiusPoisonNode);
		if ((!SlashNode && !BuffProjectileNode) || !WaitNode || !PoisonHitVfxNode || !PoisonSpreadVfxNode || !PrimaryPoisonNode || !RadiusPoisonNode)
		{
			return false;
		}

		if (SlashNode)
		{
			TestEqual(FString::Printf(TEXT("%s slash-wave sends poison hit event"), Label), SlashNode->HitGameplayEventTag, PoisonHitTag);
			TestNull(FString::Printf(TEXT("%s slash-wave does not use inline hit Niagara"), Label), SlashNode->HitNiagaraSystem);
			TestNull(FString::Printf(TEXT("%s slash-wave does not use inline expire Niagara"), Label), SlashNode->ExpireNiagaraSystem);
			TestNull(FString::Printf(TEXT("%s slash-wave does not use inline additional hit GE"), Label), SlashNode->AdditionalHitEffect);
		}
		if (BuffProjectileNode)
		{
			TestEqual(FString::Printf(TEXT("%s buff-flow projectile sends poison hit event"), Label), BuffProjectileNode->TriggerGameplayEventTag.Value, PoisonHitTag);
			TestTrue(FString::Printf(TEXT("%s buff-flow projectile sends event to creator"), Label), BuffProjectileNode->bSendTriggerEventToCreator);
		}

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
	const FGameplayTag PoisonedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Poison"), false);
	const FGameplayTag PoisonPercentTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Poison.PercentPerStack"), false);
	const FGameplayTag PoisonArmorPercentTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Poison.ArmorPercentPerStack"), false);
	TestTrue(TEXT("Buff.Poison tag exists"), PoisonedTag.IsValid());
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
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Fire"), false);
	const FGameplayTag BurnDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Burn"), false);
	TestTrue(TEXT("Buff.Fire tag exists"), BurningTag.IsValid());
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
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Fire"), false);
	TestTrue(TEXT("Buff.Fire tag exists"), BurningTag.IsValid());

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
