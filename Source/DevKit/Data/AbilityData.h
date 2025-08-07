// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WeaponData.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;


USTRUCT(BlueprintType)
struct FYogAbilityData : public FTableRowBase
{
    GENERATED_BODY()

public:
	FYogAbilityData()
		: Damage(0.0f), DMGAmplify(0.0f), MontagePlayRate(0.0f), AbilityPower(1.0f)
	{
	}
    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DMGAmplify;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MontagePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityPower;
};



USTRUCT(BlueprintType)
struct FHitBoxData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHitBoxData()
		: Point(FVector(0,0,0)), HasTriggered(false), Index(0), FrameAt(0)
	{
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Point;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FVector Location_Start;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool HasTriggered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Index;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int FrameAt;
};


UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//Get the item price info
	UFUNCTION(BlueprintPure)
	const FYogAbilityData& GetAbilityData() const;


	UPROPERTY(EditDefaultsOnly, meta = (RowType = "YogAbilityData"))
	FDataTableRowHandle AbilityDataRow;


	UPROPERTY(EditDefaultsOnly, meta = (RowType = "HitBoxData"))
	TArray<FHitBoxData> array_Hitbox;


	inline static const FYogAbilityData DefaultAbilityData;


};
