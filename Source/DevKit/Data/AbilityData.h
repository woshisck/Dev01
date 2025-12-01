// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;


USTRUCT(Blueprintable)
struct FYogTagContainerWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer TagContainer;

	// Equality operator
	bool operator==(const FYogTagContainerWrapper& Other) const
	{
		// This uses the FGameplayTagContainer's own == operator, which checks for exact matching tags.
		return TagContainer == Other.TagContainer;
	}

	// Hash function
	friend uint32 GetTypeHash(const FYogTagContainerWrapper& Wrapper)
	{
		// A simple but potentially inefficient hash: combine hashes of all individual tags.
		uint32 Hash = 0;
		for (const FGameplayTag& Tag : Wrapper.TagContainer)
		{
			Hash = HashCombine(Hash, GetTypeHash(Tag));
		}
		return Hash;
	}
};



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



//USTRUCT(BlueprintType)
//struct FAbilityType
//{
//	GENERATED_BODY()
//
//public:
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "ActionData"))
//	FDataTableRowHandle ActionRow;
//
//	const FActionData& GetAction() const;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	TSubclassOf<UYogGameplayAbility> ability;
//
//	inline static const FActionData DefaultActionData;
//};


USTRUCT(BlueprintType)
struct DEVKIT_API FCharacterAnimationsConfig : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (Categories = "Animation"))
	TMap<FGameplayTag, UAnimMontage*> MontagesList;
};



UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, meta = (ForceInlineRow))
	TMap<FGameplayTag, FActionData> AbilityMap;
	//TMap<FYogTagContainerWrapper, FActionData> AbilityMap;


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FActionData GetAbility(const FGameplayTag& Key) const
	{
		return AbilityMap.FindRef(Key);
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool HasAbility(const FGameplayTag& Key) const
	{
		return AbilityMap.Contains(Key);
	}


};