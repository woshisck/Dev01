// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "YogTask_PlayMontageAbility.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayMontageSimpleDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayMontageAbilityDelegate, FGameplayTag, EventTag, const FGameplayEventData&, EventData);


class UYogAbilitySystemComponent;

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UYogTask_PlayMontageAbility : public UAbilityTask
{
	GENERATED_BODY()

public:
	UYogTask_PlayMontageAbility(const FObjectInitializer& ObjectInitializer);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool AbilityEnded) override;

	/** The montage completely finished playing */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageSimpleDelegate OnCompleted;

	/** The montage started blending out */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageSimpleDelegate OnBlendOut;

	/** The montage was interrupted */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageSimpleDelegate OnInterrupted;

	/** The ability task was explicitly cancelled by another ability */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageSimpleDelegate OnCancelled;

	/** One of the triggering gameplay events happened */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageAbilityDelegate OnEventReceived;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",DisplayName = "Play Montage Ability Task", BlueprintInternalUseOnly = "TRUE"))
	static UYogTask_PlayMontageAbility* YogPlayMontageAbility(
		UGameplayAbility * OwningAbility,
		FName TaskInstanceName,
		UAnimMontage * MontageToPlay,
		FGameplayTagContainer EventTags,
		float Rate = 1.f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.f);

private:
	/** Montage that is playing */
	UPROPERTY()
	UAnimMontage* MontageToPlay;

	/** List of tags to match against gameplay events */
	UPROPERTY()
	FGameplayTagContainer EventTags;

	/** Playback rate */
	UPROPERTY()
	float Rate;

	/** Section to start montage from */
	UPROPERTY()
	FName StartSection;

	/** Modifies how root motion movement to apply */
	UPROPERTY()
	float AnimRootMotionTranslationScale;

	/** Rather montage should be aborted if ability ends */
	UPROPERTY()
	bool bStopWhenAbilityEnds;


	UFUNCTION()
	bool StopPlayingMontage();

	
	/** Returns our ability system component */
	UFUNCTION()
	UYogAbilitySystemComponent* GetTargetASC();

	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnAbilityCancelled();
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle CancelledHandle;
	FDelegateHandle EventHandle;

};