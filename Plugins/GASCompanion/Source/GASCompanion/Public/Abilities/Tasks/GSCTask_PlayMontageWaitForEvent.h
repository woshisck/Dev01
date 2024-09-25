// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimMontage.h"
#include "GSCTask_PlayMontageWaitForEvent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSCPlayMontageAndWaitForEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
* This task combines PlayMontageAndWait and WaitForEvent into one task, so you can wait for multiple
* types of activations such as from a melee combo
*
* Much of this code is copied from one of those two ability tasks
*
* This is a good task to look at as an example when creating game-specific tasks
*
* It is expected that each game will have a set of game-specific tasks to do what they want
*/
UCLASS()
class GASCOMPANION_API UGSCTask_PlayMontageWaitForEvent : public UAbilityTask
{
	GENERATED_BODY()

public:
	UGSCTask_PlayMontageWaitForEvent(const FObjectInitializer& ObjectInitializer);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool AbilityEnded) override;

	/** The montage completely finished playing */
	UPROPERTY(BlueprintAssignable)
	FGSCPlayMontageAndWaitForEventDelegate OnCompleted;

	/** The montage started blending out */
	UPROPERTY(BlueprintAssignable)
	FGSCPlayMontageAndWaitForEventDelegate OnBlendOut;

	/** The montage was interrupted */
	UPROPERTY(BlueprintAssignable)
	FGSCPlayMontageAndWaitForEventDelegate OnInterrupted;

	/** The ability task was explicitly cancelled by another ability */
	UPROPERTY(BlueprintAssignable)
	FGSCPlayMontageAndWaitForEventDelegate OnCancelled;

	/** One of the triggering gameplay events happened */
	UPROPERTY(BlueprintAssignable)
	FGSCPlayMontageAndWaitForEventDelegate EventReceived;

	/**
	* Unbinds all animation delegate on this Ability Task (except OnCanceled)
	*/
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
    void UnbindAllDelegate();

	/**
	* Play a montage and wait for it end. If a gameplay event happens that matches EventTags (or EventTags is empty),
	* the EventReceived delegate will fire with a tag and event data.
	*
	* If StopWhenAbilityEnds is true, this montage will be aborted if the ability ends normally. It is always stopped
	* when the ability is explicitly cancelled.
	*
	* On normal execution, OnBlendOut is called when the montage is blending out, and OnCompleted when it is completely done playing
	* OnInterrupted is called if another montage overwrites this, and OnCancelled is called if the ability or task is cancelled
	*
	* @param TaskInstanceName Set to override the name of this task, for later querying
	* @param MontageToPlay The montage to play on the character
	* @param EventTags Any gameplay events matching this tag will activate the EventReceived callback. If empty, all events will trigger callback
	* @param Rate Change to play the montage faster or slower
	* @param StartSection Change to montage section to play during montage
	* @param bStopWhenAbilityEnds If true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled
	* @param AnimRootMotionTranslationScale Change to modify size of root motion or set to 0 to block it entirely
	*/
	UFUNCTION(BlueprintCallable, Category= "Ability|GAS Companion|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
    static UGSCTask_PlayMontageWaitForEvent* PlayMontageAndWaitForEvent(
        UGameplayAbility* OwningAbility,
        FName TaskInstanceName,
        UAnimMontage* MontageToPlay,
        FGameplayTagContainer EventTags,
        float Rate = 1.f,
        FName StartSection = NAME_None,
        bool bStopWhenAbilityEnds = true,
        float AnimRootMotionTranslationScale = 1.f);

private:
	/** Montage that is playing */
	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** List of tags to match against gameplay events */
	UPROPERTY()
	FGameplayTagContainer EventTags;

	/** Playback rate */
	UPROPERTY()
	float Rate = 1.0f;

	/** Section to start montage from */
	UPROPERTY()
	FName StartSection;

	/** Modifies how root motion movement to apply */
	UPROPERTY()
	float AnimRootMotionTranslationScale;

	/** Rather montage should be aborted if ability ends */
	UPROPERTY()
	bool bStopWhenAbilityEnds = true;

	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage() const;

	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted) const;
	void OnAbilityCancelled() const;
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) const;
	void OnServerSyncEventReceived(FGameplayTag EventTag, FGameplayEventData EventData) const;

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle CancelledHandle;
	FDelegateHandle EventHandle;
};
