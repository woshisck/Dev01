#include "Animation/ANS_MontageVFXBinding.h"

#include "Animation/AN_MeleeDamage.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Component/MontageVFXBindingComponent.h"

namespace
{
	const UAN_MeleeDamage* FindFirstMeleeDamageNotify(const UAnimSequenceBase* Animation)
	{
		if (!Animation)
		{
			return nullptr;
		}

		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			if (const UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(Event.Notify))
			{
				return DamageNotify;
			}
		}

		return nullptr;
	}
}

void UANS_MontageVFXBinding::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || SlotName.IsNone())
	{
		return;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	UMontageVFXBindingComponent* VFXBindingComponent = Character
		? Character->FindComponentByClass<UMontageVFXBindingComponent>()
		: nullptr;
	if (!Character || !VFXBindingComponent)
	{
		return;
	}

	FActionData ActionData;
	const FActionData* ActionDataPtr = nullptr;
	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
		{
			ActionData = MeleeGA->GetAbilityActionData();
			ActionDataPtr = &ActionData;
		}
	}

	if (!ActionDataPtr)
	{
		if (const UAN_MeleeDamage* DamageNotify = FindFirstMeleeDamageNotify(Animation))
		{
			ActionData = DamageNotify->BuildActionData();
			ActionDataPtr = &ActionData;
		}
	}

	VFXBindingComponent->ActivateSlot(SlotName, ActionDataPtr, RemainTime);
}

void UANS_MontageVFXBinding::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp || SlotName.IsNone())
	{
		return;
	}

	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	UMontageVFXBindingComponent* VFXBindingComponent = Character
		? Character->FindComponentByClass<UMontageVFXBindingComponent>()
		: nullptr;
	if (!VFXBindingComponent)
	{
		return;
	}

	VFXBindingComponent->DeactivateSlot(SlotName);
}

FString UANS_MontageVFXBinding::GetNotifyName_Implementation() const
{
	if (!SlotName.IsNone())
	{
		return FString::Printf(TEXT("VFX Binding [%s]"), *SlotName.ToString());
	}
	return TEXT("Montage VFX Binding");
}
