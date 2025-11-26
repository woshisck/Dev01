// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "AbilityData.h"
#include "LevelData.generated.h"



UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API ULevelData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UWorld>> WorldList;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<TObjectPtr<UAnimMontage>> IncludedMontage;


};
