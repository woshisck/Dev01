// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;


USTRUCT(BlueprintType)
struct FUniqueEffect 
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int level;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> GameplayEffect;
};


USTRUCT(BlueprintType)
struct FHitBoxData 
{
	GENERATED_BODY()

public:
	//FHitBoxData()
	//	: Point(FVector(0, 0, 0)), HasTriggered(false), Index(0), FrameAt(0)
	//{
	//}


	UPROPERTY(BlueprintReadOnly)
	FVector Point = FVector(Radius * cos(Degree), Radius * sin(Degree), 0.0);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FVector Location_Start;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool HasTriggered;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Degree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Radius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int Index;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int FrameAt;

};

USTRUCT(BlueprintType)
struct FActionData
{
    GENERATED_BODY()

public:
	FActionData(){}
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
	TArray<FUniqueEffect> UniqueEffects;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<UYogGameplayAbility> Ability_Template;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHitBoxData> hitbox;


};



USTRUCT(BlueprintType)
struct FAbilityType
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "ActionData"))
	FDataTableRowHandle ActionRow;

	const FActionData& GetAction() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayAbility> ability;

	inline static const FActionData DefaultActionData;
};


UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FActionData> AbilityMap;

};