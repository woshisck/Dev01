// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "Abilities/Tasks/ComboGraphAbilityTask_NetworkSyncPoint.h"

#include "AbilitySystemComponent.h"
#include "Runtime/Launch/Resources/Version.h"

UComboGraphAbilityTask_NetworkSyncPoint::UComboGraphAbilityTask_NetworkSyncPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SyncType = EComboGraphAbilityTaskNetSyncType::OnlyServerWait;
	ReplicatedEventToListenFor = EAbilityGenericReplicatedEvent::MAX;
}

void UComboGraphAbilityTask_NetworkSyncPoint::OnSignalCallback()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	if (AbilitySystemComponent.IsValid())
#else
	if (AbilitySystemComponent)
#endif
	{
		AbilitySystemComponent->ConsumeGenericReplicatedEvent(ReplicatedEventToListenFor, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}
	SyncFinished();
}

UComboGraphAbilityTask_NetworkSyncPoint* UComboGraphAbilityTask_NetworkSyncPoint::WaitNetSync(class UGameplayAbility* OwningAbility, EComboGraphAbilityTaskNetSyncType InSyncType)
{
	UComboGraphAbilityTask_NetworkSyncPoint* MyObj = NewAbilityTask<UComboGraphAbilityTask_NetworkSyncPoint>(OwningAbility);
	MyObj->SyncType = InSyncType;
	return MyObj;
}

void UComboGraphAbilityTask_NetworkSyncPoint::Activate()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());
#else
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent, IsPredictingClient());
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	if (AbilitySystemComponent.IsValid())
#else
	if (AbilitySystemComponent)
#endif
	{
		if (IsPredictingClient())
		{
			if (SyncType != EComboGraphAbilityTaskNetSyncType::OnlyServerWait)
			{
				// As long as we are waiting (!= OnlyServerWait), listen for the GenericSignalFromServer event
				ReplicatedEventToListenFor = EAbilityGenericReplicatedEvent::GenericSignalFromServer;
			}
			if (SyncType != EComboGraphAbilityTaskNetSyncType::OnlyClientWait)
			{
				// As long as the server is waiting (!= OnlyClientWait), send the Server and RPC for this signal
				AbilitySystemComponent->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::GenericSignalFromClient, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
			}
		}
		else if (IsForRemoteClient())
		{
			if (SyncType != EComboGraphAbilityTaskNetSyncType::OnlyClientWait)
			{
				// As long as we are waiting (!= OnlyClientWait), listen for the GenericSignalFromClient event
				ReplicatedEventToListenFor = EAbilityGenericReplicatedEvent::GenericSignalFromClient;
			}
			if (SyncType != EComboGraphAbilityTaskNetSyncType::OnlyServerWait)
			{
				// As long as the client is waiting (!= OnlyServerWait), send the Server and RPC for this signal
				AbilitySystemComponent->ClientSetReplicatedEvent(EAbilityGenericReplicatedEvent::GenericSignalFromServer, GetAbilitySpecHandle(), GetActivationPredictionKey());
			}
		}

		if (ReplicatedEventToListenFor != EAbilityGenericReplicatedEvent::MAX)
		{
			CallOrAddReplicatedDelegate(ReplicatedEventToListenFor, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UComboGraphAbilityTask_NetworkSyncPoint::OnSignalCallback));
		}
		else
		{
			// We aren't waiting for a replicated event, so the sync is complete.
			SyncFinished();
		}
	}
}

void UComboGraphAbilityTask_NetworkSyncPoint::SyncFinished()
{
	if (IsValid(this))
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnSync.Broadcast();
		}
		EndTask();
	}
}
