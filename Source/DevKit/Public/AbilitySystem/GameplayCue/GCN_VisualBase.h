// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_VisualBase.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AGCN_VisualBase : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameplayCue")
	static AActor* GetInstigatorFromGameplayCueParameters(const FGameplayCueParameters& Parameters);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameplayCue")
	static FVector GetOriginFromGameplayCueParameters(const FGameplayCueParameters& Parameters);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameplayCue")
	static AActor* GetSourceObjectFromGameplayCueParameters(const FGameplayCueParameters& Parameters);
};
