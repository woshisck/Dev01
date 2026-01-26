// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "WeaponInstance.h"
#include "DevKit/Character/YogCharacterBase.h"
#include "Player/PlayerCharacterBase.h"
#include <DevKit/YogBlueprintFunctionLibrary.h>


// Sets default values
AWeaponSpawner::AWeaponSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &AWeaponSpawner::OnOverlapBegin);

	//WeaponMesh
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	if (WeaponDefinition != nullptr)
	{
		WeaponMesh->SetStaticMesh(WeaponDefinition->DisplayMesh);

	}

	WeaponMeshRotationSpeed = 40.0f;
}

// Called when the game starts or when spawned
void AWeaponSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

// Called every frame
void AWeaponSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	WeaponMesh->AddRelativeRotation(FRotator(0.0f, World->GetDeltaSeconds() * WeaponMeshRotationSpeed, 0.0f));

}

void AWeaponSpawner::OnConstruction(const FTransform& Transform)
{	
	if (WeaponDefinition != nullptr && WeaponDefinition->DisplayMesh != nullptr)
	{
		WeaponMesh->SetStaticMesh(WeaponDefinition->DisplayMesh);
		WeaponMesh->SetRelativeLocation(WeaponDefinition->WeaponMeshOffset);
		WeaponMesh->SetRelativeScale3D(WeaponDefinition->WeaponMeshScale);
	}
}




void AWeaponSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("OnOverlapBegin Happens"));
	APlayerCharacterBase* OverlappingPawn = Cast<APlayerCharacterBase>(OtherActor);
	
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("PlayerState.HasWeapon"));

	if (OverlappingPawn->GetASC()->HasMatchingGameplayTag(Tag))
	{
		return;
	}

	if (OverlappingPawn != nullptr)
	{
		for (const FWeaponSpawnData& weapon_spawn_data : WeaponDefinition->ActorsToSpawn)
		{
			FWeaponSpawnData SpawnData;
			SpawnData.ActorToSpawn = weapon_spawn_data.ActorToSpawn;
			SpawnData.AttachSocket = weapon_spawn_data.AttachSocket;
			SpawnData.AttachTransform = weapon_spawn_data.AttachTransform;
			SpawnData.WeaponLayer = weapon_spawn_data.WeaponLayer;
			SpawnData.bShouldSaveToGame = true;

			AWeaponInstance* WeaponActor = SpawnWeaponDeferred(OverlappingPawn->GetWorld(), OverlappingPawn->GetActorTransform(), SpawnData);
			WeaponActor->AttachToComponent(OverlappingPawn->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponActor->AttachSocket);
			
			OverlappingPawn->GetMesh()->GetAnimInstance()->LinkAnimClassLayers(SpawnData.WeaponLayer);
		
		}
	}

	OverlappingPawn->GetASC()->AddLooseGameplayTag(Tag);
}

AWeaponInstance* AWeaponSpawner::SpawnWeaponDeferred(UWorld* World, const FTransform& SpawnTransform, const FWeaponSpawnData& SpawnData)
{

	AWeaponInstance* WeaponActor = GetWorld()->SpawnActorDeferred<AWeaponInstance>(
		SpawnData.ActorToSpawn,
		SpawnTransform,
		this,  // Owner
		nullptr,                // Instigator
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (WeaponActor)
	{
		ApplySpawnDataToWeapon(WeaponActor, SpawnData);
	}

	WeaponActor->FinishSpawning(SpawnTransform);
		
	return WeaponActor;
}

void AWeaponSpawner::ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data)
{
	Weapon->AttachSocket = Data.AttachSocket;
	Weapon->AttachTransform = Data.AttachTransform;
	Weapon->WeaponLayer = Data.WeaponLayer;
}



