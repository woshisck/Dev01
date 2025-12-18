// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DevKit/Data/AbilityData.h"
#include "GameplayEffect.h"
#include "CardData.generated.h"



UENUM(BlueprintType)
enum class ERarity : uint8
{
    Common     UMETA(DisplayName = "Common"),
    Uncommon   UMETA(DisplayName = "Uncommon"),
    Rare       UMETA(DisplayName = "Rare"),
    Epic       UMETA(DisplayName = "Epic"),
    Legendary  UMETA(DisplayName = "Legendary")
};

UENUM(Blueprintable)
enum class CardEffectTarget : uint8
{
	ToSelf UMETA(DisplayName = "ToSelf"),
	ToTarget UMETA(DisplayName = "ToTarget")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCardProperty
{
	GENERATED_BODY()

public:
	FCardProperty() {}
	// Transform in pivot space (*not* texture space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYogApplyEffect> CardEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EYogEffectTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow))
    FGameplayTag DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Rare = 1;

};


UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UCardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
	TArray<FCardProperty> CardProperties;

    UFUNCTION()
    FCardProperty GetCardPropertyWithRareRandom(int target_rare);
	
};
