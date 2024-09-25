// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Abilities/ComboGraphNativeAbility.h"

#include "ComboGraphLog.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_StartGraph.h"
#include "Graph/ComboGraph.h"

UComboGraphNativeAbility::UComboGraphNativeAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// TODO: Figure out native tag
	// FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.ComboGraph.Native"));
	// AbilityTags.AddTag(AbilityTag);
	// ActivationOwnedTags.AddTag(AbilityTag);
}

void UComboGraphNativeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNativeAbility: Passed in TriggerEventData is invalid"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayEventData EventData = *TriggerEventData;
	UComboGraph* Graph = GetComboGraphFromEventData(EventData);
	UInputAction* InputAction = GetInputActionEventData(EventData);
	const bool bBroadcastInternalEvents = TriggerEventData->EventMagnitude != 0.f;

	if (!Graph)
	{
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphNativeAbility: Unable to run NULL combo graph asset (EventData OptionalObjects are not a Combo Graph Asset)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		//	Failed to commit ... end ability with failed state
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphNativeAbility: Ability %s failed to commit"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Task = UComboGraphAbilityTask_StartGraph::CreateStartComboGraph(this, Graph, InputAction, bBroadcastInternalEvents);
	if (!Task)
	{
		//	Failed to create task ... end ability with failed state
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNativeAbility: Ability %s couldn't create combo graph task"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Task->OnGraphEnd.AddDynamic(this, &UComboGraphNativeAbility::HandleOnGraphEnd);
	Task->EventReceived.AddDynamic(this, &UComboGraphNativeAbility::HandleEventReceived);
	Task->ReadyForActivation();
}

void UComboGraphNativeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Clear off any delegates previously bound
	if (Task)
	{
		Task->OnGraphEnd.RemoveAll(this);
		Task->EventReceived.RemoveAll(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphNativeAbility::HandleEventReceived(const FGameplayTag EventTag, const FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphNativeAbility::HandleEventReceived"));
	OnEventReceived.Broadcast(EventTag, EventData);
}

void UComboGraphNativeAbility::HandleOnGraphEnd(FGameplayTag EventTag, FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphNativeAbility::HandleOnGraphEnd - Graph ended, End Ability."));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UComboGraph* UComboGraphNativeAbility::GetComboGraphFromEventData(const FGameplayEventData EventData)
{
	if (const UComboGraph* Graph = Cast<UComboGraph>(EventData.OptionalObject))
	{
		return const_cast<UComboGraph*>(Graph);
	}

	if (const UComboGraph* Graph = Cast<UComboGraph>(EventData.OptionalObject2))
	{
		return const_cast<UComboGraph*>(Graph);
	}

	return nullptr;
}

UInputAction* UComboGraphNativeAbility::GetInputActionEventData(FGameplayEventData EventData)
{
	if (const UInputAction* InputAction = Cast<UInputAction>(EventData.OptionalObject))
	{
		return const_cast<UInputAction*>(InputAction);
	}

	if (const UInputAction* InputAction = Cast<UInputAction>(EventData.OptionalObject2))
	{
		return const_cast<UInputAction*>(InputAction);
	}

	return nullptr;
}
