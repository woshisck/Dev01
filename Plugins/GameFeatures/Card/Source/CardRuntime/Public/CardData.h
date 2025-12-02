// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "CardData.generated.h"

/**
 * 
 */
UENUM(Blueprintable)
enum class CardEffectTarget : uint8
{
	ToSelf UMETA(DisplayName = "ToSelf"),
	ToTarget UMETA(DisplayName = "ToTarget")
};

USTRUCT(BlueprintType)
struct CARDRUNTIME_API FCardEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> CardGamplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveTable> PowerCurve;
};


USTRUCT(BlueprintType)
struct CARDRUNTIME_API FCardPropertyConfig : public FTableRowBase
{
	GENERATED_BODY()

	// Transform in pivot space (*not* texture space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardEffect> CardEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	CardEffectTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Rare;
};


UCLASS(Blueprintable, BlueprintType)
class CARDRUNTIME_API UCardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<TObjectPtr<UYogGameplayEffect>> CardEffects;
	
};
