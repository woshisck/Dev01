// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphRuntimeTypes.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_StartGraph.h"
#include "ComboGraphEdge.generated.h"

class UComboGraphNodeBase;
class UInputAction;

UCLASS(Blueprintable, AutoExpandCategories=("Combo Graph | Transition", "Combo Graph | Icons"))
class COMBOGRAPH_API UComboGraphEdge : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The EnhancedInput Action to consider to transition to next node.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Graph | Transition")
	TObjectPtr<UInputAction> TransitionInput;

	/**
	 * The EnhancedInput Trigger Event type to use for the transition activation.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Combo Graph | Transition", meta=(EditCondition = "TransitionInput != nullptr", EditConditionHides))
	EComboGraphTransitionInputEvent TriggerEvent = EComboGraphTransitionInputEvent::Triggered;

	/**
	 * Transition Behavior.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Combo Graph | Transition", meta=(EditCondition = "TransitionInput != nullptr", EditConditionHides))
	EComboGraphTransitionBehavior TransitionBehavior = EComboGraphTransitionBehavior::OnComboWindowEnd;

	/**
	 * Configure the Anim Notify name (skeleton notify) to consider when Transition Behavior is set to transition on a specific notify frame.
	 *
	 * By the time an input trigger is received, combo transition will happen when the animation
	 * reaches this notify state or immediately if animation current time frame is passed the notify state.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Combo Graph | Transition", meta=(EditCondition = "TransitionBehavior == EComboGraphTransitionBehavior::OnAnimNotifyName", EditConditionHides))
	FName TransitionAnimNotifyName = NAME_None;

	/**
	 * Configure the Anim Notify class to consider when Transition Behavior is set to transition on a specific notify frame.
	 *
	 * By the time an input trigger is received, combo transition will happen when the animation
	 * reaches this notify state or immediately if animation current time frame is passed the notify state.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Combo Graph | Transition", meta=(EditCondition = "TransitionBehavior == EComboGraphTransitionBehavior::OnAnimNotifyClass", EditConditionHides))
	TSoftClassPtr<UAnimNotify> TransitionAnimNotify;

	/** Start or parent node for this edge */
	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeBase> StartNode;

	/** End or child node for this edge */
	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeBase> EndNode;

	//~ Begin UObject Interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface

	/** Confirmed flag getter, to track input confirmation and avoid multiple sending of gameplay events */
	bool IsConfirmed() const;

	/** Confirmed flag setter, to track input confirmation and avoid multiple sending of gameplay events */
	void SetIsConfirmed(const bool bInIsConfirmed);

	/** Returns the enhanced input event to consider for transitioning nodes */
	ETriggerEvent GetEnhancedInputTriggerEvent() const;

	/** Returns whether configured trigger event for this Edge is set on Canceled event */
	bool IsUsingCanceledTriggerEvent() const;

	/** Enhanced Input Action instance value getter */
	FInputActionInstance GetCurrentInputActionInstance() const;
	
	/** Enhanced Input Action instance value setter */
	void SetCurrentInputActionInstance(const FInputActionInstance& CurrentInputActionInstance);

#if WITH_EDITORONLY_DATA
	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	FText NodeTitle;

	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Icons")
	FLinearColor EdgeColour = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
#endif

#if WITH_EDITOR
	virtual FText GetNodeTitle() const { return NodeTitle; }
	FLinearColor GetEdgeColour() const { return EdgeColour; }
	virtual void SetNodeTitle(const FText& InTitle);
#endif

	
protected:
	/** The value of Input Action instance set during input confirm (replicated from client to server) */
	UPROPERTY(Replicated)
	FInputActionInstance CurrentInputActionInstance;

private:
	/** Keep track whether this edge has been confirmed already and prevent multiple sending of event */
	bool bConfirmed = false;
};
