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

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	//virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
