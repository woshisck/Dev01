#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	bool IsLegacyComboProgressTag(const FGameplayTag& Tag)
	{
		const FString TagName = Tag.ToString();
		return TagName.StartsWith(TEXT("PlayerState.AbilityCast.LightAtk.Combo"))
			|| TagName.StartsWith(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo"));
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

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
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

	const bool bUseDashSavedNode = !SavedDashNodeId.IsNone();
	const FName StartNodeId = bUseDashSavedNode ? SavedDashNodeId : CurrentNodeId;

	FWeaponComboNodeConfig GraphNodeConfig;
	const FWeaponComboNodeConfig* NextNode = nullptr;
	bool bFoundChildNode = false;

	if (ComboGraph)
	{
		const UGameplayAbilityComboGraphNode* NextGraphNode = ComboGraph->FindChildComboNode(StartNodeId, InputAction);
		bFoundChildNode = NextGraphNode != nullptr;
		if (!NextGraphNode)
		{
			NextGraphNode = ComboGraph->FindRootComboNode(InputAction);
		}
		if (NextGraphNode)
		{
			GraphNodeConfig = NextGraphNode->BuildRuntimeConfig(InputAction);
			NextNode = &GraphNodeConfig;
		}
	}
	else if (ComboConfig)
	{
		NextNode = ComboConfig->FindChildNode(StartNodeId, InputAction);
		bFoundChildNode = NextNode != nullptr;
		if (!NextNode)
		{
			NextNode = ComboConfig->FindRootNode(InputAction);
		}
	}

	if (!NextNode || !NextNode->AbilityTag.IsValid())
	{
		if (!CurrentNodeId.IsNone() && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
		bComboContinued = false;
		bExitedComboState = !CurrentNodeId.IsNone();
		CurrentNodeId = NAME_None;
		ComboIndex = 0;
		ComboTags.Reset();
		return false;
	}

	ActiveNode = *NextNode;
	bActiveNodeValid = true;
	bActivationFromDashSave = bUseDashSavedNode;
	bComboContinued = bFoundChildNode || bUseDashSavedNode;
	bExitedComboState = !CurrentNodeId.IsNone() && !bFoundChildNode && !bUseDashSavedNode;
	ActiveAttackGuid = FGuid::NewGuid();

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(NextNode->AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, MatchingSpecs);

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
			// style progress tags are only compatibility gates. CanCombo/weapon/state
			// requirements still come from the real ASC state.
			if (IsLegacyComboProgressTag(RequiredTag) && ASC->GetTagCount(RequiredTag) <= 0)
			{
				TemporaryRequiredTags.AddTag(RequiredTag);
			}
		}
	}

	for (const FGameplayTag& TemporaryTag : TemporaryRequiredTags)
	{
		ASC->AddLooseGameplayTag(TemporaryTag);
	}

	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags, true);

	for (const FGameplayTag& TemporaryTag : TemporaryRequiredTags)
	{
		ASC->RemoveLooseGameplayTag(TemporaryTag);
	}

	if (!bActivated)
	{
		bActiveNodeValid = false;
		ActiveNode = FWeaponComboNodeConfig();
		ActiveAttackGuid.Invalidate();
		bActivationFromDashSave = false;
		bComboContinued = false;
		return false;
	}

	ComboIndex = bFoundChildNode || bUseDashSavedNode
		? FMath::Max(1, ComboIndex + 1)
		: 1;
	ComboTags.Reset();
	if (NextNode->AbilityTag.IsValid())
	{
		ComboTags.AddTag(NextNode->AbilityTag);
	}

	CurrentNodeId = NextNode->NodeId;
	SavedDashNodeId = NAME_None;
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
	ActiveAttackGuid.Invalidate();
	ComboIndex = 0;
	ComboTags.Reset();
	bActiveNodeValid = false;
	bActivationFromDashSave = false;
	bComboContinued = false;
	bExitedComboState = true;
	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner()))
	{
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
	}
}

void UComboRuntimeComponent::SaveCurrentNodeForDash()
{
	if (bActiveNodeValid && ActiveNode.bAllowDashSave)
	{
		SavedDashNodeId = ActiveNode.NodeId;
	}
}

const FWeaponComboNodeConfig* UComboRuntimeComponent::GetActiveNode() const
{
	return bActiveNodeValid ? &ActiveNode : nullptr;
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
