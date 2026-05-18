#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	FGameplayTag CardActionToInputTag(ECardRequiredAction Action)
	{
		static const FName LightName(TEXT("Combo.Input.Light"));
		static const FName HeavyName(TEXT("Combo.Input.Heavy"));
		switch (Action)
		{
			case ECardRequiredAction::Light: return FGameplayTag::RequestGameplayTag(LightName, false);
			case ECardRequiredAction::Heavy: return FGameplayTag::RequestGameplayTag(HeavyName, false);
			case ECardRequiredAction::Any:   return FGameplayTag();
			default:                          return FGameplayTag();
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

	if (!GetCurrentNodeId().IsNone() && !bUseDashSavedNode)
	{
		static const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (ASC->GetTagCount(CanComboTag) <= 0)
		{
			return false;
		}
	}

	FWeaponComboNodeConfig NextNodeConfig;
	const FWeaponComboNodeConfig* NextNode = nullptr;
	bool bFoundChildNode = false;

	if (GetComboGraph())
	{
		const FGameplayTag InputTag = CardActionToInputTag(InputAction);
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);

		FYogComboGraphNodeSelection Selection;
		if (FindNextComboGraphNode(InputTag, &OwnedTags, Selection))
		{
			NextNodeConfig = FWeaponComboNodeConfig::FromComboGraphNode(Selection.Node, InputAction);
			NextNode = &NextNodeConfig;
			bFoundChildNode = Selection.bFoundChildNode;
			PrepareComboGraphNodeActivation(Selection);
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
		if (NextNode)
		{
			PrepareComboNodeActivation(NextNode->NodeId, bFoundChildNode, bUseDashSavedNode);
		}
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

	ActiveNode = *NextNode;

	if (DidExitComboState())
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
			*GetCurrentNodeId().ToString(),
			*GetNameSafe(NextNode->Montage.Get()),
			*GetNameSafe(NextNode->MontageConfig.Get()));
		ActiveNode = FWeaponComboNodeConfig();
		ClearPreparedComboActivation();
		return false;
	}

	CommitPreparedComboActivation();
	if (bUseDashSavedNode && PlayerOwner->CombatDeckComponent)
	{
		PlayerOwner->CombatDeckComponent->RestorePendingLinkContextFromDash();
	}
	return true;
}

void UComboRuntimeComponent::ResetCombo()
{
	Super::ResetCombo();
	ActiveNode = FWeaponComboNodeConfig();
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
	if (const FWeaponComboNodeConfig* Node = GetActiveNode())
	{
		if (Node->bAllowDashSave)
		{
			SaveComboNodeForDash(Node->NodeId);
		}
	}
}

void UComboRuntimeComponent::NotifyMontageStarted()
{
	if (GetActiveGraphNode())
	{
		Super::NotifyMontageStarted();
		return;
	}

	if (const FWeaponComboNodeConfig* Node = GetActiveNode())
	{
		PlayFxBinding(Node->OnMontageStartFx);
	}
}

void UComboRuntimeComponent::NotifyHitLanded()
{
	if (GetActiveGraphNode())
	{
		Super::NotifyHitLanded();
		return;
	}

	if (const FWeaponComboNodeConfig* Node = GetActiveNode())
	{
		PlayFxBinding(Node->OnHitSuccessFx);
		ApplyHitDilation(Node->HitSuccessDilation);
	}
}

const FWeaponComboNodeConfig* UComboRuntimeComponent::GetActiveNode() const
{
	return GetActiveGraphNodeId().IsNone() ? nullptr : &ActiveNode;
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
	Context.AbilityTag = ActiveNode.AbilityTag;
	Context.WeaponDef = PlayerOwner ? PlayerOwner->EquippedWeaponDef : nullptr;
	Context.bIsComboFinisher = ActiveNode.bIsComboFinisher;
	Context.bComboContinued = DidComboContinue();
	Context.bExitedComboState = DidExitComboState();
	Context.bFromDashSave = WasActivationFromDashSave();
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = GetActiveAttackGuid();
	return Context;
}
