#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Abilities/GA_PlayerDash.h"
#include "AbilitySystem/Abilities/GA_RangeAttack.h"
#include "AbilitySystem/Abilities/GA_Special.h"
#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
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
			return EYogComboGraphInputAction::Attack;
		case ECardRequiredAction::Heavy:
			return EYogComboGraphInputAction::WeaponSkill;
		case ECardRequiredAction::Any:
		default:
			return EYogComboGraphInputAction::Any;
		}
	}

	TSubclassOf<UGameplayAbility> GetDefaultAbilityForInput(
		EYogComboGraphInputAction GraphInput,
		EYogComboGraphAttackType AttackType,
		TSubclassOf<UYogGameplayAbility> WeaponSkillAbility)
	{
		switch (GraphInput)
		{
		case EYogComboGraphInputAction::Attack:
			return AttackType == EYogComboGraphAttackType::Range
				? TSubclassOf<UGameplayAbility>(UGA_RangeAttack::StaticClass())
				: TSubclassOf<UGameplayAbility>(UGA_MeleeAttack::StaticClass());
		case EYogComboGraphInputAction::Dash:
			return TSubclassOf<UGameplayAbility>(UGA_PlayerDash::StaticClass());
		case EYogComboGraphInputAction::Special:
			return TSubclassOf<UGameplayAbility>(UGA_Special::StaticClass());
		case EYogComboGraphInputAction::WeaponSkill:
		case EYogComboGraphInputAction::LegacyWeaponSkill:
			return WeaponSkillAbility
				? TSubclassOf<UGameplayAbility>(WeaponSkillAbility.Get())
				: TSubclassOf<UGameplayAbility>(UGA_WeaponSkill::StaticClass());
		default:
			return TSubclassOf<UGameplayAbility>(UGA_MeleeAttack::StaticClass());
		}
	}

	FGameplayTag GetActivationTagForInput(EYogComboGraphInputAction GraphInput)
	{
		switch (GraphInput)
		{
		case EYogComboGraphInputAction::Attack:
			return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"), false);
		case EYogComboGraphInputAction::Dash:
			return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash"), false);
		case EYogComboGraphInputAction::Special:
			return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special"), false);
		case EYogComboGraphInputAction::WeaponSkill:
		case EYogComboGraphInputAction::LegacyWeaponSkill:
			return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"), false);
		default:
			return FGameplayTag();
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

		const FGameplayTag SpecialTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special"), false);
		const FGameplayTag LegacySpecialAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack"), false);
		const bool bHasSpecialTag =
			(SpecialTag.IsValid() && ASC->HasMatchingGameplayTag(SpecialTag)) ||
			(LegacySpecialAttackTag.IsValid() && ASC->HasMatchingGameplayTag(LegacySpecialAttackTag));
		if (!bHasSpecialTag)
		{
			return false;
		}

		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
		return !CanComboTag.IsValid() || ASC->GetTagCount(CanComboTag) <= 0;
	}

	// Returns true if a granted ability of AbilityClass would pass its own
	// activation conditions right now (charge, cooldown, blocked tags, etc.)
	// without actually activating it. Mirrors GAS by preferring the live instance
	// over the CDO when one exists.
	bool CanActivateGrantedAbility(UAbilitySystemComponent* ASC, const UClass* AbilityClass)
	{
		if (!ASC || !AbilityClass)
		{
			return false;
		}

		const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.Ability || !Spec.Ability->GetClass()->IsChildOf(AbilityClass))
			{
				continue;
			}

			const UGameplayAbility* AbilityToCheck =
				Spec.GetPrimaryInstance() ? Spec.GetPrimaryInstance() : Spec.Ability.Get();
			if (AbilityToCheck && AbilityToCheck->CanActivateAbility(Spec.Handle, ActorInfo))
			{
				return true;
			}
		}

		return false;
	}

	void AddUniqueAbilityClass(
		TArray<TSubclassOf<UGameplayAbility>>& OutAbilityClasses,
		TSubclassOf<UGameplayAbility> AbilityClass)
	{
		if (AbilityClass)
		{
			OutAbilityClasses.AddUnique(AbilityClass);
		}
	}

	void AddDefaultAbilityForInput(
		TArray<TSubclassOf<UGameplayAbility>>& OutAbilityClasses,
		EYogComboGraphInputAction GraphInput,
		EYogComboGraphAttackType AttackType,
		TSubclassOf<UYogGameplayAbility> WeaponSkillAbility)
	{
		if (GraphInput == EYogComboGraphInputAction::Any)
		{
			AddDefaultAbilityForInput(
				OutAbilityClasses,
				EYogComboGraphInputAction::Attack,
				AttackType,
				WeaponSkillAbility);
			AddDefaultAbilityForInput(
				OutAbilityClasses,
				EYogComboGraphInputAction::WeaponSkill,
				AttackType,
				WeaponSkillAbility);
			AddDefaultAbilityForInput(
				OutAbilityClasses,
				EYogComboGraphInputAction::Dash,
				AttackType,
				WeaponSkillAbility);
			AddDefaultAbilityForInput(
				OutAbilityClasses,
				EYogComboGraphInputAction::Special,
				AttackType,
				WeaponSkillAbility);
			return;
		}

		AddUniqueAbilityClass(
			OutAbilityClasses,
			GetDefaultAbilityForInput(GraphInput, AttackType, WeaponSkillAbility));
	}

	void GatherWeaponComboAbilityClasses(
		const UGameplayAbilityComboGraph* SourceGraph,
		TSubclassOf<UYogGameplayAbility> WeaponSkillAbility,
		TArray<TSubclassOf<UGameplayAbility>>& OutAbilityClasses)
	{
		if (!SourceGraph)
		{
			return;
		}

		for (const UGenericGraphNode* GenericNode : SourceGraph->AllNodes)
		{
			const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(GenericNode);
			if (!ComboNode)
			{
				continue;
			}

			AddUniqueAbilityClass(OutAbilityClasses, ComboNode->GameplayAbilityClass);

			const bool bIsRootNode = SourceGraph->RootNodes.Contains(GenericNode) || ComboNode->ParentNodes.IsEmpty();
			if (bIsRootNode)
			{
				AddDefaultAbilityForInput(
					OutAbilityClasses,
					ComboNode->RootInputAction,
					ComboNode->AttackType,
					WeaponSkillAbility);
			}

			for (const TPair<UGenericGraphNode*, UGenericGraphEdge*>& EdgePair : ComboNode->Edges)
			{
				const UGameplayAbilityComboGraphNode* ChildNode = Cast<UGameplayAbilityComboGraphNode>(EdgePair.Key);
				const UGameplayAbilityComboGraphEdge* Edge = Cast<UGameplayAbilityComboGraphEdge>(EdgePair.Value);
				if (!ChildNode || !Edge)
				{
					continue;
				}

				AddDefaultAbilityForInput(
					OutAbilityClasses,
					Edge->InputAction,
					ChildNode->AttackType,
					WeaponSkillAbility);
			}
		}
	}
}

