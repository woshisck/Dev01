// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../AbilitySystem/YogAbilitySystemComponent.h"
#include "../Weapon/WeaponDefinition.h"

#include "WeaponSpawner.generated.h"


class APawn;
class UCapsuleComponent;
class AYogCharacterBase;

class UObject;
class UPrimitiveComponent;
class UStaticMeshComponent;

class UWeaponDefinition;

struct FFrame;
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

protected:

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Yog|ItemPickup")
	TObjectPtr<UWeaponDefinition> WeaponDefinition;

	//Delay between when the weapon is made available and when we check for a pawn standing in the spawner. Used to give the bIsWeaponAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	float CheckExistingOverlapDelay;

public:

	UFUNCTION(BlueprintImplementableEvent)
	void GrantWeapon(AYogCharacterBase* ReceivingChar);

	//UFUNCTION(BlueprintCallable)
	//void GiveWeaponToCharacter(AYogCharacterBase* ReceivingChar);


	UFUNCTION(BlueprintCallable)
	void SpawnAttachWeapon(AYogCharacterBase* ReceivingChar);

	UFUNCTION(BlueprintCallable)
	void GrantWeaponAbility(AYogCharacterBase* ReceivingChar);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;


	UPROPERTY(BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ItemPickup")
	float WeaponMeshRotationSpeed;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);


};
