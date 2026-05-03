#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

namespace
{
	ECombatGraphInputAction ToGraphInputAction(ECardRequiredAction InputAction)
	{
		switch (InputAction)
		{
		case ECardRequiredAction::Light:
			return ECombatGraphInputAction::Light;
		case ECardRequiredAction::Heavy:
			return ECombatGraphInputAction::Heavy;
		case ECardRequiredAction::Any:
		default:
			return ECombatGraphInputAction::Any;
		}
	}

	FString GetGraphInputName(ECombatGraphInputAction InputAction)
	{
		return StaticEnum<ECombatGraphInputAction>()
			? StaticEnum<ECombatGraphInputAction>()->GetNameStringByValue(static_cast<int64>(InputAction))
			: FString(TEXT("Input"));
	}

	bool IsLegacyComboProgressTag(const FGameplayTag& Tag)
	{
		const FString TagName = Tag.ToString();
		return TagName.StartsWith(TEXT("PlayerState.AbilityCast.LightAtk.Combo"))
			|| TagName.StartsWith(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo"));
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

		const FGameplayTag DashSavePointTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Combo.DashSavePoint"), false);
		if (DashSavePointTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(DashSavePointTag, 0);
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
}

UComboRuntimeComponent::UComboRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboRuntimeComponent::LoadComboConfig(UWeaponComboConfigDA* InComboConfig)
{
	ComboConfig = InComboConfig;
	ComboGraph = nullptr;
	ResetCombo();

	if (!ComboConfig)
	{
		return;
	}

	TArray<FText> Warnings;
	ComboConfig->ValidateConfig(Warnings);
	for (const FText& Warning : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponComboConfig] %s: %s"), *GetNameSafe(ComboConfig), *Warning.ToString());
	}
}

void UComboRuntimeComponent::LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	ComboGraph = InComboGraph;
	ComboConfig = nullptr;
	ResetCombo();

	if (!ComboGraph)
	{
		return;
	}

	TArray<FText> Warnings;
	ComboGraph->ValidateComboGraph(Warnings);
	for (const FText& Warning : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameplayAbilityComboGraph] %s: %s"), *GetNameSafe(ComboGraph), *Warning.ToString());
	}
}

bool UComboRuntimeComponent::HasDashInputNode() const
{
	return ComboGraph && ComboGraph->FindRootComboNode(ECombatGraphInputAction::Dash) != nullptr;
}

