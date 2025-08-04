// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include <NiagaraSystem.h>
#include "WeaponData.generated.h"


class UYogGameplayAbility;

USTRUCT(BlueprintType)
struct FHitBoxData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHitBoxData()
		: normalVector(FVector(0, 0, 1)), Location_End(FVector(0, 0, 0)), Location_Start(FVector(0, 0, 0)), HasTriggered(false), Index(0)
	{
	}
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector normalVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location_End;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasTriggered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Index;
};


USTRUCT(BlueprintType)
struct FWeaponAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FWeaponAttributeData()
		: AttackPower(0), AttackSpeed(0), AttackRange(0), CrticalRate(0), CriticalDamage(0)
	{
	};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CrticalRate = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UYogGameplayAbility*> Actions;

};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//Get the item price info
	UFUNCTION(BlueprintPure)
	const FWeaponAttributeData& GetWeaponData() const;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "MovementData"))
	FDataTableRowHandle MoveDataRow;

	inline static const FWeaponAttributeData DefaultWPNData;

};
