#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "AbilitySystem/Abilities/GA_RangeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"

namespace
{
	ECardRequiredAction ToCardAction(ECombatGraphInputAction InputAction)
	{
		switch (InputAction)
		{
		case ECombatGraphInputAction::Light:
			return ECardRequiredAction::Light;
		case ECombatGraphInputAction::Heavy:
			return ECardRequiredAction::Heavy;
		case ECombatGraphInputAction::Dash:
		case ECombatGraphInputAction::Any:
		default:
			return ECardRequiredAction::Any;
		}
	}

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
	Super::LoadComboGraph(nullptr);

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
	ComboConfig = nullptr;
	Super::LoadComboGraph(InComboGraph);
}

bool UComboRuntimeComponent::HasDashInputNode() const
{
	return GetComboGraph() && GetComboGraph()->FindRootComboNode(EYogComboGraphInputAction::Dash) != nullptr;
}

bool UComboRuntimeComponent::TryActivateDash(APlayerCharacterBase* PlayerOwner)
{
	if (!GetComboGraph())
	{
		return false;
	}
	return TryActivateCombo(ECombatGraphInputAction::Dash, PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	if ((!ComboConfig && !HasComboGraph()) || !PlayerOwner)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const bool bUseDashSavedNode = HasSavedDashNode();
	const FName StartNodeId = GetComboStartNodeId();
	const bool bHasActiveAttackNode = !bUseDashSavedNode
		&& !StartNodeId.IsNone()
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
		const EYogComboGraphInputAction GraphInput = CardActionToComboGraphInput(InputAction);
		if (FindNextComboGraphNode(GraphInput, &OwnedTags, Selection))
		{
			if (bHasActiveAttackNode && !Selection.bFoundChildNode && !Selection.bFromDashSave)
			{
				bBlockedRootFallbackDuringActiveNode = true;
			}
			else
			{
				NextNodeConfig = FWeaponComboNodeConfig::FromComboGraphNode(Selection.Node, InputAction);
				NextNode = &NextNodeConfig;
				bFoundChildNode = Selection.bFoundChildNode;
				PrepareComboGraphNodeActivation(Selection);
			}
		}
	}
	else if (ComboConfig)
	{
		NextNode = ComboConfig->FindChildNode(StartNodeId, InputAction);
		bFoundChildNode = NextNode != nullptr;
		if (!NextNode)
		{
			if (bHasActiveAttackNode)
			{
				bBlockedRootFallbackDuringActiveNode = true;
			}
			else
			{
				NextNode = ComboConfig->FindRootNode(InputAction);
			}
		}
		if (NextNode)
		{
			PrepareComboNodeActivation(NextNode->NodeId, bFoundChildNode, bUseDashSavedNode);
		}
	}

	if (bBlockedRootFallbackDuringActiveNode)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[ComboRuntime] Ignore input without child while active node is playing input=%s current=%s"),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
			*StartNodeId.ToString());
		return false;
	}

	if (!NextNode || (!NextNode->Montage && !NextNode->MontageConfig))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] No combo node for input=%s current=%s graph=%s config=%s"),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
			*GetCurrentNodeId().ToString(),
			*GetNameSafe(GetComboGraph()),
			*GetNameSafe(ComboConfig));
		if (!GetCurrentNodeId().IsNone() && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
		ActiveNode = FWeaponComboNodeConfig();
		MarkComboActivationMiss();
		return false;
	}

	if (bFoundChildNode && !bUseDashSavedNode)
	{
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
		if (CanComboTag.IsValid() && ASC->GetTagCount(CanComboTag) <= 0)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[ComboRuntime] Queue child activation until CanCombo opens input=%s current=%s next=%s"),
				*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
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

	if (bFoundChildNode && !bUseDashSavedNode && ActiveAbilitySpecHandle.IsValid())
	{
		ASC->CancelAbilityHandle(ActiveAbilitySpecHandle);
	}

	// TEMP: route to GA_MeleeAttack / GA_RangeAttack based on the ComboGraph
	// node's AttackType. Previously hard-coded to UGA_PlayMontage, which doesn't
	// run the YogTargetType hit-collection pipeline (that lives on UGA_MeleeAttack).
	// GA_RangeAttack is a stub for now; flip the enum in the editor to test routing.
	TSubclassOf<UGameplayAbility> AbilityClass = (NextNode->AttackType == EYogComboGraphAttackType::Range)
		? TSubclassOf<UGameplayAbility>(UGA_RangeAttack::StaticClass())
		: TSubclassOf<UGameplayAbility>(UGA_MeleeAttack::StaticClass());

	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass, true);
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate node=%s input=%s current=%s montage=%s montageConfig=%s attackType=%s ability=%s"),
			*NextNode->NodeId.ToString(),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
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

	if (bUseDashSavedNode && PlayerOwner->CombatDeckComponent)
	{
		PlayerOwner->CombatDeckComponent->RestorePendingLinkContextFromDash();
	}

	static const FGameplayTag ChainActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Combo.ChainActive"), false);
	if (ChainActiveTag.IsValid() && ASC->GetTagCount(ChainActiveTag) <= 0)
	{
		ASC->AddLooseGameplayTag(ChainActiveTag);
		TrackRuntimeCombatLooseTag(ChainActiveTag);
	}

	return true;
}