UComboRuntimeComponent::UComboRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboRuntimeComponent::SetWeaponSkillAbility(TSubclassOf<UYogGameplayAbility> InAbility)
{
	WeaponSkillAbility = InAbility;
}

void UComboRuntimeComponent::EnsureWeaponComboAbilitiesGranted(APlayerCharacterBase* PlayerOwner)
{
	UAbilitySystemComponent* ASC = PlayerOwner ? PlayerOwner->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	TArray<TSubclassOf<UGameplayAbility>> AbilityClasses;
	GatherWeaponComboAbilityClasses(WeaponComboGraph, WeaponSkillAbility, AbilityClasses);
	GatherWeaponComboAbilityClasses(WeaponSkillComboGraph, WeaponSkillAbility, AbilityClasses);

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : AbilityClasses)
	{
		EnsureAbilityGranted(ASC, AbilityClass);
	}
}

void UComboRuntimeComponent::LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	LoadWeaponComboGraph(InComboGraph);
}

void UComboRuntimeComponent::LoadWeaponComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	WeaponComboGraph = InComboGraph;

	// Equipping a weapon is a clean slate: drop every preserved per-graph cursor and
	// let the base loader validate + reset the live state with the weapon graph active.
	GraphContexts.Empty();
	bSuppressStaleClearOnce = false;
	Super::LoadComboGraph(WeaponComboGraph);
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

