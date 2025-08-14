// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;


USTRUCT(BlueprintType)
struct FYogAbilityData : public FTableRowBase
{
    GENERATED_BODY()

public:
	FYogAbilityData(){}
    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AtkRange = 400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActResilience = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActDmgReduce = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActRotateSpeed = 360;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpFrameTime = 0.15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FreezeFrameTime = 0.15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UYogGameplayAbility> Ability;

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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "YogAbilityData"))
	FDataTableRowHandle AbilityDataRow;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "HitBoxData"))
	TArray<FHitBoxData> array_Hitbox;



	inline static const FYogAbilityData DefaultAbilityData;


};
