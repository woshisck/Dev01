// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTask.h"
#include "GameplayTask_StartComboGraph.generated.h"

class UComboGraphNativeAbility;
class UGameplayAbility;
class UInputAction;
class UComboGraph;


/**
 * Convenience task for running combo graphs assets.
 */
UCLASS(BlueprintType)
class COMBOGRAPH_API UGameplayTask_StartComboGraph : public UGameplayTask
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComboGraphTaskEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

public:
	/** Event called when ability was successfully activated */
	UPROPERTY(BlueprintAssignable)
	FComboGraphTaskEventDelegate OnActivationSuccess;

	/** Event called when ability failed to activate */
	UPROPERTY(BlueprintAssignable)
	FComboGraphTaskEventDelegate OnActivationFailed;

	/** Event called when ability (and combo graph) ends */
	UPROPERTY(BlueprintAssignable)
	FComboGraphTaskEventDelegate OnGraphEnd;

	/** Event called when the owner receives a gameplay event combo graph is listening to */
	UPROPERTY(BlueprintAssignable)
	FComboGraphTaskEventDelegate EventReceived;

	UGameplayTask_StartComboGraph(const FObjectInitializer& ObjectInitializer);

	/**
	* This Gameplay Task is meant to be used to start / activate a Combo Graph Asset from outside a Gameplay Ability and regular GAS route.
	*
	* @param TaskOwner Gameplay Task Owner for the task (such as ComboGraphSystemComponent)
	* @param ComboGraph The Combo Graph asset to start / activate
	* @param InitialInput Initial Input Action to pass in to combo graph (only relevant with Conduit immediately following entry node). Without conduit, will be ignored.
	* @param bBroadcastInternalEvents Whether to broadcast internal gameplay events with EventReceived delegate (including Combo Being and End events)
	*/
	UFUNCTION(BlueprintCallable, Category = "GameplayTasks", meta = (DisplayName="Start Combo Graph for Gameplay Task", AdvancedDisplay = "bBroadcastInternalEvents, InitialInput", DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE"))
	static UGameplayTask_StartComboGraph* TaskStartComboGraph(
		TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
		UComboGraph* ComboGraph,
		UInputAction* InitialInput,
		bool bBroadcastInternalEvents = false
	);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	/** Ability Ended delegate callback */
	void OnAbilityEnded(UGameplayAbility* Ability);

	/** Ability received event delegate callback */
	void HandleEventReceived(FGameplayTag GameplayTag, FGameplayEventData GameplayEventData);

private:
	/** Cached Graph when activating the task for an actor (task param) */
	UPROPERTY(Transient)
	TObjectPtr<UComboGraph> CachedGraph;

	/** Cached Graph when activating the task for an actor (task param) */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InitialInput;

	UPROPERTY(Transient)
	TObjectPtr<UComboGraphNativeAbility> CachedAbility;

	/** Whether HandleEventReceived should broadcast internal events (task param) */
	bool bBroadcastInternalEvents = false;
};