bool UComboRuntimeComponent::TryActivateCombo(ECombatGraphInputAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	if (InputAction == ECombatGraphInputAction::Dash)
	{
		if ((!ComboConfig && !HasComboGraph()) || !PlayerOwner)
		{
			return false;
		}

		UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent();
		if (!ASC)
		{
			return false;
		}

		if (!GetComboGraph())
		{
			return false;
		}

		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);

		FYogComboGraphNodeSelection Selection;
		if (!FindNextComboGraphNode(EYogComboGraphInputAction::Dash, &OwnedTags, Selection))
		{
			return false;
		}

		FWeaponComboNodeConfig DashNode = FWeaponComboNodeConfig::FromComboGraphNode(Selection.Node, ECardRequiredAction::Any);
		const FName PreviousSavedDashNodeId = SavedDashNodeId;
		const bool bSavedComboNode = SaveCurrentNodeForDashWithPolicy(DashNode.DashSaveMode, DashNode.DashSaveExpireSeconds);
		if (bSavedComboNode && DashNode.bSavePendingLinkContext && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->SavePendingLinkContextForDash();
		}

		PrepareComboGraphNodeActivation(Selection);
		ActiveNode = DashNode;

		const bool bActivated = ASC->TryActivateAbilitiesByTag(
			FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash"))),
			true);
		if (!bActivated)
		{
			ClearPreparedComboActivation();
			ActiveNode = FWeaponComboNodeConfig();
			ClearSavedDashNode();
			if (!PreviousSavedDashNodeId.IsNone())
			{
				SaveComboNodeForDash(PreviousSavedDashNodeId);
			}
			if (PlayerOwner->CombatDeckComponent)
			{
				PlayerOwner->CombatDeckComponent->ClearDashSavedLinkContext();
			}
			return false;
		}

		CommitPreparedComboActivation();
		return true;
	}

	return TryActivateCombo(ToCardAction(InputAction), PlayerOwner);
}

void UComboRuntimeComponent::ResetCombo()
{
	Super::ResetCombo();
	ActiveNode = FWeaponComboNodeConfig();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
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

bool UComboRuntimeComponent::SaveCurrentNodeForDash()
{
	if (const FWeaponComboNodeConfig* Node = GetActiveNode())
	{
		return SaveCurrentNodeForDashWithPolicy(Node->DashSaveMode, Node->DashSaveExpireSeconds);
	}
	return false;
}

bool UComboRuntimeComponent::SaveCurrentNodeForDashWithPolicy(EComboDashSaveMode SaveMode, float ExpireSeconds)
{
	if (SaveMode == EComboDashSaveMode::None)
	{
		ClearSavedDashNode();
		return false;
	}

	const FWeaponComboNodeConfig* Node = GetActiveNode();
	const bool bSourceAllowsSave = Node && Node->bAllowDashSave;
	const bool bForcePreserve = SaveMode == EComboDashSaveMode::ForcePreserve;
	const FName SourceNodeId = Node ? Node->NodeId : GetCurrentNodeId();
	if (SourceNodeId.IsNone() || (!bForcePreserve && !bSourceAllowsSave))
	{
		ClearSavedDashNode();
		return false;
	}

	SaveComboNodeForDash(SourceNodeId);
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
	SaveComboNodeForDash(NAME_None);
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
	const bool bClearCombatTags = ActiveNode.bClearCombatTagsOnDashEnd;
	const bool bBreakOnCancel = ActiveNode.bBreakComboOnDashCancel;

	if (bWasCancelled && bBreakOnCancel)
	{
		ClearSavedDashNode();
	}

	if (bClearCombatTags)
	{
		ClearRuntimeCombatLooseTags();
	}

	// The dash ability never calls HandleAttackAbilityEnded, so ActiveAttackGuid and
	// ActiveAbilitySpecHandle from the committed dash node remain valid after the dash
	// ends. This makes bHasActiveAttackNode stay true, which blocks all root-fallback
	// attacks (since the dash node has no Light/Heavy children). Clear them here so the
	// player can attack again after dashing.
	ClearPreparedComboActivation();
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
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

const FWeaponComboNodeConfig* UComboRuntimeComponent::GetActiveNode() const
{
	return GetActiveGraphNodeId().IsNone() ? nullptr : &ActiveNode;
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

void UComboRuntimeComponent::ExpireSavedDashNode()
{
	UE_LOG(LogTemp, Log,
		TEXT("[ComboRuntime] DashSave expired savedNode=%s"),
		*GetComboStartNodeId().ToString());
	ClearSavedDashNode();
}

FCombatDeckActionContext UComboRuntimeComponent::BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const
{
	FCombatDeckActionContext Context;
	if (!GetActiveNode())
	{
		return Context;
	}

	Context.ActionType = ActiveNode.InputAction;
	Context.ComboIndex = GetComboIndex();
	Context.ComboNodeId = ActiveNode.NodeId;
	Context.ComboTags = GetComboTags();
	Context.WeaponDef = PlayerOwner ? PlayerOwner->EquippedWeaponDef : nullptr;
	Context.bIsComboFinisher = ActiveNode.bIsComboFinisher;
	Context.bComboContinued = DidComboContinue();
	Context.bExitedComboState = DidExitComboState();
	Context.bFromDashSave = WasActivationFromDashSave();
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = GetActiveAttackGuid();
	return Context;
}
