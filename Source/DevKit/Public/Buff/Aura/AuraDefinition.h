// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AuraDefinition.generated.h"

/**
 * 
 */
class AAuraBase;

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UAuraDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura")
	TSubclassOf<AAuraBase> AuraClass;


};
