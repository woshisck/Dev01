// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include <NiagaraSystem.h>
#include "AbilityData.h"
#include "WeaponData.generated.h"

class AWeaponInstance;
class UYogGameplayAbility;
class AYogCharacterBase;



USTRUCT(BlueprintType)
struct FWeaponAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FWeaponAttributeData() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtk = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtkPower = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtkRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weapon_CritRate = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weapon_CritDmg = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AWeaponInstance> WeaponInstance;

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


	UFUNCTION(BlueprintCallable)
	void GrantAbilityToOwner(AYogCharacterBase* Owner);

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
