// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"


#include "YogGameRule.generated.h"



class UGameFeatureAction;

/**
 * 
 */

UCLASS(BlueprintType, Const)
class UYogGameRule : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYogGameRule();


	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif


};
