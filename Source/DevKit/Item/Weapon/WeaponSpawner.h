// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../AbilitySystem/YogAbilitySystemComponent.h"
#include "DevKit/Item/Weapon/WeaponDefinition.h"

#include "WeaponSpawner.generated.h"


class APawn;
class UCapsuleComponent;
class AYogCharacterBase;
class APlayerCharacterBase;

class UObject;
class UPrimitiveComponent;
class UStaticMeshComponent;

class UWeaponDefinition;


struct FGameplayTag;
struct FHitResult;




UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AWeaponSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponSpawner(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Definition")
	TObjectPtr<UWeaponDefinition> WeaponDefinition;

	//Delay between when the weapon is made available and when we check for a pawn standing in the spawner. Used to give the bIsWeaponAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Definition")
	float CheckExistingOverlapDelay;

	UFUNCTION(BlueprintImplementableEvent)
	void GrantWeapon(APlayerCharacterBase* ReceivingChar);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;


	UPROPERTY(BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ItemPickup")
	float WeaponMeshRotationSpeed;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	TArray<AWeaponInstance*>SpawnWeaponDeferred(UWorld* World, const FTransform& SpawnTransform, const FWeaponSpawnData& SpawnData);

private:

	// Helper function to apply spawn data to weapon
	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);
};
