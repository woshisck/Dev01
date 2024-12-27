// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YogMapDefinition.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UYogMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Asset")
	TObjectPtr<UWorld> Level;

};