void UComboRuntimeComponent::LoadWeaponSkillComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	const bool bWasActiveSkillGraph = WeaponSkillComboGraph && GetComboGraph() == WeaponSkillComboGraph;

	if (WeaponSkillComboGraph)
	{
		GraphContexts.Remove(WeaponSkillComboGraph);
	}

	WeaponSkillComboGraph = InComboGraph;

	if (WeaponSkillComboGraph)
	{
		GraphContexts.Remove(WeaponSkillComboGraph);
	}

	if (bWasActiveSkillGraph)
	{
		SetActiveComboGraph(WeaponComboGraph ? WeaponComboGraph : WeaponSkillComboGraph);
	}
}

void UComboRuntimeComponent::SetActiveComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	if (GetComboGraph() == InComboGraph)
	{
		return;
	}

	// Switch graphs without resetting: stash the outgoing graph's cursor and restore the
	// incoming graph's, so weapon and weapon-skill combos keep independent positions.
	SaveActiveContext();
	ComboGraph = InComboGraph;
	RestoreContextFor(InComboGraph);
}

void UComboRuntimeComponent::SaveActiveContext()
{
	UGameplayAbilityComboGraph* Active = GetComboGraph();
	if (!Active)
	{
		return;
	}

	FComboGraphContext& Context = GraphContexts.FindOrAdd(Active);
	Context.CurrentNodeId = CurrentNodeId;
	Context.ActiveNodeId = ActiveNodeId;
	Context.ActiveGraphNode = ActiveGraphNode;
	Context.ActiveAttackGuid = ActiveAttackGuid;
	Context.ComboIndex = ComboIndex;
	Context.ComboTags = ComboTags;
	Context.bActiveNodeValid = bActiveNodeValid;
	Context.bComboContinued = bComboContinued;
	Context.bExitedComboState = bExitedComboState;
	Context.ActiveNode = ActiveNode;
	Context.ActiveAbilitySpecHandle = ActiveAbilitySpecHandle;
}

