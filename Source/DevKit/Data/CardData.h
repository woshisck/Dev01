// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardData.generated.h"

/**
 * 
 */
class UYogGameplayEffect;

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UCardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UYogGameplayEffect>> CardEffects;
	
};
