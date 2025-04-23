// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include <DevKit/Character/YogCharacterBase.h>


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

//void AWeaponSpawner::GrantWeapon(AYogCharacterBase* ReceivingChar)
//{
//
//	UE_LOG(LogTemp, Warning, TEXT("AttemptPickUpWeapon_Implementaion running, ReceivingChar: %s"), *ReceivingChar->GetName());
//
//	if (ReceivingChar->bWeaponEquiped == false)
//	{
//		//spawn && attach weapon
//		USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
//		for (FWeaponActorToSpawn& WeaponActorInst : WeaponDefinition->ActorsToSpawn)
//		{
//			TSubclassOf<AActor> WeaponActorClass = WeaponActorInst.ActorToSpawn;
//			FName Socket = WeaponActorInst.AttachSocket;
//			FTransform Transform = WeaponActorInst.AttachTransform;
//
//			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(WeaponActorClass, FTransform::Identity, ReceivingChar);
//			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
//			NewActor->SetActorRelativeTransform(Transform);
//			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
//
//		}
//		for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
//		{
//			for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
//			{
//				ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
//			}
//		}
//		ReceivingChar->bWeaponEquiped = true;
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("AttemptPickUpWeapon_Implementaion running, YogCharacterBase"));
//}

void AWeaponSpawner::GiveWeaponToCharacter(AYogCharacterBase* ReceivingChar)
{
	USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
	for (FWeaponActorToSpawn& WeaponActorInst : WeaponDefinition->ActorsToSpawn)
	{
		TSubclassOf<AActor> WeaponActorClass = WeaponActorInst.ActorToSpawn;
		FName Socket = WeaponActorInst.AttachSocket;
		FTransform Transform = WeaponActorInst.AttachTransform;

		AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(WeaponActorClass, FTransform::Identity, ReceivingChar);
		NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		NewActor->SetActorRelativeTransform(Transform);
		NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);

	}
	for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
	{
		for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
		{
			ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
		}
	}

}

void AWeaponSpawner::SpawnAttachWeapon(AYogCharacterBase* ReceivingChar)
{
	USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
	for (FWeaponActorToSpawn& WeaponActorInst : WeaponDefinition->ActorsToSpawn)
	{
		TSubclassOf<AActor> WeaponActorClass = WeaponActorInst.ActorToSpawn;
		FName Socket = WeaponActorInst.AttachSocket;
		FTransform Transform = WeaponActorInst.AttachTransform;

		AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(WeaponActorClass, FTransform::Identity, ReceivingChar);
		NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		NewActor->SetActorRelativeTransform(Transform);
		NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);

	}
}

void AWeaponSpawner::GrantWeaponAbility(AYogCharacterBase* ReceivingChar)
{
	for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
	{
		for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
		{
			ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
		}
	}
}

void AWeaponSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("OnOverlapBegin Happens"));
	AYogCharacterBase* OverlappingPawn = Cast<AYogCharacterBase>(OtherActor);

	if (OverlappingPawn != nullptr)
	{
		GrantWeapon(OverlappingPawn);
	}
}



