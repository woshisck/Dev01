// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "AnimNotifies/ComboGraphANS_ComboWindow.h"

#include "Abilities/ComboGraphNativeTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Utils/ComboGraphBlueprintLibrary.h"
#include "Utils/ComboGraphUtils.h"

#if ENGINE_MAJOR_VERSION == 5

// void UComboGraphANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const float TotalDuration)
void UComboGraphANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	SendEvent(MeshComp, FComboGraphNativeTags::Get().ComboBegin);
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

// void UComboGraphANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
void UComboGraphANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	SendEvent(MeshComp, FComboGraphNativeTags::Get().ComboEnd);
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

#else

void UComboGraphANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const float TotalDuration)
{
	SendEvent(MeshComp, FComboGraphNativeTags::Get().ComboBegin);
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}

void UComboGraphANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	SendEvent(MeshComp, FComboGraphNativeTags::Get().ComboEnd);
	Super::NotifyEnd(MeshComp, Animation);
}

#endif

void UComboGraphANS_ComboWindow::SendEvent(USkeletalMeshComponent* InMeshComp, FGameplayTag InGameplayTag)
{
	if (InMeshComp)
	{
		AActor* Owner = InMeshComp->GetOwner();
		
		// Avoid sending event for animation preview actor (notifiers are triggered in persona preview viewport)
		if (Owner && !FComboGraphUtils::IsAnimationPreviewActor(Owner))
		{
			UComboGraphBlueprintLibrary::SendGameplayEventToActor(Owner, InGameplayTag, FGameplayEventData());
		}
	}
}
