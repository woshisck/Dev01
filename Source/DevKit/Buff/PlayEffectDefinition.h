// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DevKit/AbilitySystem/GameplayEffect/YogGameplayEffect.h"

#include "PlayEffectDefinition.generated.h"



/**
 * 
 */
USTRUCT(Blueprintable)
struct FYogPlayEffect
{
	GENERATED_BODY()

	FYogPlayEffect() {};

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Image;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int level;
};



UCLASS(Blueprintable)
class DEVKIT_API UPlayEffectDefinition : public UObject
{
	GENERATED_BODY()

public:

	UPlayEffectDefinition();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Image;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int level;


	
};