bool UComboRuntimeComponent::TryActivateDash(APlayerCharacterBase* PlayerOwner)
{
	if (!ComboGraph || !PlayerOwner)
	{
		return false;
	}

	const UGameplayAbilityComboGraphNode* DashGraphNode = ComboGraph->FindRootComboNode(ECombatGraphInputAction::Dash);
	if (!DashGraphNode)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	FWeaponComboNodeConfig DashNode = DashGraphNode->BuildRuntimeConfig(ECombatGraphInputAction::Dash);
	if (!DashNode.AbilityTag.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Dash graph node=%s has no ability tag."),
			*DashNode.NodeId.ToString());
		return false;
	}

	const FName PreviousSavedDashNodeId = SavedDashNodeId;
	const bool bSavedComboNode = SaveCurrentNodeForDash(DashNode.DashSaveMode, DashNode.DashSaveExpireSeconds);
	if (bSavedComboNode && DashNode.bSavePendingLinkContext && PlayerOwner->CombatDeckComponent)
	{
		PlayerOwner->CombatDeckComponent->SavePendingLinkContextForDash();
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(DashNode.AbilityTag);
	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags, true);
	if (!bActivated)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DashSaveExpireTimerHandle);
		}
		SavedDashNodeId = PreviousSavedDashNodeId;
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->ClearDashSavedLinkContext();
		}

		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate dash node=%s ability=%s savedCombo=%d owned=%s"),
			*DashNode.NodeId.ToString(),
			*DashNode.AbilityTag.ToString(),
			bSavedComboNode ? 1 : 0,
			*OwnedTags.ToStringSimple());
		return false;
	}

	ActiveDashNode = DashNode;
	bActiveDashNodeValid = true;
	return true;
}

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	return TryActivateCombo(ToGraphInputAction(InputAction), PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateCombo(ECombatGraphInputAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	if ((!ComboConfig && !ComboGraph) || !PlayerOwner)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	if (InputAction == ECombatGraphInputAction::Dash)
	{
		return TryActivateDash(PlayerOwner);
	}

	const bool bUseDashSavedNode = !SavedDashNodeId.IsNone();
	const FName StartNodeId = bUseDashSavedNode ? SavedDashNodeId : CurrentNodeId;
	const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
	const bool bHasActiveAttackNode = !bUseDashSavedNode
		&& !StartNodeId.IsNone()
		&& (ActiveAttackGuid.IsValid() || ActiveAbilitySpecHandle.IsValid());

	FWeaponComboNodeConfig GraphNodeConfig;
	const FWeaponComboNodeConfig* NextNode = nullptr;
	bool bFoundChildNode = false;
	bool bBlockedRootFallbackDuringActiveNode = false;

	if (ComboGraph)
	{
		const UGameplayAbilityComboGraphNode* NextGraphNode = ComboGraph->FindChildComboNode(StartNodeId, InputAction);
		bFoundChildNode = NextGraphNode != nullptr;
		if (!NextGraphNode)
		{
			if (bHasActiveAttackNode)
			{
				bBlockedRootFallbackDuringActiveNode = true;
			}
			else
			{
				NextGraphNode = ComboGraph->FindRootComboNode(InputAction);
			}
		}
		if (NextGraphNode)
		{
			GraphNodeConfig = NextGraphNode->BuildRuntimeConfig(InputAction);
			NextNode = &GraphNodeConfig;
		}
	}
	else if (ComboConfig)
	{
		const ECardRequiredAction CardInputAction = InputAction == ECombatGraphInputAction::Heavy
			? ECardRequiredAction::Heavy
			: ECardRequiredAction::Light;
		NextNode = ComboConfig->FindChildNode(StartNodeId, CardInputAction);
		bFoundChildNode = NextNode != nullptr;
		if (!NextNode)
		{
			if (bHasActiveAttackNode)
			{
				bBlockedRootFallbackDuringActiveNode = true;
			}
			else
			{
				NextNode = ComboConfig->FindRootNode(CardInputAction);
			}
		}
	}

	if (bBlockedRootFallbackDuringActiveNode)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[ComboRuntime] Ignore input without child while active node is playing input=%s current=%s"),
			*GetGraphInputName(InputAction),
			*StartNodeId.ToString());
		return false;
	}

	if (!NextNode || !NextNode->AbilityTag.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] No combo node for input=%s current=%s graph=%s config=%s"),
			*GetGraphInputName(InputAction),
			*CurrentNodeId.ToString(),
			*GetNameSafe(ComboGraph),
			*GetNameSafe(ComboConfig));
		if (!CurrentNodeId.IsNone() && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
		bComboContinued = false;
		bExitedComboState = !CurrentNodeId.IsNone();
		CurrentNodeId = NAME_None;
		ComboIndex = 0;
		ComboTags.Reset();
		ClearRuntimeCombatLooseTags();
		return false;
	}

	if (bFoundChildNode && !bUseDashSavedNode)
	{
		if (CanComboTag.IsValid() && ASC->GetTagCount(CanComboTag) <= 0)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[ComboRuntime] Queue child activation until CanCombo opens input=%s current=%s next=%s"),
				*GetGraphInputName(InputAction),
				*CurrentNodeId.ToString(),
				*NextNode->NodeId.ToString());
			return false;
		}
	}

	const FWeaponComboNodeConfig PreviousActiveNode = ActiveNode;
	const FGuid PreviousAttackGuid = ActiveAttackGuid;
	const FGameplayAbilitySpecHandle PreviousAbilitySpecHandle = ActiveAbilitySpecHandle;
	const bool bPreviousActiveNodeValid = bActiveNodeValid;
	const bool bPreviousActivationFromDashSave = bActivationFromDashSave;
	const bool bPreviousComboContinued = bComboContinued;
	const bool bPreviousExitedComboState = bExitedComboState;
	const FName PreviousCurrentNodeId = CurrentNodeId;
	const FName PreviousSavedDashNodeId = SavedDashNodeId;
	const int32 PreviousComboIndex = ComboIndex;
	const FGameplayTagContainer PreviousComboTags = ComboTags;

	const int32 NextComboIndex = bFoundChildNode || bUseDashSavedNode
		? FMath::Max(1, ComboIndex + 1)
		: 1;

	ActiveNode = *NextNode;
	bActiveNodeValid = true;
	bActivationFromDashSave = bUseDashSavedNode;
	bComboContinued = bFoundChildNode || bUseDashSavedNode;
	bExitedComboState = !CurrentNodeId.IsNone() && !bFoundChildNode && !bUseDashSavedNode;
	ActiveAttackGuid = FGuid::NewGuid();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
	ComboIndex = NextComboIndex;
	ComboTags.Reset();
	if (NextNode->AbilityTag.IsValid())
	{
		ComboTags.AddTag(NextNode->AbilityTag);
	}
	CurrentNodeId = NextNode->NodeId;

	const bool bRestartingFromRoot = bExitedComboState;
	if (bRestartingFromRoot)
	{
		FGameplayTagContainer TagsToCancel;
		AddLegacyComboProgressTags(TagsToCancel);
		if (ActiveNode.AbilityTag.IsValid())
		{
			TagsToCancel.AddTag(ActiveNode.AbilityTag);
		}
		ASC->CancelAbilities(&TagsToCancel);
		ClearComboWindowAndProgressLooseTags(ASC);
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(NextNode->AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, MatchingSpecs, false);
	if (MatchingSpecs.IsEmpty() && NextNode->GameplayAbilityClass)
	{
		if (FGameplayAbilitySpec* ClassSpec = ASC->FindAbilitySpecFromClass(NextNode->GameplayAbilityClass))
		{
			MatchingSpecs.Add(ClassSpec);
		}
	}

	bool bCancelledPreviousAbilityForTransition = false;
	if (bFoundChildNode && !bUseDashSavedNode && PreviousAbilitySpecHandle.IsValid() && MatchingSpecs.Num() > 0)
	{
		ASC->CancelAbilityHandle(PreviousAbilitySpecHandle);
		bCancelledPreviousAbilityForTransition = true;
	}

	FGameplayTagContainer TemporaryRequiredTags;
	for (FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		UYogGameplayAbility* YogAbility = Cast<UYogGameplayAbility>(Spec->Ability);
		if (!YogAbility)
		{
			continue;
		}

		for (const FGameplayTag& RequiredTag : YogAbility->GetActivationRequiredTags())
		{
			// The config chooses the branch now, so legacy "Light2 requires Light1"
			// style progress tags are only compatibility gates. During a child
			// transition, the previous montage is cancelled before activation, so
			// preserve CanCombo for old GA RequiredTags on this activation frame.
			const bool bNeedsTransitionCanCombo =
				(bFoundChildNode || bUseDashSavedNode) &&
				CanComboTag.IsValid() &&
				RequiredTag == CanComboTag;
			if ((IsLegacyComboProgressTag(RequiredTag) || bNeedsTransitionCanCombo) && ASC->GetTagCount(RequiredTag) <= 0)
			{
				TemporaryRequiredTags.AddTag(RequiredTag);
			}
		}
	}

	for (const FGameplayTag& TemporaryTag : TemporaryRequiredTags)
	{
		ASC->AddLooseGameplayTag(TemporaryTag);
		TrackRuntimeCombatLooseTag(TemporaryTag);
	}

	bool bActivated = false;
	if (!MatchingSpecs.IsEmpty())
	{
		for (FGameplayAbilitySpec* Spec : MatchingSpecs)
		{
			if (Spec && ASC->TryActivateAbility(Spec->Handle, true))
			{
				bActivated = true;
				break;
			}
		}
	}
	else
	{
		bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags, true);
	}

	for (const FGameplayTag& TemporaryTag : TemporaryRequiredTags)
	{
		if (ASC->GetTagCount(TemporaryTag) > 0)
		{
			ASC->RemoveLooseGameplayTag(TemporaryTag);
		}
	}

	if (!bActivated)
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate node=%s ability=%s input=%s current=%s montageConfig=%s specs=%d tempRequired=%s owned=%s"),
			*NextNode->NodeId.ToString(),
			*NextNode->AbilityTag.ToString(),
			*GetGraphInputName(InputAction),
			*CurrentNodeId.ToString(),
			*GetNameSafe(NextNode->MontageConfig.Get()),
			MatchingSpecs.Num(),
			*TemporaryRequiredTags.ToStringSimple(),
			*OwnedTags.ToStringSimple());
		bActiveNodeValid = false;
		ActiveNode = FWeaponComboNodeConfig();
		ActiveAttackGuid.Invalidate();
		ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
		bActivationFromDashSave = false;
		bComboContinued = false;
		if (bFoundChildNode && !bUseDashSavedNode && PreviousAttackGuid.IsValid() && !bCancelledPreviousAbilityForTransition)
		{
			ActiveNode = PreviousActiveNode;
			ActiveAttackGuid = PreviousAttackGuid;
			ActiveAbilitySpecHandle = PreviousAbilitySpecHandle;
			bActiveNodeValid = bPreviousActiveNodeValid;
			bActivationFromDashSave = bPreviousActivationFromDashSave;
			bComboContinued = bPreviousComboContinued;
			bExitedComboState = bPreviousExitedComboState;
			CurrentNodeId = PreviousCurrentNodeId;
			SavedDashNodeId = PreviousSavedDashNodeId;
			ComboIndex = PreviousComboIndex;
			ComboTags = PreviousComboTags;
		}
		else if (bCancelledPreviousAbilityForTransition)
		{
			if (!CurrentNodeId.IsNone() && PlayerOwner->CombatDeckComponent)
			{
				PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
			}
			CurrentNodeId = NAME_None;
			ComboIndex = 0;
			ComboTags.Reset();
			bExitedComboState = true;
		}
		else
		{
			CurrentNodeId = PreviousCurrentNodeId;
			SavedDashNodeId = PreviousSavedDashNodeId;
			ComboIndex = PreviousComboIndex;
			ComboTags = PreviousComboTags;
			bExitedComboState = bPreviousExitedComboState;
		}
		return false;
	}

	SavedDashNodeId = NAME_None;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashSaveExpireTimerHandle);
	}
	if (bUseDashSavedNode && PlayerOwner->CombatDeckComponent)
	{
		PlayerOwner->CombatDeckComponent->RestorePendingLinkContextFromDash();
	}
	return true;
}

