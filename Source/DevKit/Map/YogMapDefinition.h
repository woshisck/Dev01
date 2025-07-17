// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../System/YogWorldSubsystem.h"
#include "YogMapDefinition.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UYogMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	TArray<FLevel2DRow> LevelDefinition;

};
