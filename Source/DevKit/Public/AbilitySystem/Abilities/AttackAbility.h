// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Templates/SharedPointer.h" 
#include "AttackAbility.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UAttackAbility : public UYogGameplayAbility
{
	GENERATED_BODY()
	
public:
	UAttackAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


	TSharedPtr<FActionData> cache_action_data;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;
};
