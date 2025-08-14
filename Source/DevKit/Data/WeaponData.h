// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include <NiagaraSystem.h>
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
	float AttackSpeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrticalRate = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalDamage = 1;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<UYogGameplayAbility*> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UAnimInstance*> WeaponAnimLayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UDataTable> Actions;

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

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "WeaponAttributeData"))
	FDataTableRowHandle MoveDataRow;




	inline static const FWeaponAttributeData DefaultWPNData;

};
