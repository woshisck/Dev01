#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "Character/PlayerCharacterBase.h"
#include "Animation/AnimMontage.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
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

	if (!NextNode || (!NextNode->Montage && !NextNode->MontageConfig))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] No combo node for input=%s current=%s graph=%s config=%s"),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
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
		return false;
	}

	ActiveNode = *NextNode;
	bActiveNodeValid = true;
	bActivationFromDashSave = bUseDashSavedNode;
	bComboContinued = bFoundChildNode || bUseDashSavedNode;
	bExitedComboState = !CurrentNodeId.IsNone() && !bFoundChildNode && !bUseDashSavedNode;
	ActiveAttackGuid = FGuid::NewGuid();

	if (bExitedComboState)
	{
		FGameplayTagContainer TagsToCancel;
		AddLegacyComboProgressTags(TagsToCancel);
		ASC->CancelAbilities(&TagsToCancel);
		ClearComboWindowAndProgressLooseTags(ASC);
	}

	const bool bActivated = ASC->TryActivateAbilityByClass(UGA_PlayMontage::StaticClass());

	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate node=%s input=%s current=%s montage=%s montageConfig=%s"),
			*NextNode->NodeId.ToString(),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
			*CurrentNodeId.ToString(),
			*GetNameSafe(NextNode->Montage.Get()),
			*GetNameSafe(NextNode->MontageConfig.Get()));
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
