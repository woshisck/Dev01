// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogGameplayEffect.h"
#include "../Abilities/YogGameplayAbility.h"
#include "PassiveEffect.generated.h"

/**
 * 
 */

class UYogGameplayAbility;

UCLASS()
class DEVKIT_API UPassiveEffect : public UYogGameplayEffect
{
	GENERATED_BODY()
	
public:


	//UPassiveEffect() 
	//{
	//	FGameplayAbilitySpec AbilitySpec(UYogGameplayAbility::StaticClass());
	//	GrantedAbilities.Add(AbilitySpec);
	//}

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//UYogGameplayAbility GrantAbility;
};
