// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "WeaponInstance.generated.h"


/**
 * 
 */
struct FWeaponActorToSpawn;

class UYogAnimInstance;
class USceneComponent;
class UYogGameplayAbility;
class UGameplayEffect;
class UAbilityData;


USTRUCT(BlueprintType)
struct FWeaponSocketLoc
{
public:
	GENERATED_USTRUCT_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* DmgBox_Start = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* DmgBox_End = nullptr;
};

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AWeaponInstance : public AActor
{
	GENERATED_BODY()
public:
	AWeaponInstance();

	virtual void BeginPlay()override;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FName AttachSocket;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	FTransform Relative_Transform;

	UPROPERTY(BlueprintReadWrite, SaveGame)
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	UPROPERTY()
	TObjectPtr<UAbilityData> WeaponAbilities;


	UFUNCTION(BlueprintCallable)
	void EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar);

};
