// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "GA_PlayMontage.generated.h"

class UAN_MeleeDamage;
class UAnimMontage;
class UYogTask_PlayMontageAbility;

UCLASS()
class DEVKIT_API UGA_PlayMontage : public UYogGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UYogGameplayEffect> DynamicEffectClass;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	UPROPERTY()
	TObjectPtr<UYogTask_PlayMontageAbility> ActivePlayMontageTask;

private:
	void OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount);

	FDelegateHandle CanComboTagHandle;
	float AbilityActivationTime = 0.0f;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveMontage;

	FTimerHandle ComboWindowOpenHandle;
	FTimerHandle ComboWindowCloseHandle;

	bool bIsHandlingMeleeEvent = false;
};
