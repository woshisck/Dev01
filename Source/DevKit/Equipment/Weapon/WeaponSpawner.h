// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <Devkit/Equipment/Weapon/WeaponDefinition.h>
#include <Devkit/Item/YogPickupDefinition.h>


#include "WeaponSpawner.generated.h"


class APawn;
class UCapsuleComponent;


class UObject;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FFrame;
struct FGameplayTag;
struct FHitResult;



UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AWeaponSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void OnConstruction(const FTransform& Transform) override;

protected:

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Yog|WeaponPickup")
	TObjectPtr<UWeaponDefinition> WeaponDefinition;

	//Delay between when the weapon is made available and when we check for a pawn standing in the spawner. Used to give the bIsWeaponAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponPickup")
	float CheckExistingOverlapDelay;


public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;


	UPROPERTY(BlueprintReadOnly, Category = "WeaponPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponPickup")
	float WeaponMeshRotationSpeed;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	//Check for pawns standing on pad when the weapon is spawned. 
	void CheckForExistingOverlaps();

	UFUNCTION(BlueprintNativeEvent)
	void AttemptPickUpWeapon(APawn* Pawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponPickup")
	bool GiveWeapon(APawn* ReceivingPawn);


	UFUNCTION()
	void OnCoolDownTimerComplete();

	void SetWeaponPickupVisibility(bool bShouldBeVisible);

	UFUNCTION(BlueprintNativeEvent, Category = "WeaponPickup")
	void PlayPickupEffects();

	UFUNCTION(BlueprintNativeEvent, Category = "WeaponPickup")
	void PlayRespawnEffects();

};
