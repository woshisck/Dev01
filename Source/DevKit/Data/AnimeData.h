// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "AnimeData.generated.h"






UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UAnimeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	////UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (Categories = "Animation"))
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TMap<FGameplayTag, FMontageProperty> MontagesList;

};
