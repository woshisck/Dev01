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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FTransform Relative_Transform;



	UFUNCTION(BlueprintCallable)
	void EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar);

};
