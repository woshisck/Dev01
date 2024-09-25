// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTasksComponent.h"
#include "ComboGraphGameplayTasksComponent.generated.h"

/**
 * Internal helper actor component spawned from Combo Graph task mainly to handle sending replicated gameplay event
 * to transition from a node to another on input press.
 *
 * This component shouldn't be Blueprintable nor BlueprintSpawnableComponent, child class can however.
 */
UCLASS()
class COMBOGRAPH_API UComboGraphGameplayTasksComponent : public UGameplayTasksComponent
{
	GENERATED_BODY()
public:

	/**
	 * This function is functionally the same as SendGameplayEventToActor (from Ability System Blueprint Library),
	 * except it can be used to also replicate the event on server.
	 *
	 * @param EventTag Gameplay Tag to use for the Event
	 * @param Payload Gameplay Event Data to send along the event
	 * @param bSendOnlyOnAuthority Whether to send the event only on server or on both server and local client
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability | Combo Graph", Meta = (DisplayName="SendGameplayEventToActor (Replicated)", AdvancedDisplay = "TaskOwner, bSendOnlyOnAuthority"))
	void SendGameplayEventReplicated(FGameplayTag EventTag, FGameplayEventData Payload, bool bSendOnlyOnAuthority = false);

	UFUNCTION(Server, Reliable)
	void ServerSendGameplayEvent(FGameplayTag EventTag, FGameplayEventData Payload);
};