void UComboRuntimeComponent::RestoreContextFor(UGameplayAbilityComboGraph* Graph)
{
	ClearPreparedComboActivation();
	bSuppressStaleClearOnce = false;

	if (const FComboGraphContext* Context = GraphContexts.Find(Graph))
	{
		CurrentNodeId = Context->CurrentNodeId;
		ActiveNodeId = Context->ActiveNodeId;
		ActiveGraphNode = Context->ActiveGraphNode;
		ActiveAttackGuid = Context->ActiveAttackGuid;
		ComboIndex = Context->ComboIndex;
		ComboTags = Context->ComboTags;
		bActiveNodeValid = Context->bActiveNodeValid;
		bComboContinued = Context->bComboContinued;
		bExitedComboState = Context->bExitedComboState;
		ActiveNode = Context->ActiveNode;
		ActiveAbilitySpecHandle = Context->ActiveAbilitySpecHandle;

		// A restored mid-combo cursor has no live ability running, so shield it from the
		// stale-state guard on the very next input (honours "preserve weapon cursor").
		bSuppressStaleClearOnce = !CurrentNodeId.IsNone();
	}
	else
	{
		// No saved cursor for this graph: start fresh from root (matches ResetCombo).
		CurrentNodeId = NAME_None;
		ActiveNodeId = NAME_None;
		ActiveGraphNode = nullptr;
		ActiveAttackGuid.Invalidate();
		ComboIndex = 0;
		ComboTags.Reset();
		bActiveNodeValid = false;
		bComboContinued = false;
		bExitedComboState = true;
		ActiveNode = FWeaponComboNodeConfig();
		ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
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

bool UComboRuntimeComponent::TryActivateAttack(APlayerCharacterBase* PlayerOwner)
{
	return TryActivateCombo(
		ECardRequiredAction::Light,
		PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateWeaponSkill(APlayerCharacterBase* PlayerOwner)
{
	// Weapon skill runs on its own graph so it never disturbs the weapon attack/dash
	// cursor. Fall back to the weapon graph for weapons that ship no dedicated skill graph.
	UGameplayAbilityComboGraph* SkillGraph = WeaponSkillComboGraph ? WeaponSkillComboGraph : WeaponComboGraph;

	const bool bActivated = TryActivateComboFromGraph(
		SkillGraph,
		EYogComboGraphInputAction::WeaponSkill,
		ECardRequiredAction::Any,
		ECombatDeckActionSlot::WeaponSkill,
		ECombatDeckFlowRole::Finisher,
		PlayerOwner,
		WeaponSkillAbility);

	if (!bActivated)
	{
		ResetCombo();
	}

	return bActivated;
}

bool UComboRuntimeComponent::TryActivateDash(APlayerCharacterBase* PlayerOwner)
{
	return TryActivateComboFromGraph(
		WeaponComboGraph,
		EYogComboGraphInputAction::Dash,
		ECardRequiredAction::Any,
		ECombatDeckActionSlot::Dash,
		ECombatDeckFlowRole::Catalyst,
		PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateSpecial(APlayerCharacterBase* PlayerOwner)
{
	return TryActivateComboFromGraph(
		WeaponComboGraph,
		EYogComboGraphInputAction::Special,
		ECardRequiredAction::Heavy,
		ECombatDeckActionSlot::WeaponSkill,
		ECombatDeckFlowRole::Finisher,
		PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateSpecialCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner)
{
	return TryActivateComboFromGraph(
		SpecialAttackComboGraph,
		EYogComboGraphInputAction::Special,
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
		if (bSuppressStaleClearOnce)
		{
			// Cursor was just restored from a saved context; it is intentionally idle.
			bSuppressStaleClearOnce = false;
		}
		else
		{
			ClearStaleActiveComboState(ASC, TEXT("AttackInput"));
		}
	}
	else
	{
		bSuppressStaleClearOnce = false;
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
			if (bHasActiveAttackNode && !Selection.bFoundChildNode && GraphInput != EYogComboGraphInputAction::Dash)
			{
				bBlockedRootFallbackDuringActiveNode = true;
			}
			else
			{
				NextNodeConfig = FWeaponComboNodeConfig::FromComboGraphNode(
					Selection.Node,
					RuntimeInputAction,
					ActionSlot,
					FlowRole);
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

	const bool bDashInterruptInput = GraphInput == EYogComboGraphInputAction::Dash;
	if (bFoundChildNode && !bDashInterruptInput)
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

	const UClass* TargetClass = AbilityOverride
		? AbilityOverride.Get()
		: NextNode->GameplayAbilityClass
			? NextNode->GameplayAbilityClass.Get()
			: GetDefaultAbilityForInput(GraphInput, NextNode->AttackType, WeaponSkillAbility).Get();

	// Dash is an interrupt input: it bypasses the combo window and tears down the
	// in-progress attack (via the DidExitComboState cancel below and the dash GA's
	// own CancelAbilitiesWithTag on activation). Only commit to that interrupt once
	// we know the dash ability can actually activate. Otherwise an unavailable dash
	// (no charge / on cooldown / blocked) would cancel the attack montage with
	// nothing to replace it.
	if (bDashInterruptInput && !CanActivateGrantedAbility(ASC, TargetClass))
	{
		ClearPreparedComboActivation();
		return false;
	}

	ActiveNode = *NextNode;

	if (DidExitComboState())
	{
		FGameplayTagContainer TagsToCancel;
		AddLegacyComboProgressTags(TagsToCancel);
		ASC->CancelAbilities(&TagsToCancel);
		ClearComboWindowAndProgressLooseTags(ASC);
	}

	if (bFoundChildNode && !bDashInterruptInput && ActiveAbilitySpecHandle.IsValid())
	{
		ASC->CancelAbilityHandle(ActiveAbilitySpecHandle);
	}

	bool bActivated = false;
	int32 CandidateAbilityCount = 0;
	if (TargetClass)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetClass()->IsChildOf(TargetClass))
			{
				++CandidateAbilityCount;
				if (GraphInput == EYogComboGraphInputAction::Dash)
				{
					PendingAbilityNode = *NextNode;
					bPendingAbilityNodeValid = true;
				}
				bActivated = ASC->TryActivateAbility(Spec.Handle, true);
				if (bActivated)
				{
					break;
				}
				if (GraphInput == EYogComboGraphInputAction::Dash)
				{
					PendingAbilityNode = FWeaponComboNodeConfig();
					bPendingAbilityNodeValid = false;
				}
			}
		}
	}

	if (!bActivated)
	{
		const FGameplayTag ActivationTag = GetActivationTagForInput(GraphInput);
		if (ActivationTag.IsValid())
		{
			FGameplayTagContainer ActivationTags;
			ActivationTags.AddTag(ActivationTag);
			if (GraphInput == EYogComboGraphInputAction::Dash)
			{
				PendingAbilityNode = *NextNode;
				bPendingAbilityNodeValid = true;
			}
			bActivated = ASC->TryActivateAbilitiesByTag(ActivationTags, true);
			if (!bActivated && GraphInput == EYogComboGraphInputAction::Dash)
			{
				PendingAbilityNode = FWeaponComboNodeConfig();
				bPendingAbilityNodeValid = false;
			}
		}
	}

	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate node=%s input=%s current=%s montage=%s montageConfig=%s attackType=%s ability=%s candidates=%d"),
			*NextNode->NodeId.ToString(),
			*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)),
			*GetCurrentNodeId().ToString(),
			*GetNameSafe(NextNode->Montage.Get()),
			*GetNameSafe(NextNode->MontageConfig.Get()),
			*StaticEnum<EYogComboGraphAttackType>()->GetNameStringByValue(static_cast<int64>(NextNode->AttackType)),
			TargetClass ? *TargetClass->GetName() : TEXT("None"),
			CandidateAbilityCount);
		ActiveNode = FWeaponComboNodeConfig();
		ClearPreparedComboActivation();
		return false;
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("[ComboRuntime] Activated node=%s input=%s previous=%s child=%d montage=%s ability=%s candidates=%d"),
		*NextNode->NodeId.ToString(),
		*StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(GraphInput)),
		*StartNodeId.ToString(),
		bFoundChildNode ? 1 : 0,
		*GetNameSafe(NextNode->Montage.Get()),
		TargetClass ? *TargetClass->GetName() : TEXT("None"),
		CandidateAbilityCount);

	CommitPreparedComboActivation();
	// Do NOT clear ActiveAbilitySpecHandle here — RegisterActiveAttackAbility already set it
	// to the new activation's handle during TryActivateAbility. Clearing it would
	// prevent CancelAbilityHandle from cancelling the current ability on the next combo hit.

	return true;
}

void UComboRuntimeComponent::ResetCombo()
{
	// Reset clears the live (active graph) state, so its preserved cursor is now moot;
	// drop only that entry and leave other graphs' cursors intact.
	if (UGameplayAbilityComboGraph* Active = GetComboGraph())
	{
		GraphContexts.Remove(Active);
	}
	bSuppressStaleClearOnce = false;

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

bool UComboRuntimeComponent::ConsumePendingAbilityNode(FWeaponComboNodeConfig& OutNode)
{
	if (!bPendingAbilityNodeValid)
	{
		return false;
	}

	OutNode = PendingAbilityNode;
	PendingAbilityNode = FWeaponComboNodeConfig();
	bPendingAbilityNodeValid = false;
	return true;
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

	if (GetActiveAttackGuid().IsValid())
	{
		return true;
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
	bSuppressStaleClearOnce = false;
	ClearPreparedComboActivation();

	if (ASC)
	{
		ClearComboWindowAndProgressLooseTags(ASC);
	}
}

void UComboRuntimeComponent::EnsureAbilityGranted(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!ASC || !AbilityClass)
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass()->IsChildOf(AbilityClass))
		{
			return;
		}
	}

	ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
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
