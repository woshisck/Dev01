// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogGameplayAbility.h"
#include "Passive_YogAbility.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UPassive_YogAbility : public UYogGameplayAbility
{
	GENERATED_BODY()

public:


	UPassive_YogAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

};
