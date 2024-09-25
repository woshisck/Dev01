// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "AnimNotifies/ComboGraphANS_CollisionWindow.h"

#include "Components/ComboGraphCollisionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Utils/ComboGraphUtils.h"

#if ENGINE_MAJOR_VERSION == 5

void UComboGraphANS_CollisionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	ActivateCollision(MeshComp, true);
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UComboGraphANS_CollisionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	ActivateCollision(MeshComp, false);
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

#else

void UComboGraphANS_CollisionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	ActivateCollision(MeshComp, true);
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}

void UComboGraphANS_CollisionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ActivateCollision(MeshComp, false);
	Super::NotifyEnd(MeshComp, Animation);
}
#endif

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphANS_CollisionWindow::ActivateCollision(const USkeletalMeshComponent* MeshComp, const bool bShouldStart)
{
	if (MeshComp)
	{
		const AActor* Owner = MeshComp->GetOwner();
		if (!IsValid(Owner))
		{
			return;
		}

		// Only relevant for authority (Fix #8)
		if (!Owner->HasAuthority())
		{
			return;
		}

		// Avoid doing anything for animation preview actor (notifiers are triggered in persona preview viewport)
		if (!FComboGraphUtils::IsAnimationPreviewActor(Owner))
		{

			if (UComboGraphCollisionComponent* CollisionComponent = GetCollisionComponent(Owner))
			{
				if (bShouldStart)
				{
					CollisionComponent->StartTrace();

				}
				else
				{
					CollisionComponent->EndTrace();
				}
			}
		}
	}
}

UComboGraphCollisionComponent* UComboGraphANS_CollisionWindow::GetCollisionComponent(const AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	return OwnerActor->FindComponentByClass<UComboGraphCollisionComponent>();
}

