// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "AbilityData.generated.h"


class UYogGameplayAbility;


UENUM(BlueprintType)
enum class EYogEffectTarget : uint8
{
	ToSelf,
	ToTarget
};


UENUM(BlueprintType)
enum class EHitBoxType : uint8
{
	Annulus,
	Triangle,
	Square,
	Circle
};

USTRUCT(Blueprintable)
struct FHitboxAnnulus
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float inner_radius = 0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float outer_radius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float degree = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float offset_degree = 0;

};

USTRUCT(Blueprintable)
struct FHitboxTriangle
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Degree = 0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Radius = 0;
};


USTRUCT(Blueprintable)
struct FHitboxSquare
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 0;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Length = 0;
};


USTRUCT(Blueprintable)
struct FHitboxCircle
{
	GENERATED_BODY()

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Radius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Offset = FVector(0,0,0);
};

USTRUCT(Blueprintable)
struct FYogHitboxType
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitBoxType hitboxType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus", EditConditionHides))
	FHitboxAnnulus AnnulusHitbox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Circle", EditConditionHides))
	TArray<FHitboxCircle> HitboxCircles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Square", EditConditionHides))
	TArray<FHitboxSquare> HitboxSquares;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle", EditConditionHides))
	TArray<FHitboxTriangle> HitboxTriangles;



};



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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EYogEffectTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveTable> PowerCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
	//FVector Point = FVector(Radius * cos(Degree), Radius * sin(Degree), 0.0);
	FVector Point = FVector(0,0,0);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FVector Location_Start;


	/*
	* 	Annulus,
		Triangle,
		Square,
		Circle
	*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus"))
	float inner_radius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus"))
	float outer_radius = 0;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle"))
	float Degree = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle"))
	float Radius = 0;


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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
	TArray<FYogHitboxType> hitboxTypes;




	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Annulus", EditConditionHides))
	//FHitboxAnnulus AnnulusHitbox;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Circle", EditConditionHides))
	//TArray<FHitboxCircle> HitboxCircles;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = ( EditCondition = "hitboxType == EHitBoxType::Square", EditConditionHides))
	//TArray<FHitboxSquare> HitboxSquares;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox", meta = (EditCondition = "hitboxType == EHitBoxType::Triangle", EditConditionHides))
	//TArray<FHitboxTriangle> HitboxTriangles;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<FHitBoxData> hitbox;

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