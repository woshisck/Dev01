// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphRuntimeTypes.h"
#include "Abilities/GameplayAbility.h"
#include "ComboGraphNativeAbility.generated.h"

class UInputAction;
class UComboGraphAbilityTask_StartGraph;

/**
 * Internal native Gameplay Ability to run Combo Graph passed in from Gameplay Event Data.
 */
UCLASS(NotBlueprintable)
class COMBOGRAPH_API UComboGraphNativeAbility : public UGameplayAbility
{
	GENERATED_BODY()

	DECLARE_MULTICAST_DELEGATE_TwoParams(FComboGraphAbilityEventDelegate, FGameplayTag, FGameplayEventData);

public:
	explicit UComboGraphNativeAbility(const FObjectInitializer& ObjectInitializer);

	/** Native delegate invoked when underlying task EventReceive event is triggered */
	FComboGraphAbilityEventDelegate OnEventReceived;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UComboGraphAbilityTask_StartGraph> Task;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void HandleEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void HandleOnGraphEnd(FGameplayTag EventTag, FGameplayEventData EventData);

private:

	UComboGraph* GetComboGraphFromEventData(FGameplayEventData EventData);
	UInputAction* GetInputActionEventData(FGameplayEventData EventData);

};
