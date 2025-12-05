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
	TSubclassOf<UGameplayEffect> CardGameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveTable> PowerCurve;

    FString ToString(bool bDetailed = false) const
    {
        FString GameplayEffectName = CardGameplayEffect ?
            CardGameplayEffect->GetName() : TEXT("None");
        FString CurveTableName = PowerCurve ?
            PowerCurve->GetName() : TEXT("None");

        return FString::Printf(
            TEXT("FCardEffect(GE: %s, Curve: %s)"),
            *GameplayEffectName,
            *CurveTableName);
    }
};


USTRUCT(BlueprintType)
struct CARDRUNTIME_API FCardProperty
{
	GENERATED_BODY()

public:
	FCardProperty() {}
	// Transform in pivot space (*not* texture space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardEffect> CardEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	CardEffectTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow))
    FGameplayTag DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Rare;

    FString CardPropertyToString(const FCardProperty& CardProp)
    {
        FString Output;

        // Start with header
        Output += TEXT("Card Property:\n");

        // Texture
        if (CardProp.CardTexture)
        {
            Output += FString::Printf(TEXT("  Texture: %s  "),
                *CardProp.CardTexture->GetName());
        }
        else
        {
            Output += TEXT("  Texture: None  ");
        }

        // Display name
        Output += FString::Printf(TEXT("  Name: %s  "),
            *CardProp.DisplayName.ToString());

        // Rarity
        Output += FString::Printf(TEXT("  Rarity: %d  "), CardProp.Rare);

        // Target
        Output += FString::Printf(TEXT("  Target: %s  "),
            *UEnum::GetDisplayValueAsText(CardProp.Target).ToString());

        // Effects
        Output += FString::Printf(TEXT("  Effects (%d):  "),
            CardProp.CardEffect.Num());

        for (int32 i = 0; i < CardProp.CardEffect.Num(); i++)
        {
            Output += FString::Printf(TEXT("    [%d] %s  "),
                i,
                *CardProp.CardEffect[i].ToString());
        }

        return Output;
    }

};


UCLASS(Blueprintable, BlueprintType)
class CARDRUNTIME_API UCardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
	TArray<FCardProperty> CardProperties;

    UFUNCTION()
    FCardProperty GetCardPropertyWithRareRandom(int target_rare);
	
};
