// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphRuntimeTypes.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimMontage.h"
#include "ComboGraphAbilityTask_PlayMontage.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComboGraphPlayMontageEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

DECLARE_MULTICAST_DELEGATE(FComboGraphMontageOnTriggerTransition);
DECLARE_MULTICAST_DELEGATE_OneParam(FComboGraphMontageOnSectionChanged, FName SectionName);

USTRUCT()
struct FComboGraphMontageNotifyAutoSetupData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimNotifyState> CachedNotify = nullptr;

	bool bBeginNotified = false;
	bool bEndNotified = false;

	FComboGraphNotifyStateAutoSetup AutoSetup;
};

/** Holds information about a registered Anim Notify State with state whether begin / end has been triggered */
USTRUCT()
struct FComboGraphRegisteredNotifyData
{
	GENERATED_BODY()

	bool bBeginNotified = false;
	bool bEndNotified = false;

	FComboGraphNotifyStateAutoSetup AutoSetup;

	void ResetState()
	{
		bBeginNotified = false;
		bEndNotified = false;
	}
};

/**
* This task is a variation of PlayMontageAndWaitForEvent that combines PlayMontageAndWait and WaitForEvent into one task.
*
* The added functionality here is that it is a ticking task to optionally trigger AnimNotifyStates Begin and End event, without having
* them actually registered in the Animation Asset (Sequence or Montage)
*/
UCLASS()
class COMBOGRAPH_API UComboGraphAbilityTask_PlayMontage : public UAbilityTask
{
	GENERATED_BODY()

public:
	UComboGraphAbilityTask_PlayMontage(const FObjectInitializer& ObjectInitializer);

	/** The montage completely finished playing */
	UPROPERTY(BlueprintAssignable)
	FComboGraphPlayMontageEventDelegate OnCompleted;

	/** The montage started blending out */
	UPROPERTY(BlueprintAssignable)
	FComboGraphPlayMontageEventDelegate OnBlendOut;

	/** The montage was interrupted */
	UPROPERTY(BlueprintAssignable)
	FComboGraphPlayMontageEventDelegate OnInterrupted;

	/** The ability task was explicitly cancelled by another ability */
	UPROPERTY(BlueprintAssignable)
	FComboGraphPlayMontageEventDelegate OnCancelled;

	/** One of the triggering gameplay events happened */
	UPROPERTY(BlueprintAssignable)
	FComboGraphPlayMontageEventDelegate EventReceived;

	/** Native event to communicate back to owning play combo task that we should transition to next node */
	FComboGraphMontageOnTriggerTransition OnTriggerTransition;

	/** Native event to communicate back to owning play combo task that we jump to a new section within a montage */
	FComboGraphMontageOnSectionChanged OnSectionChanged;

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool AbilityEnded) override;
	virtual void TickTask(float DeltaTime) override;
	void TickHandleNotifyState(USkeletalMeshComponent* SkeletalMeshComponent, float SequenceLength, UAnimNotifyState* NotifyState);
	bool ShouldTriggerTransition() const;
	bool ShouldHandleAutoSetup() const;

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
	* @param OwningAbility
	* @param TaskInstanceName Set to override the name of this task, for later querying
	* @param MontageToPlay The montage to play on the character
	* @param NotifiesAutoSetup Map of anim notify states and auto setup definitions
	* @param EventTags Any gameplay events matching this tag will activate the EventReceived callback. If empty, all events will trigger callback
	* @param Rate Change to play the montage faster or slower
	* @param StartSection Change to montage section to play during montage
	* @param bStopWhenAbilityEnds If true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled
	* @param AnimRootMotionTranslationScale Change to modify size of root motion or set to 0 to block it entirely
	*/
	UFUNCTION(BlueprintCallable, Category= "Combo Graph|Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UComboGraphAbilityTask_PlayMontage* PlayComboGraphMontage(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		UAnimMontage* MontageToPlay,
		TMap<TSoftClassPtr<UAnimNotifyState>, FComboGraphNotifyStateAutoSetup> NotifiesAutoSetup,
		FGameplayTagContainer EventTags,
		float Rate = 1.f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.f
	);

	float GetElapsedTime() const;
	void SetTransitionTriggerTime(float InTriggerTime);

private:

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle CancelledHandle;
	FDelegateHandle EventHandle;

	/** Montage that is playing */
	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** Current montage time */
	float ElapsedTime = 0.f;

	/** Current montage time for a given section */
	float CurrentSectionElapsedTime = 0.f;

	/** Trigger time for tick task to consider and check against Elapsed Time. If we're passed trigger time,  */
	float TransitionTriggerTime = -1.f;

	/** Ensures transition trigger happens only once. Checking if Elapsed Time has reached an anim notify trigger time */
	bool bTransitionWasTriggered = false;

	/** Whether auto setup notify states are registered */
	bool bHasNotifyAutoSetup = false;

	/** For notify auto setup, if there is, will store the computed length of linked sections if StartSection name provided is not None */
	float CachedSectionsLength = 0.f;

	/** Keep track of section name currently playing */
	FName CachedCurrentSection = NAME_None;

	/** Cached Anim Instance we get from ActorInfo */
	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> CachedAnimInstance;

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

	/** Cached list of notify auto setups, passed in during task creation */
	UPROPERTY()
	TArray<FComboGraphNotifyStateAutoSetup> NotifyStateAutoSetups;

	UPROPERTY()
	TMap<TSoftClassPtr<UAnimNotifyState>, FComboGraphNotifyStateAutoSetup> NotifiesAutoSetup;

	/** Cached list of Anim Notify States to consider notifying during tick*/
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimNotifyState>> RegisteredNotifyStates;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, FComboGraphRegisteredNotifyData> RegisteredNotifyData;

	/** Store */

	// TArray<UAnimNotifyState>

	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage() const;

	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted) const;
	void OnAbilityCancelled() const;
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) const;

	void CreateAutoSetupNotifyStates();

	/** Helper to return the combo notify state from a montage, if it has one */
	static FAnimNotifyEvent* GetAnimNotify(UAnimSequenceBase* AnimSequenceBase, UClass* NotifyClass);
	static bool HasAnimNotify(UAnimMontage* Montage, UClass* NotifyClass);
};
