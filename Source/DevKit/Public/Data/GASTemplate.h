// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GASTemplate.generated.h"

/**
 * 
 */
class UYogGameplayAbility;

UCLASS()
class DEVKIT_API UGASTemplate : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow), Category = "Abiltiy|Action")
	TArray<TSubclassOf<UYogGameplayAbility>> AbilityMap;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow), Category = "Abiltiy|Passive")
	TArray<TSubclassOf<UYogGameplayAbility>> PassiveMap;
	//TMap<FYogTagContainerWrapper, FActionData> AbilityMap;

};
