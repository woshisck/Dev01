// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawner.h"
#include "YogPickupDefinition.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"


// Sets default values
AItemSpawner::AItemSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &AItemSpawner::OnOverlapBegin);

	//PickupMesh
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(RootComponent);

	PickupMeshRotationSpeed = 0.0f;
}

// Called when the game starts or when spawned
void AItemSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

void AItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AItemSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	PickupMesh->AddRelativeRotation(FRotator(0.0f, World->GetDeltaSeconds() * PickupMeshRotationSpeed, 0.0f));

}

void AItemSpawner::OnConstruction(const FTransform& Transform)
{
	//TODO

	//if (WeaponPickUpDefinition != nullptr && WeaponPickUpDefinition->DisplayMesh != nullptr)
	//{
	//	PickupMesh->SetStaticMesh(WeaponPickUpDefinition->DisplayMesh);
	//	PickupMesh->SetRelativeLocation(WeaponPickUpDefinition->PickupMeshOffset);
	//	PickupMesh->SetRelativeScale3D(WeaponPickUpDefinition->PickupMeshScale);
	//}
}

void AItemSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("OnOverlapBegin Hppens"));
	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (OverlappingPawn != nullptr)
	{
		AttemptPickUpWeapon(OverlappingPawn);
	}
}

void AItemSpawner::CheckForExistingOverlaps()
{
}

void AItemSpawner::AttemptPickUpWeapon_Implementation(APawn* Pawn)
{
	UE_LOG(LogTemp, Warning, TEXT("AttemptPickUpWeapon_Implementation"));
	GiveGameplayEffect(Pawn);
	//if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn)) 
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("GetAbilitySystemComponent"));
	//	if (GiveGameplayEffect(Pawn))
	//	{
	//		//Weapon picked up by pawn
	//		SetWeaponPickupVisibility(false);
	//		PlayPickupEffects();
	//	}
	//}

}

void AItemSpawner::OnCoolDownTimerComplete()
{
}

void AItemSpawner::SetWeaponPickupVisibility(bool bShouldBeVisible)
{
}

void AItemSpawner::PlayPickupEffects_Implementation()
{
}

void AItemSpawner::PlayRespawnEffects_Implementation()
{
}
