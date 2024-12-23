// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"


#include "YogSchema.generated.h"



class UGameFeatureAction;

/**
 * 
 */

UCLASS(BlueprintType, Const)
class UYogSchema : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYogSchema();


	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;


};
