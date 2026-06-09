#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Abilities/GA_RangeAttack.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"

namespace
{
	EYogComboGraphInputAction CardActionToComboGraphInput(ECardRequiredAction InputAction)
	{
		switch (InputAction)
		{
		case ECardRequiredAction::Light:
			return EYogComboGraphInputAction::Light;
		case ECardRequiredAction::Heavy:
			return EYogComboGraphInputAction::Heavy;
		case ECardRequiredAction::Any:
		default:
			return EYogComboGraphInputAction::Any;
		}
	}

	void AddLegacyComboProgressTags(FGameplayTagContainer& OutTags)
	{
		static const FName KnownComboTagNames[] = {
			TEXT("PlayerState.AbilityCast.LightAtk.Combo1"),
			TEXT("PlayerState.AbilityCast.LightAtk.Combo2"),
			TEXT("PlayerState.AbilityCast.LightAtk.Combo3"),
			TEXT("PlayerState.AbilityCast.LightAtk.Combo4"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"),
		};

		for (const FName& TagName : KnownComboTagNames)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
			if (Tag.IsValid())
			{
				OutTags.AddTag(Tag);
			}
		}
	}

	void ClearComboWindowAndProgressLooseTags(UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return;
		}

		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
		if (CanComboTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}

		FGameplayTagContainer ProgressTags;
		AddLegacyComboProgressTags(ProgressTags);
		for (const FGameplayTag& Tag : ProgressTags)
		{
			if (ASC->GetTagCount(Tag) > 0)
			{
				ASC->SetLooseGameplayTagCount(Tag, 0);
			}
		}
	}

	bool IsSpecialAttackBlockingCombo(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return false;
		}

		const FGameplayTag SpecialAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack"), false);
		if (!SpecialAttackTag.IsValid() || !ASC->HasMatchingGameplayTag(SpecialAttackTag))
		{
			return false;
		}

		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
		return !CanComboTag.IsValid() || ASC->GetTagCount(CanComboTag) <= 0;
	}
}

UComboRuntimeComponent::UComboRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboRuntimeComponent::SetComboSpecialActionAbility(TSubclassOf<UYogGameplayAbility> InAbility)
{
	ComboSpecialActionAbility = InAbility;
}

void UComboRuntimeComponent::LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	LoadWeaponComboGraph(InComboGraph);
}

void UComboRuntimeComponent::LoadWeaponComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	WeaponComboGraph = InComboGraph;
	SetActiveComboGraph(WeaponComboGraph);
}

void UComboRuntimeComponent::LoadSpecialAttackComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	const bool bWasActiveSpecialGraph = GetComboGraph() == SpecialAttackComboGraph;
	SpecialAttackComboGraph = InComboGraph;

	if (bWasActiveSpecialGraph)
	{
		SetActiveComboGraph(SpecialAttackComboGraph ? SpecialAttackComboGraph : WeaponComboGraph);
	}
	else if (!WeaponComboGraph && SpecialAttackComboGraph)
	{
		SetActiveComboGraph(SpecialAttackComboGraph);
	}
}

void UComboRuntimeComponent::SetActiveComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	if (GetComboGraph() != InComboGraph)
	{
		Super::LoadComboGraph(InComboGraph);
	}
}

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	return TryActivateComboFromGraph(
		WeaponComboGraph,
		CardActionToComboGraphInput(InputAction),
		InputAction,
		ECombatDeckActionSlot::Attack,
		ECombatDeckFlowRole::Starter,
		PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateWeaponSkill(APlayerCharacterBase* PlayerOwner)
{
	return TryActivateComboFromGraph(
		WeaponComboGraph,
		EYogComboGraphInputAction::WeaponSkill,
		ECardRequiredAction::Any,
		ECombatDeckActionSlot::Dash,
		ECombatDeckFlowRole::Catalyst,
		PlayerOwner,
		ComboSpecialActionAbility);
}

bool UComboRuntimeComponent::TryActivateSpecialAttackCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner)
{
	return TryActivateComboFromGraph(
		SpecialAttackComboGraph,
		EYogComboGraphInputAction::Heavy,
		ECardRequiredAction::Heavy,
		ECombatDeckActionSlot::WeaponSkill,
		ECombatDeckFlowRole::Finisher,
		PlayerOwner,
		AbilityClass);
}