void UComboRuntimeComponent::ResetCombo()
{
	CurrentNodeId = NAME_None;
	SavedDashNodeId = NAME_None;
	ActiveNode = FWeaponComboNodeConfig();
	ActiveDashNode = FWeaponComboNodeConfig();
	ActiveAttackGuid.Invalidate();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
	ComboIndex = 0;
	ComboTags.Reset();
	bActiveNodeValid = false;
	bActiveDashNodeValid = false;
	bActivationFromDashSave = false;
	bComboContinued = false;
	bExitedComboState = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashSaveExpireTimerHandle);
	}
	ClearRuntimeCombatLooseTags();
	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner()))
	{
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
	}
}

bool UComboRuntimeComponent::SaveCurrentNodeForDash(EComboDashSaveMode SaveMode, float ExpireSeconds)
{
	if (SaveMode == EComboDashSaveMode::None)
	{
		ClearSavedDashNode();
		return false;
	}

	const bool bSourceAllowsSave = bActiveNodeValid && ActiveNode.bAllowDashSave;
	const bool bForcePreserve = SaveMode == EComboDashSaveMode::ForcePreserve;
	const FName SourceNodeId = bActiveNodeValid ? ActiveNode.NodeId : CurrentNodeId;
	if (SourceNodeId.IsNone() || (!bForcePreserve && !bSourceAllowsSave))
	{
		ClearSavedDashNode();
		return false;
	}

	SavedDashNodeId = SourceNodeId;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashSaveExpireTimerHandle);
		if (ExpireSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				DashSaveExpireTimerHandle,
				this,
				&UComboRuntimeComponent::ExpireSavedDashNode,
				ExpireSeconds,
				false);
		}
	}

	return true;
}

