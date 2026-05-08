// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "GA_PlayMontage.generated.h"

class UYogTask_PlayMontageAbility;
class UMontageAttackDataAsset;

/**
 *
 */
UCLASS()
class DEVKIT_API UGA_PlayMontage : public UYogGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	bool HasConfiguredAttackData() const { return ActiveComboAttackData != nullptr; }
	const UMontageAttackDataAsset* GetConfiguredAttackData() const { return ActiveComboAttackData; }


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

	UPROPERTY()
	TObjectPtr<UMontageAttackDataAsset> ActiveComboAttackData;

private:
	// CanCombo tag 变化时的回调，检查输入缓存并触发连击
	void OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Node-driven combo window: open/close CanCombo tag at frame-defined times
	void OnComboWindowOpen();
	void OnComboWindowClose();

	// Wipe ComboRuntimeComponent state back to root. Only called from montage-end callbacks,
	// never from EndAbility, so it cannot collide with bRetriggerInstancedAbility flow.
	void ResetComboToRoot();

	FDelegateHandle CanComboTagHandle;

	// ActivateAbility 时记录的世界时间，OnCanComboTagChanged 只接受此时间之后的输入
	float AbilityActivationTime = 0.0f;

	// Timers for node-driven combo window (bOverrideComboWindow = true)
	FTimerHandle ComboWindowOpenHandle;
	FTimerHandle ComboWindowCloseHandle;
};
