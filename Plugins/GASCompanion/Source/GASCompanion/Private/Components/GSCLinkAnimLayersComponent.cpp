// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Components/GSCLinkAnimLayersComponent.h"

#include "GSCLog.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

void UGSCLinkAnimLayersComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GSC_LOG(Verbose, TEXT("Link Anim Layers Component Begin Play"))
	LinkAnimLayers();
}

void UGSCLinkAnimLayersComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GSC_LOG(Verbose, TEXT("Link Anim Layers Component End Play"))
	UnlinkAnimLayers();

	Super::EndPlay(EndPlayReason);
}

void UGSCLinkAnimLayersComponent::LinkAnimLayer(const TSubclassOf<UAnimInstance> AnimInstance)
{
	if (USkeletalMeshComponent* Mesh = GetOwnerSkeletalMeshComponent())
	{
		GSC_LOG(Verbose, TEXT("Linking Anim Layer %s"), *GetNameSafe(AnimInstance))
		Mesh->LinkAnimClassLayers(AnimInstance);
	}
}

void UGSCLinkAnimLayersComponent::UnlinkAnimLayer(const TSubclassOf<UAnimInstance> AnimInstance)
{
	if (USkeletalMeshComponent* Mesh = GetOwnerSkeletalMeshComponent())
	{
		GSC_LOG(Verbose, TEXT("Unlinking Anim Layer %s"), *GetNameSafe(AnimInstance))
		Mesh->UnlinkAnimClassLayers(AnimInstance);
	}
}

USkeletalMeshComponent* UGSCLinkAnimLayersComponent::GetOwnerSkeletalMeshComponent() const
{
	if (const ACharacter* Owner = GetPawn<ACharacter>())
	{
		return Owner->GetMesh();
	}

	return nullptr;
}

void UGSCLinkAnimLayersComponent::LinkAnimLayers()
{
	if (!GetOwnerSkeletalMeshComponent())
	{
		return;
	}
	
	for (const TSubclassOf<UAnimInstance>& LayerType : LayerTypes)
	{
		LinkAnimLayer(LayerType);
	}
}

void UGSCLinkAnimLayersComponent::UnlinkAnimLayers()
{
	if (!GetOwnerSkeletalMeshComponent())
	{
		return;
	}
	
	for (const TSubclassOf<UAnimInstance>& LayerType : LayerTypes)
	{
		UnlinkAnimLayer(LayerType);
	}
}
