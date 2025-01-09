// Fill out your copyright notice in the Description page of Project Settings.


#include "YogEquipmentInstance.h"
#include "YogEquipmentDefinition.h"
#include "GameFramework/Character.h"



UYogEquipmentInstance::UYogEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UYogEquipmentInstance::GetWorld() const
{
	return nullptr;
}

APawn* UYogEquipmentInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

APawn* UYogEquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType) {
		if (GetOuter()->IsA(ActualPawnType)) {
			Result = Cast<APawn>(GetOuter());
		}
	}

	return Result;
}

void UYogEquipmentInstance::SpawnEquipmentActors(const TArray<FYogEquipmentActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		for (const FYogEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

			SpawnedActors.Add(NewActor);
		}
	}
}

void UYogEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* actor : SpawnedActors) {
		if(actor) {
			actor->Destroy();
		}
	}
}

void UYogEquipmentInstance::OnEquipped(FTransform& SpawnLoc)
{
	K2_OnEquipped(SpawnLoc);
}

void UYogEquipmentInstance::OnUnequipped()
{
	K2_OnUnequipped();
}

