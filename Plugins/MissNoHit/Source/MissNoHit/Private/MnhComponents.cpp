// Copyright 2024 Eren Balatkan. All Rights Reserved.


#include "MnhComponents.h"


// Sets default values for this component's properties
UMnhSphereComponent::UMnhSphereComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	UPrimitiveComponent::SetSimulatePhysics(false);
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CanCharacterStepUpOn = ECB_No;
	SetGenerateOverlapEvents(false);
}

UMnhBoxComponent::UMnhBoxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	UPrimitiveComponent::SetSimulatePhysics(false);
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CanCharacterStepUpOn = ECB_No;
	SetGenerateOverlapEvents(false);
}

UMnhCapsuleComponent::UMnhCapsuleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	UPrimitiveComponent::SetSimulatePhysics(false);
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CanCharacterStepUpOn = ECB_No;
	SetGenerateOverlapEvents(false);
}
