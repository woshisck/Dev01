// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSpawner.h"
#include "../Equipment/YogPickupDefinition.h"


#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"


// Sets default values
AWeaponSpawner::AWeaponSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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


}

void AWeaponSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
}

void AWeaponSpawner::CheckForExistingOverlaps()
{
}

void AWeaponSpawner::AttemptPickUpWeapon_Implementation(APawn* Pawn)
{
}

void AWeaponSpawner::OnCoolDownTimerComplete()
{
}

void AWeaponSpawner::SetWeaponPickupVisibility(bool bShouldBeVisible)
{
}

void AWeaponSpawner::PlayPickupEffects_Implementation()
{
}

void AWeaponSpawner::PlayRespawnEffects_Implementation()
{
}