void UComboRuntimeComponent::ClearSavedDashNode()
{
	SavedDashNodeId = NAME_None;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashSaveExpireTimerHandle);
	}

	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner()))
	{
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->ClearDashSavedLinkContext();
		}
	}
}

void UComboRuntimeComponent::NotifyDashEnded(bool bWasCancelled)
{
	const bool bClearCombatTags = !bActiveDashNodeValid || ActiveDashNode.bClearCombatTagsOnDashEnd;
	const bool bBreakOnCancel = !bActiveDashNodeValid || ActiveDashNode.bBreakComboOnDashCancel;

	if (bWasCancelled && bBreakOnCancel)
	{
		ClearSavedDashNode();
	}

	if (bClearCombatTags)
	{
		ClearRuntimeCombatLooseTags();
	}

	ActiveDashNode = FWeaponComboNodeConfig();
	bActiveDashNodeValid = false;
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
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC))
	{
		YogASC->ConsumeDashSave();
	}
	RuntimeCombatLooseTags.Reset();
}

void UComboRuntimeComponent::TrackRuntimeCombatLooseTag(const FGameplayTag& Tag)
{
	if (Tag.IsValid())
	{
		RuntimeCombatLooseTags.AddTag(Tag);
	}
}

void UComboRuntimeComponent::ExpireSavedDashNode()
{
	UE_LOG(LogTemp, Log,
		TEXT("[ComboRuntime] DashSave expired savedNode=%s"),
		*SavedDashNodeId.ToString());
	ClearSavedDashNode();
}

