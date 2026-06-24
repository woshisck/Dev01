#include "Animation/AnimNotifyState_ComboWindow.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagContainer.h"

namespace
{
	FGameplayTag GetComboWindowTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"), false);
	}

	FGameplayTag GetLegacyComboWindowTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
	}

	UAbilitySystemComponent* GetASC(USkeletalMeshComponent* MeshComp)
	{
		if (!MeshComp)
		{
			return nullptr;
		}

		AActor* Owner = MeshComp->GetOwner();
		return Owner ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner) : nullptr;
	}
}

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
	{
		const FGameplayTag ComboWindowTag = GetComboWindowTag();
		const FGameplayTag LegacyComboWindowTag = GetLegacyComboWindowTag();
		if (ComboWindowTag.IsValid())
		{
			ASC->AddLooseGameplayTag(ComboWindowTag);
		}
		if (LegacyComboWindowTag.IsValid())
		{
			ASC->AddLooseGameplayTag(LegacyComboWindowTag);
		}
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
	{
		const FGameplayTag ComboWindowTag = GetComboWindowTag();
		const FGameplayTag LegacyComboWindowTag = GetLegacyComboWindowTag();
		if (ComboWindowTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(ComboWindowTag);
		}
		if (LegacyComboWindowTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(LegacyComboWindowTag);
		}
	}
}

FString UAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}
