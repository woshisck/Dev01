// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;


USTRUCT(BlueprintType)
struct FHitBoxData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHitBoxData()
		: Point(FVector(0, 0, 0)), HasTriggered(false), Index(0), FrameAt(0)
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

USTRUCT(BlueprintType)
struct FYogAbilityData : public FTableRowBase
{
    GENERATED_BODY()

public:
	FYogAbilityData(){}
    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActDamage = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActRange = 400;

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
	TSubclassOf<UYogGameplayAbility> Ability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "HitBoxData"))
	TArray<FHitBoxData> hitbox;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "HitBoxData"))
	//FDataTableRowHandle hitbox;

};



USTRUCT(BlueprintType)
struct FActionData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FActionData(){};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "HitBoxData"))
	FDataTableRowHandle hitbox;


};


UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYogAbilityData> Abilities;


};
