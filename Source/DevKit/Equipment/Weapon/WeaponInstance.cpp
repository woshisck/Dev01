// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponInstance.h"
#include "GameFramework/Character.h"
#include "WeaponDefinition.h"
UWeaponInstance::UWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UWeaponInstance::GetWorld() const
{
	if (APawn* OwningPawn = GetPawn())
	{
		return OwningPawn->GetWorld();
	}
	else
	{
		return nullptr;
	}
}

APawn* UWeaponInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

APawn* UWeaponInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	return nullptr;
}

void UWeaponInstance::SpawnEquipmentActors(const TArray<FWeaponActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		for (const FWeaponActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

			SpawnedActors.Add(NewActor);
		}
	}
}

void UWeaponInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

void UWeaponInstance::OnEquipped()
{
	K2_OnEquipped();
}

void UWeaponInstance::OnUnequipped()
{
	K2_OnUnequipped();
}