const FWeaponComboNodeConfig* UComboRuntimeComponent::GetActiveNode() const
{
	return bActiveNodeValid ? &ActiveNode : nullptr;
}

void UComboRuntimeComponent::RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle)
{
	if (AttackGuid.IsValid() && ActiveAttackGuid.IsValid() && ActiveAttackGuid == AttackGuid)
	{
		ActiveAbilitySpecHandle = AbilityHandle;
	}
}

bool UComboRuntimeComponent::HandleAttackAbilityEnded(const FGuid& EndedAttackGuid)
{
	if (!EndedAttackGuid.IsValid() || !ActiveAttackGuid.IsValid() || ActiveAttackGuid != EndedAttackGuid)
	{
		return false;
	}

	const bool bHadActiveNode = bActiveNodeValid || !CurrentNodeId.IsNone();

	CurrentNodeId = NAME_None;
	ActiveNode = FWeaponComboNodeConfig();
	ActiveAttackGuid.Invalidate();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
	ComboIndex = 0;
	ComboTags.Reset();
	bActiveNodeValid = false;
	bActivationFromDashSave = false;
	bComboContinued = false;
	bExitedComboState = true;

	if (bHadActiveNode && SavedDashNodeId.IsNone())
	{
		ClearRuntimeCombatLooseTags();
		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner()))
		{
			if (PlayerOwner->CombatDeckComponent)
			{
				PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
			}
		}
	}

	return bHadActiveNode;
}

bool UComboRuntimeComponent::ConsumeActivationFromDashSave()
{
	const bool bResult = bActivationFromDashSave;
	bActivationFromDashSave = false;
	return bResult;
}

FCombatDeckActionContext UComboRuntimeComponent::BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const
{
	FCombatDeckActionContext Context;
	if (!bActiveNodeValid)
	{
		return Context;
	}

	Context.ActionType = ActiveNode.InputAction;
	Context.ComboIndex = ComboIndex;
	Context.ComboNodeId = ActiveNode.NodeId;
	Context.ComboTags = ComboTags;
	Context.AbilityTag = ActiveNode.AbilityTag;
	Context.WeaponDef = PlayerOwner ? PlayerOwner->EquippedWeaponDef : nullptr;
	Context.bIsComboFinisher = ActiveNode.bIsComboFinisher;
	Context.bComboContinued = bComboContinued;
	Context.bExitedComboState = bExitedComboState;
	Context.bFromDashSave = bActivationFromDashSave;
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = ActiveAttackGuid;
	return Context;
}