bool UComboRuntimeComponent::TryActivateComboFromGraph(
	UGameplayAbilityComboGraph* SourceGraph,
	EYogComboGraphInputAction GraphInput,
	ECardRequiredAction RuntimeInputAction,
	ECombatDeckActionSlot ActionSlot,
	ECombatDeckFlowRole FlowRole,
	APlayerCharacterBase* PlayerOwner,
	TSubclassOf<UGameplayAbility> AbilityOverride)
{
	if (!SourceGraph || !PlayerOwner)
	{
		return false;
	}

	SetActiveComboGraph(SourceGraph);

	UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	if (IsSpecialAttackBlockingCombo(ASC))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[ComboRuntime] Block combo activation during special attack until CanCombo opens input=%s"),
			*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)));
		return false;
	}

	if (!GetComboStartNodeId().IsNone() && !IsActiveComboAbilityRunning(ASC))
	{
		ClearStaleActiveComboState(ASC, TEXT("AttackInput"));
	}

	const FName StartNodeId = GetComboStartNodeId();
	const bool bHasActiveAttackNode = !StartNodeId.IsNone()
		&& (GetActiveAttackGuid().IsValid() || ActiveAbilitySpecHandle.IsValid());

	FWeaponComboNodeConfig NextNodeConfig;
	const FWeaponComboNodeConfig* NextNode = nullptr;
	bool bFoundChildNode = false;
	bool bBlockedRootFallbackDuringActiveNode = false;

	if (GetComboGraph())
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);

		FYogComboGraphNodeSelection Selection;
		if (FindNextComboGraphNode(GraphInput, &OwnedTags, Selection))
		{
			if (bHasActiveAttackNode && !Selection.bFoundChildNode)
			{
				bBlockedRootFallbackDuringActiveNode = true;
			}
			else
			{
				NextNodeConfig = FWeaponComboNodeConfig::FromComboGraphNode(Selection.Node, RuntimeInputAction);
				NextNodeConfig.CombatDeckActionSlot = ActionSlot;
				NextNodeConfig.CombatDeckFlowRole = NextNodeConfig.bIsComboFinisher ? ECombatDeckFlowRole::Finisher : FlowRole;
				NextNodeConfig.bIsComboFinisher = NextNodeConfig.bIsComboFinisher || NextNodeConfig.CombatDeckFlowRole == ECombatDeckFlowRole::Finisher;
				NextNode = &NextNodeConfig;
				bFoundChildNode = Selection.bFoundChildNode;
				PrepareComboGraphNodeActivation(Selection);
			}
		}
	}

	if (bBlockedRootFallbackDuringActiveNode)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[ComboRuntime] Ignore input without child while active node is playing input=%s current=%s"),
			*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)),
			*StartNodeId.ToString());
		return false;
	}

	if (!NextNode || (!NextNode->Montage && !NextNode->MontageConfig))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] No combo node for input=%s current=%s graph=%s"),
			*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)),
			*GetCurrentNodeId().ToString(),
			*GetNameSafe(GetComboGraph()));
		if (!GetCurrentNodeId().IsNone() && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
		ActiveNode = FWeaponComboNodeConfig();
		MarkComboActivationMiss();
		return false;
	}

	if (bFoundChildNode)
	{
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
		if (CanComboTag.IsValid() && ASC->GetTagCount(CanComboTag) <= 0)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[ComboRuntime] Queue child activation until CanCombo opens input=%s current=%s next=%s"),
				*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)),
				*GetCurrentNodeId().ToString(),
				*NextNode->NodeId.ToString());
			ClearPreparedComboActivation();
			return false;
		}
	}

	ActiveNode = *NextNode;

	if (DidExitComboState())
	{
		FGameplayTagContainer TagsToCancel;
		AddLegacyComboProgressTags(TagsToCancel);
		ASC->CancelAbilities(&TagsToCancel);
		ClearComboWindowAndProgressLooseTags(ASC);
	}

	if (bFoundChildNode && ActiveAbilitySpecHandle.IsValid())
	{
		ASC->CancelAbilityHandle(ActiveAbilitySpecHandle);
	}

	// TEMP: route to GA_MeleeAttack / GA_RangeAttack based on the ComboGraph
	// node's AttackType. Previously hard-coded to UGA_PlayMontage, which doesn't
	// run the YogTargetType hit-collection pipeline (that lives on UGA_MeleeAttack).
	// GA_RangeAttack is a stub for now; flip the enum in the editor to test routing.
	TSubclassOf<UGameplayAbility> AbilityClass = AbilityOverride
		? TSubclassOf<UGameplayAbility>(AbilityOverride.Get())
		: (NextNode->AttackType == EYogComboGraphAttackType::Range)
			? TSubclassOf<UGameplayAbility>(UGA_RangeAttack::StaticClass())
			: TSubclassOf<UGameplayAbility>(UGA_MeleeAttack::StaticClass());

	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass, true);
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate node=%s input=%s current=%s montage=%s montageConfig=%s attackType=%s ability=%s"),
			*NextNode->NodeId.ToString(),
			*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)),
			*GetCurrentNodeId().ToString(),
			*GetNameSafe(NextNode->Montage.Get()),
			*GetNameSafe(NextNode->MontageConfig.Get()),
			*StaticEnum<EYogComboGraphAttackType>()->GetNameStringByValue(static_cast<int64>(NextNode->AttackType)),
			*GetNameSafe(AbilityClass.Get()));
		ActiveNode = FWeaponComboNodeConfig();
		ClearPreparedComboActivation();
		return false;
	}

	CommitPreparedComboActivation();
	// Do NOT clear ActiveAbilitySpecHandle here — RegisterActiveAttackAbility already set it
	// to the new activation's handle during TryActivateAbilityByClass. Clearing it would
	// prevent CancelAbilityHandle from cancelling the current ability on the next combo hit.

	static const FGameplayTag ChainActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Combo.ChainActive"), false);
	if (ChainActiveTag.IsValid() && ASC->GetTagCount(ChainActiveTag) <= 0)
	{
		ASC->AddLooseGameplayTag(ChainActiveTag);
		TrackRuntimeCombatLooseTag(ChainActiveTag);
	}

	return true;
}

