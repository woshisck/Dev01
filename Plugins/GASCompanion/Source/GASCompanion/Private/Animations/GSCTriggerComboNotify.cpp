// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Animations/GSCTriggerComboNotify.h"

#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Components/GSCComboManagerComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UGSCTriggerComboNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	const AActor* Owner = GetOwnerActor(MeshComp);
	if (!Owner)
	{
		return;
	}

	// run only on server
	if (!Owner->HasAuthority())
	{
		return;
	}

	UGSCComboManagerComponent* ComboManagerComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(Owner);
	if (!ComboManagerComponent)
	{
		return;
	}

	ComboManagerComponent->bRequestTriggerCombo = true;
}

FString UGSCTriggerComboNotify::GetNotifyName_Implementation() const
{
	return "GSC Trigger Combo";
}

AActor* UGSCTriggerComboNotify::GetOwnerActor(USkeletalMeshComponent* MeshComponent) const
{
	AActor* OwnerActor = MeshComponent->GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	const FString ActorName = OwnerActor->GetName();
	if (ActorName.StartsWith(AnimationEditorPreviewActorString))
	{
		return nullptr;
	}

	return OwnerActor;
}
