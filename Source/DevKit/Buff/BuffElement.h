// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DevKit/AbilitySystem/GameplayEffect/YogGameplayEffect.h"

#include "BuffElement.generated.h"



/**
 * 
 */

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBuffElement : public UObject
{
	GENERATED_BODY()

public:

	UBuffElement();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Image;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int level;


	
};