void UComboRuntimeComponent::ResetCombo()
{
	Super::ResetCombo();
	ActiveNode = FWeaponComboNodeConfig();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
	ClearRuntimeCombatLooseTags();

	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner()))
	{
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
	}
}

void UComboRuntimeComponent::ClearRuntimeCombatLooseTags()
{
	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = PlayerOwner ? PlayerOwner->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		RuntimeCombatLooseTags.Reset();
		return;
	}

	for (const FGameplayTag& Tag : RuntimeCombatLooseTags)
	{
		if (Tag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(Tag, 0);
		}
	}
	ClearComboWindowAndProgressLooseTags(ASC);
	RuntimeCombatLooseTags.Reset();
}

const FWeaponComboNodeConfig* UComboRuntimeComponent::GetActiveNode() const
{
	return GetActiveGraphNodeId().IsNone() ? nullptr : &ActiveNode;
}

bool UComboRuntimeComponent::IsActiveComboAbilityRunning(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return false;
	}

	if (ActiveAbilitySpecHandle.IsValid())
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(ActiveAbilitySpecHandle))
		{
			if (Spec->IsActive())
			{
				return true;
			}
		}
	}

	return false;
}

void UComboRuntimeComponent::ClearStaleActiveComboState(UAbilitySystemComponent* ASC, const TCHAR* Reason)
{
	UE_LOG(LogTemp, Warning,
		TEXT("[ComboRuntime] Clear stale active combo state Reason=%s Current=%s Active=%s GuidValid=%d HandleValid=%d"),
		Reason ? Reason : TEXT("Unknown"),
		*GetCurrentNodeId().ToString(),
		*GetActiveGraphNodeId().ToString(),
		GetActiveAttackGuid().IsValid() ? 1 : 0,
		ActiveAbilitySpecHandle.IsValid() ? 1 : 0);

	CurrentNodeId = NAME_None;
	ActiveNode = FWeaponComboNodeConfig();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
	ClearPreparedComboActivation();

	if (ASC)
	{
		ClearComboWindowAndProgressLooseTags(ASC);
	}
}

void UComboRuntimeComponent::RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle)
{
	if (AttackGuid.IsValid() && GetActiveAttackGuid().IsValid() && GetActiveAttackGuid() == AttackGuid)
	{
		ActiveAbilitySpecHandle = AbilityHandle;
	}
}

bool UComboRuntimeComponent::HandleAttackAbilityEnded(const FGuid& EndedAttackGuid)
{
	if (!EndedAttackGuid.IsValid() || !GetActiveAttackGuid().IsValid() || GetActiveAttackGuid() != EndedAttackGuid)
	{
		return false;
	}

	const bool bHadActiveNode = GetActiveNode() != nullptr || !GetCurrentNodeId().IsNone();
	ResetCombo();
	return bHadActiveNode;
}

void UComboRuntimeComponent::TrackRuntimeCombatLooseTag(const FGameplayTag& Tag)
{
	if (Tag.IsValid())
	{
		RuntimeCombatLooseTags.AddTag(Tag);
	}
}

FCombatDeckActionContext UComboRuntimeComponent::BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const
{
	FCombatDeckActionContext Context;
	if (!GetActiveNode())
	{
		return Context;
	}

	Context.ActionType = ActiveNode.InputAction;
	Context.ActionSlot = ActiveNode.CombatDeckActionSlot;
	Context.FlowRole = ActiveNode.CombatDeckFlowRole;
	Context.ComboIndex = GetComboIndex();
	Context.ComboNodeId = ActiveNode.NodeId;
	Context.ComboTags = GetComboTags();
	Context.WeaponDef = PlayerOwner ? PlayerOwner->EquippedWeaponDef : nullptr;
	Context.bIsComboFinisher = ActiveNode.bIsComboFinisher;
	Context.ReleaseMode = ActiveNode.bIsComboFinisher ? ECombatCardReleaseMode::Finisher : ECombatCardReleaseMode::Normal;
	Context.bComboContinued = DidComboContinue();
	Context.bExitedComboState = DidExitComboState();
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = GetActiveAttackGuid();
	return Context;
}
