#include "Animation/ANS_MontageVFXBinding.h"

#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/MontageVFXBindingComponent.h"

void UANS_MontageVFXBinding::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || SlotName.IsNone())
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(MeshComp->GetOwner());
	if (!Player || !Player->MontageVFXBindingComponent)
	{
		return;
	}

	FActionData ActionData;
	const FActionData* ActionDataPtr = nullptr;
	if (UYogAbilitySystemComponent* ASC = Player->GetASC())
	{
		if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
		{
			ActionData = MeleeGA->GetAbilityActionData();
			ActionDataPtr = &ActionData;
		}
	}

	Player->MontageVFXBindingComponent->ActivateSlot(SlotName, ActionDataPtr);
}

void UANS_MontageVFXBinding::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp || SlotName.IsNone())
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(MeshComp->GetOwner());
	if (!Player || !Player->MontageVFXBindingComponent)
	{
		return;
	}

	Player->MontageVFXBindingComponent->DeactivateSlot(SlotName);
}

FString UANS_MontageVFXBinding::GetNotifyName_Implementation() const
{
	if (!SlotName.IsNone())
	{
		return FString::Printf(TEXT("VFX Binding [%s]"), *SlotName.ToString());
	}
	return TEXT("Montage VFX Binding");
}
