// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Abilities/Tasks/GameplayTask_StartComboGraph.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ComboGraphLog.h"
#include "InputAction.h"
#include "Abilities/ComboGraphNativeAbility.h"
#include "Graph/ComboGraph.h"
#include "Utils/ComboGraphBlueprintLibrary.h"

UGameplayTask_StartComboGraph::UGameplayTask_StartComboGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UGameplayTask_StartComboGraph* UGameplayTask_StartComboGraph::TaskStartComboGraph(const TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner, UComboGraph* ComboGraph, UInputAction* InitialInput, const bool bBroadcastInternalEvents)
{
	// Check that we're not trying to use the gameplay task version from within a GA
	UObject* TaskObject = TaskOwner.GetObject();
	if (TaskObject)
	{
		if (TaskObject->IsA(UGameplayAbility::StaticClass()))
		{
			CG_RUNTIME_SLOG(Error, TEXT("UGameplayTask_StartComboGraph: Trying to use Gameplay Task version from within a Gameplay Ability. Try to use StartComboGraph Ability Task instead."));
			return nullptr;
		}
	}

	// Check that provided Combo Graph is actually valid
	if (!ComboGraph)
	{
		CG_RUNTIME_SLOG(Error, TEXT("UGameplayTask_StartComboGraph: Provided Combo Graph asset is invalid."));
		return nullptr;
	}

	UGameplayTask_StartComboGraph* Task = NewTask<UGameplayTask_StartComboGraph>(TaskOwner);
	if (Task)
	{
		Task->CachedGraph = ComboGraph;
		Task->InitialInput = InitialInput;
		Task->bBroadcastInternalEvents = bBroadcastInternalEvents;
	}
	return Task;
}

void UGameplayTask_StartComboGraph::Activate()
{
	const AActor* OwnerActor = GetOwnerActor();
	const AActor* AvatarActor = GetAvatarActor();

	CG_RUNTIME_LOG(Verbose, TEXT("UGameplayTask_StartComboGraph Activate - Owner: %s, Avatar: %s, Graph: %s"), *GetNameSafe(OwnerActor), *GetNameSafe(AvatarActor), *GetNameSafe(CachedGraph))

	// Check ASC on avatar for this task
	UAbilitySystemComponent* ASC = UComboGraphBlueprintLibrary::GetAbilitySystemComponentFromActor(AvatarActor);
	if (!ASC)
	{
		CG_RUNTIME_LOG(Error, TEXT("UGameplayTask_StartComboGraph Activate - %s actor has no ASC we can rely on"), *GetNameSafe(AvatarActor));
		EndTask();
		return;
	}

	// On gameplay task activation, we simply forward to combo graph native ability

	const TSubclassOf<UGameplayAbility> Ability = UComboGraphNativeAbility::StaticClass();

	// Use existing spec if ability already granted
	FGameplayAbilitySpec* ExistingSpec = ASC->FindAbilitySpecFromClass(Ability);
	if (!ExistingSpec)
	{
		CG_RUNTIME_LOG(Error, TEXT("UGameplayTask_StartComboGraph Activate - ASC on %s actor doesn't have %s ability granted"), *GetNameSafe(AvatarActor), *Ability->GetName());
		EndTask();
		return;
	}

	FGameplayEventData EventData;
	EventData.OptionalObject = CachedGraph;
	EventData.OptionalObject2 = InitialInput;
	// We use the Event Magnitude to act as a boolean to pass down the info if we should broadcast internal events or not
	EventData.EventMagnitude = bBroadcastInternalEvents ? 1.f : 0.f;

	FOnGameplayAbilityEnded::FDelegate AbilityEndedDelegate = FOnGameplayAbilityEnded::FDelegate::CreateUObject(this, &UGameplayTask_StartComboGraph::OnAbilityEnded);
	UGameplayAbility* InstancedAbility = nullptr;
	if (!ASC->InternalTryActivateAbility(ExistingSpec->Handle, FPredictionKey(), &InstancedAbility, &AbilityEndedDelegate, &EventData))
	{
		// We failed to activate it, so end task now
		OnActivationFailed.Broadcast(FGameplayTag(), FGameplayEventData());
		EndTask();
		return;
	}

	if (UComboGraphNativeAbility* ComboAbility = Cast<UComboGraphNativeAbility>(InstancedAbility))
	{
		CachedAbility = ComboAbility;

		// Setup delegates
		ComboAbility->OnEventReceived.AddUObject(this, &UGameplayTask_StartComboGraph::HandleEventReceived);
	}

	CG_RUNTIME_LOG(Verbose, TEXT("UGameplayTask_StartComboGraph Activate - Instanced Ability %s"), *GetNameSafe(InstancedAbility));
	OnActivationSuccess.Broadcast(FGameplayTag(), FGameplayEventData());
}

void UGameplayTask_StartComboGraph::OnDestroy(const bool bInOwnerFinished)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UGameplayTask_StartComboGraph OnDestroy - bInOwnerFinished %s"), *GetBoolText(bInOwnerFinished));
	if (CachedAbility)
	{
		CG_RUNTIME_LOG(Verbose, TEXT("UGameplayTask_StartComboGraph OnDestroy - Clean up delegate %s"), *GetBoolText(bInOwnerFinished));
		CachedAbility->OnEventReceived.RemoveAll(this);
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UGameplayTask_StartComboGraph::OnAbilityEnded(UGameplayAbility* Ability)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UGameplayTask_StartComboGraph OnAbilityEnded - %s just ended"), *GetNameSafe(Ability));
	OnGraphEnd.Broadcast(FGameplayTag(), FGameplayEventData());
	EndTask();
}

void UGameplayTask_StartComboGraph::HandleEventReceived(const FGameplayTag GameplayTag, const FGameplayEventData GameplayEventData)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UGameplayTask_StartComboGraph Handle Event Received %s"), *GameplayTag.ToString());
	EventReceived.Broadcast(GameplayTag, GameplayEventData);
}
