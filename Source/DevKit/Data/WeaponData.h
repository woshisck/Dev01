// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include <NiagaraSystem.h>
#include "AbilityData.h"
#include "WeaponData.generated.h"


class UYogGameplayAbility;




USTRUCT(BlueprintType)
struct FWeaponAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FWeaponAttributeData() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtk = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAttackSpeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrticalRate = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalDamage = 1;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<UYogGameplayAbility*> Actions;

};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//Get the item price info
	UFUNCTION(BlueprintPure)
	const FWeaponAttributeData& GetWeaponData() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "WeaponAttributeData"))
	FDataTableRowHandle WeaponAttributeRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UAbilityData>> Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UAnimInstance>> WeaponAmineLayer;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<TObjectPtr<UAnimMontage>> IncludedMontage;

	inline static const FWeaponAttributeData DefaultWPNData;

};
