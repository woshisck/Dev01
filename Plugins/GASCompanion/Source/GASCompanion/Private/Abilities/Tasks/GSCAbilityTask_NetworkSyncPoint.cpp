// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Tasks/GSCAbilityTask_NetworkSyncPoint.h"

#include "AbilitySystemComponent.h"
#include "Runtime/Launch/Resources/Version.h"

UGSCAbilityTask_NetworkSyncPoint::UGSCAbilityTask_NetworkSyncPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), SyncType()
{
	ReplicatedEventToListenFor = EAbilityGenericReplicatedEvent::MAX;
}

void UGSCAbilityTask_NetworkSyncPoint::OnSignalCallback()
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

UGSCAbilityTask_NetworkSyncPoint* UGSCAbilityTask_NetworkSyncPoint::WaitNetSync(class UGameplayAbility* OwningAbility, EGSCAbilityTaskNetSyncType InSyncType)
{
	UGSCAbilityTask_NetworkSyncPoint* MyObj = NewAbilityTask<UGSCAbilityTask_NetworkSyncPoint>(OwningAbility);
	MyObj->SyncType = InSyncType;
	return MyObj;
}

void UGSCAbilityTask_NetworkSyncPoint::Activate()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());
	
	if (AbilitySystemComponent.IsValid())
#else
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent, IsPredictingClient());
	
	if (AbilitySystemComponent)
#endif
	{
		if (IsPredictingClient())
		{
			if (SyncType != EGSCAbilityTaskNetSyncType::OnlyServerWait )
			{
				// As long as we are waiting (!= OnlyServerWait), listen for the GenericSignalFromServer event
				ReplicatedEventToListenFor = EAbilityGenericReplicatedEvent::GenericSignalFromServer;
			}
			if (SyncType != EGSCAbilityTaskNetSyncType::OnlyClientWait)
			{
				// As long as the server is waiting (!= OnlyClientWait), send the Server and RPC for this signal
				AbilitySystemComponent->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::GenericSignalFromClient, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
			}
			
		}
		else if (IsForRemoteClient())
		{
			if (SyncType != EGSCAbilityTaskNetSyncType::OnlyClientWait )
			{
				// As long as we are waiting (!= OnlyClientWait), listen for the GenericSignalFromClient event
				ReplicatedEventToListenFor = EAbilityGenericReplicatedEvent::GenericSignalFromClient;
			}
			if (SyncType != EGSCAbilityTaskNetSyncType::OnlyServerWait)
			{
				// As long as the client is waiting (!= OnlyServerWait), send the Server and RPC for this signal
				AbilitySystemComponent->ClientSetReplicatedEvent(EAbilityGenericReplicatedEvent::GenericSignalFromServer, GetAbilitySpecHandle(), GetActivationPredictionKey());
			}
		}

		if (ReplicatedEventToListenFor != EAbilityGenericReplicatedEvent::MAX)
		{
			CallOrAddReplicatedDelegate(ReplicatedEventToListenFor, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UGSCAbilityTask_NetworkSyncPoint::OnSignalCallback));
		}
		else
		{
			// We aren't waiting for a replicated event, so the sync is complete.
			SyncFinished();
		}
	}
}

void UGSCAbilityTask_NetworkSyncPoint::SyncFinished()
{
	if (IsValid(this))
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnSync.Broadcast();
			OnSyncDelegate.Broadcast();
		}
		EndTask();
	}
}
