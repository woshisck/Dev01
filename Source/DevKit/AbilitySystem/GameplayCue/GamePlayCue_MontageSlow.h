// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GamePlayCue_MontageSlow.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UGamePlayCue_MontageSlow : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
	
public:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;


private:
    UPROPERTY(EditDefaultsOnly, Category = "Montage")
    float SlowRate = 0.1f;

    UPROPERTY(EditDefaultsOnly, Category = "Montage")
    FName MontageRateParameterName = "MontageRateMultiplier";
};
