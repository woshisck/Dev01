// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphRuntimeTypes.h"
#include "InputAction.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ComboGraphAbilityTask_StartGraph.generated.h"

class UComboGraphEdge;
class UComboGraphNodeConduit;
class UComboGraph;
class UInputAction;
class UComboGraphNodeAnimBase;
class UEnhancedInputComponent;
class UComboGraphAbilityTask_PlayMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComboGraphAbilityTaskDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
 * This task is meant to be used to start / activate a Combo Graph Asset from within a Gameplay Ability
 */
UCLASS()
class COMBOGRAPH_API UComboGraphAbilityTask_StartGraph : public UAbilityTask
{
	GENERATED_BODY()

public:
	/**
	* This task is meant to be used to start / activate a Combo Graph Asset from within a Gameplay Ability.
	*
	* @param OwningAbility
	* @param ComboGraph The Combo Graph asset to start / activate
	* @param InitialInput Initial Input Action to pass in to combo graph (only relevant with Conduit immediately following entry node). Without conduit, will be ignored.
	* @param bBroadcastInternalEvents Whether to broadcast internal gameplay events with EventReceived delegate (including Combo Being and End events)
	*/
	UFUNCTION(BlueprintCallable, Category= "Combo Graph|Ability|Tasks", meta = (DisplayName = "Start Combo Graph for Ability Task", AdvancedDisplay = "bBroadcastInternalEvents, InitialInput", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UComboGraphAbilityTask_StartGraph* CreateStartComboGraph(
		UGameplayAbility* OwningAbility,
		UComboGraph* ComboGraph,
		UInputAction* InitialInput,
		bool bBroadcastInternalEvents = false
	);

	/** Event invoked when the combat graph is started. */
	UPROPERTY(BlueprintAssignable)
	FComboGraphAbilityTaskDelegate OnGraphStart;

	/** Event invoked when the combat graph ends (usually when a montage completes, meaning no further input registered, thus should end the ability) */
	UPROPERTY(BlueprintAssignable)
	FComboGraphAbilityTaskDelegate OnGraphEnd;

	/** Event triggered when a combo graph node is listening for specific tags (either via EventTags or from Cues / Effects Containers) and owning avatar actor is receiving those events */
	UPROPERTY(BlueprintAssignable)
	FComboGraphAbilityTaskDelegate EventReceived;

	//~ Begin UObject Interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~ End UObject Interface

	//~ UAbilityTask / UGameplayTask Interface
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	//~ End UAbilityTask / UGameplayTask Interface

	/** Returns the currently active node for the task */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	UComboGraphNodeAnimBase* GetCurrentNode() const { return CurrentNode; }

	/** Returns the node that was active just before the current one */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	UComboGraphNodeAnimBase* GetPreviousNode() const { return PreviousNode; }

	/** Returns the node to go through on combo transition. Set in OnInputPressed for EventReceived to know which node to go next, based on input. */
	UComboGraphNodeAnimBase* GetQueuedNode() const { return QueuedNode; }

	AActor* GetAvatarActorFromActorInfo() const;
	UAnimInstance* GetAnimInstanceFromActorInfo() const;
	APlayerController* GetPlayerControllerFromActorInfo() const;

	UAnimMontage* GetCurrentMontageFromAbility() const;

	/** Provides getter for combo window state for external code to know (namely edges) */
	bool IsComboWindowOpened() const { return bComboWindowOpened; };

protected:

	/** Cached Graph when activating the task for an actor (task param) */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraph> RunningGraph;

	/** Cached Graph when activating the task for an actor (task param) */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InitialInput;

	/** Currently running montage task */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraphAbilityTask_PlayMontage> RunningTask;

	/** Pointer to active current node in the graph */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraphNodeAnimBase> CurrentNode;

	/** Pointer to conduit node immediately following entry node (if it exists) */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraphNodeConduit> ConduitNode;

	/** The owner pawn input component. */
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> InputComponent;

	/** Holds Enhanced Input values so that we can feed GetMagnitudeForContainer on Nodes */
	FInputActionInstance CurrentInputActionInstance;

	/** Whether HandleEventReceived should broadcast internal events (task param) */
	bool bBroadcastInternalEvents = false;

	int32 NodeIndex = 0;
	uint32 OnPressedHandle = 0;
	uint32 OnReleasedHandle = 0;

	TArray<uint32> InputHandles;

	/** Pointer to next node to go through on combo transition. Set in OnInputPressed for EventReceived to know which node to go next, based on input. */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraphNodeAnimBase> QueuedNode;

	/** Pointer to prev node we are transitioning from. Set in ServerSync when we receive the end combo event to transition to next node. */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraphNodeAnimBase> PreviousNode;

	/** Indicates state of combo window during which player can queue input */
	bool bComboWindowOpened = false;

	/** Indicates player input has been registered during combo window */
	bool bComboQueued = false;

	//~ UAbilityTask / UGameplayTask Interface
	virtual void Activate() override;
	//~ End UAbilityTask / UGameplayTask Interface

	virtual void OnNextFrame();
	virtual bool StartComboGraph(FString& FailReason);
	virtual bool AdvanceComboGraph(FString& FailReason);

	virtual bool AdvanceNextNode(UComboGraphNodeAnimBase* NextNode, FString& FailReason);
	virtual bool CheckCostForNode(UComboGraphNodeAnimBase* Node, FString& FailReason);
	virtual bool CommitAbilityCostForNode(UComboGraphNodeAnimBase* Node, FString& FailReason);
	virtual UGameplayEffect* GetCostGameplayEffectForNode(UComboGraphNodeAnimBase* Node) const;

	/** Setup Gameplay Event listener once at the start of the graph to ensure input press are handled */
	void SetupInputEvents();

	/** Called on every node activation. Clears off previously bound input handles and bind new ones based on node edges. */
	void SetupInputBindings();
	
	/** Handle input press */
	void ReceivedInputConfirm(const FInputActionInstance& InputActionInstance, UComboGraphEdge* Edge);

	/** Release any previously bind input action. */
	void ClearInputBindings();

	UFUNCTION()
	void OnEventInputReceived(FGameplayEventData EventData);

	/** When an input is pressed and received, set up queued node to transition to if the combo window is currently opened. */
	void HandleInputConfirmed(UComboGraphNodeAnimBase* NextNode, const UComboGraphEdge* Edge);

	/** Creates and configure PlayComboGraphMontage task for the current node. */
	void CreatePlayMontageTask(UAnimMontage* Montage);

	/** Handles broadcast of EndGraph event and reset of debugging states */
	void EndGraph();

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void HandleEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	void HandleTriggerTransitionEvent();

	UFUNCTION()
	void OnServerSyncAdvanceNextNode();

	void HandleComboBeginEvent();
	void HandleComboEndEvent();
	void HandleComboTransition();
	void HandleComboTransitionForNotifyTriggerTime(float TriggerTime);

	/** Make gameplay effect container spec to be applied later, using the passed in container */
	virtual FComboGraphGameplayEffectContainerSpec MakeEffectContainerSpecFromContainer(const FComboGraphGameplayEffectContainer& Container, FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	/** Search for and make a gameplay effect container spec to be applied later, from the EffectsContainerMap */
	virtual FComboGraphGameplayEffectContainerSpec MakeEffectContainerSpec(TMap<FGameplayTag, FComboGraphGameplayEffectContainer> EffectContainerMap, FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	/** Applies a gameplay effect container spec that was previously created */
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainerSpec(const FComboGraphGameplayEffectContainerSpec& ContainerSpec);

	/** Apply a previously created gameplay effect spec to a target (GameplayAbility protected method) */
	TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectSpecToTarget(const FGameplayAbilitySpecHandle AbilityHandle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEffectSpecHandle SpecHandle, const FGameplayAbilityTargetDataHandle& TargetData) const;

	/** Applies a gameplay effect container, by creating and then applying the spec */
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainer(TMap<FGameplayTag, FComboGraphGameplayEffectContainer> EffectContainerMap, FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	virtual void ApplyEffectOrDamageContainer(UComboGraphNodeAnimBase* ComboNode, FGameplayTag EventTag, FGameplayEventData EventData);
	void ApplyDamageContainer(UComboGraphNodeAnimBase* ComboNode, FGameplayTag EventTag, FGameplayEventData EventData) const;

	/** Checks all TargetData actors can apply a gameplay effect by checking if they have a valid AbilitySystemComponent */
	bool CanApplyEffectsToTargets(const FGameplayAbilityTargetDataHandle TargetDataHandle, FString& FailReason) const;

private:
	/** Checks ASC NetMode to be NM_Standalone */
	bool IsStandalone() const;

	/** Apply a gameplay effect to the owner of task's owning ability (custom version here since those methods are protected in UGameplayAbility) */
	FActiveGameplayEffectHandle ApplyGameplayEffectToOwner(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const UGameplayEffect* GameplayEffect, float GameplayEffectLevel, int32 Stacks = 1) const;

	/** Apply a previously created gameplay effect spec to the owner of task's owning ability (custom version here since those methods are protected in UGameplayAbility) */
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToOwner(const FGameplayAbilitySpecHandle AbilityHandle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEffectSpecHandle SpecHandle) const;

	/** Tests if all modifiers in this GameplayEffect will leave the attribute > 0.f (custom version to check for AttributeSet being actually granted and avoid an engine crash if not) */
	bool CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext) const;

	/** Returns spawned attribute set from passed in ASC based on provided AttributeClass (mainly because GetAttributeSubobject on AbilitySystemComponent is protected) */
	const UAttributeSet* GetAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass) const;

	/** Whether HandleEventReceived should broadcast internal events */
	bool ShouldBroadcastInternalEvents() const;

	/** Returns final magnitude to apply by asking current node for an adjusted value (if implemented in BP) */
	float GetNodeMagnitudeForContainer(const UComboGraphNodeAnimBase* ComboNode, const float OriginalMagnitude, FGameplayTag ContainerTag) const;
};
