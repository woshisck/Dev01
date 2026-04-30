#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Item/Weapon/WeaponDefinition.h"

UComboRuntimeComponent::UComboRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboRuntimeComponent::LoadComboConfig(UWeaponComboConfigDA* InComboConfig)
{
	ComboConfig = InComboConfig;
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

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	if (!ComboConfig || !PlayerOwner)
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

	const FWeaponComboNodeConfig* NextNode = ComboConfig->FindChildNode(StartNodeId, InputAction);
	const bool bFoundChildNode = NextNode != nullptr;
	if (!NextNode)
	{
		NextNode = ComboConfig->FindRootNode(InputAction);
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

	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags, true);
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
