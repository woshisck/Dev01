// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Components/ComboGraphGameplayTasksComponent.h"

#include "ComboGraphLog.h"
#include "Utils/ComboGraphBlueprintLibrary.h"
#include "Utils/ComboGraphUtils.h"

void UComboGraphGameplayTasksComponent::SendGameplayEventReplicated(const FGameplayTag EventTag, const FGameplayEventData Payload, const bool bSendOnlyOnAuthority)
{
	const FString WorldLogPrefix = FComboGraphUtils::GetWorldLogPrefix(GetWorld());

	CG_RUNTIME_LOG(Verbose, TEXT("%sUComboGraphManagerComponent::SendGameplayEventReplicated Tag: %s"), *WorldLogPrefix, *EventTag.ToString())

	ServerSendGameplayEvent(EventTag, Payload);
	if (!bSendOnlyOnAuthority)
	{
		UComboGraphBlueprintLibrary::SendGameplayEventToActor(GetOwner(), EventTag, Payload);
	}
}

void UComboGraphGameplayTasksComponent::ServerSendGameplayEvent_Implementation(const FGameplayTag EventTag, const FGameplayEventData Payload)
{
	const FString WorldLogPrefix = FComboGraphUtils::GetWorldLogPrefix(GetWorld());

	CG_RUNTIME_LOG(Verbose, TEXT("%sUComboGraphManagerComponent::ServerSendGameplayEvent Tag: %s"), *WorldLogPrefix, *EventTag.ToString())

	UComboGraphBlueprintLibrary::SendGameplayEventToActor(GetOwner(), EventTag, Payload);
}
