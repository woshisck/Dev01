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

protected:
	virtual void HandleMontageEnded(bool bWasCancelled);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Combo")
	bool bListenForComboWindow = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold")
	bool bHoldMontageUntilInputRelease = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FGameplayTag HoldReleaseEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FName HoldStartSection = TEXT("BlockStart");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FName HoldLoopSection = TEXT("BlockIdle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FName HoldEndSection = TEXT("BlockEnd");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	bool bJumpToHoldEndSectionOnRelease = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Block", meta = (EditCondition = "bHoldMontageUntilInputRelease", ClampMin = "0.0"))
	float JustBlockRewardDuration = 3.0f;

private:
	void OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount);
	bool IsHoldMontageConfigured(const UAnimMontage* Montage) const;
	bool HasMontageSection(const UAnimMontage* Montage, FName SectionName) const;
	void ConfigureHoldMontageSections();
	void HandleHoldInputReleased();
	void ClearBlockStateTags();
	void SetBlockStateTag(const FGameplayTag& Tag, int32 Count);
	void ScheduleBlockStartWindow();
	void FinishBlockStartWindow();
	void ApplyJustBlockReward();

	FDelegateHandle CanComboTagHandle;
	float AbilityActivationTime = 0.0f;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveMontage;

	FTimerHandle ComboWindowOpenHandle;
	FTimerHandle ComboWindowCloseHandle;
	FTimerHandle BlockStartWindowHandle;

	bool bIsHandlingMeleeEvent = false;
	bool bActiveHoldMontage = false;
	bool bHoldReleaseReceived = false;
};
