// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "BehaviorTree/ComboGraphBTTask_RunComboGraph.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "ComboGraphLog.h"
#include "GameplayTasksComponent.h"
#include "InputAction.h"
#include "TimerManager.h"
#include "Abilities/ComboGraphNativeTags.h"
#include "Abilities/Tasks/GameplayTask_StartComboGraph.h"
#include "Components/ComboGraphGameplayTasksComponent.h"
#include "Graph/ComboGraph.h"
#include "Utils/ComboGraphBlueprintLibrary.h"

UComboGraphBTTask_RunComboGraph::UComboGraphBTTask_RunComboGraph(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Run Combo Graph";

	// instantiating to be able to register delegates
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UComboGraphBTTask_RunComboGraph::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::ExecuteTask %s"), *GetNameSafe(OwnerComp.GetAIOwner()))

	if (!OwnerComp.GetAIOwner())
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;

	ControlledPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!ComboGraphAsset)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBTTask_RunComboGraph::ExecuteTask for %s (%s) has no valid Combo Graph Asset defined."), *GetNameSafe(OwnerComp.GetAIOwner()), *GetNameSafe(ControlledPawn))
		return EBTNodeResult::Failed;
	}

	// Look for a gameplay task component to use with the gameplay task (needs either UComboGraphGameplayTasksComponent or Ability System Component)
	// TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner;
	UGameplayTasksComponent* TasksComponent = nullptr;
	if (UComboGraphGameplayTasksComponent* ComboGraphComponent = UComboGraphBlueprintLibrary::GetComboGraphGameplayTasksComponent(ControlledPawn))
	{
		// TaskOwner = ComboGraphComponent;
		TasksComponent = ComboGraphComponent;
	}
	else if (UAbilitySystemComponent* ASC = UComboGraphBlueprintLibrary::GetAbilitySystemComponentFromActor(ControlledPawn))
	{
		// TaskOwner = ASC;
		TasksComponent = ASC;
	}

	// if (TaskOwner.GetInterface() == nullptr)
	if (!TasksComponent)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBTTask_RunComboGraph::ExecuteTask - %s owner has no Gameplay Tasks Component (either ComboGraphGameplayTasksComponent or an Ability System Component). This is required to run the combo graph gameplay task."), *GetNameSafe(ControlledPawn))
		return EBTNodeResult::Failed;
	}

	// Setup the Input Stack that will be processed on each combo end event received
	InputStack.Reset(Inputs.Num());
	InputStack.Append(Inputs);

	// Reverse the stack so that we can Pop and return the elements reversed
	Algo::Reverse(InputStack);

	// TODO: Check can safely pass in first item in the stack as Initial Input
	UInputAction* InitialInput = Inputs.IsValidIndex(0) ? Inputs[0] : nullptr;
	// Task = UGameplayTask_StartComboGraph::TaskStartComboGraph(TaskOwner, ComboGraphAsset, InitialInput, true);
	Task = UGameplayTask_StartComboGraph::TaskStartComboGraph(TasksComponent, ComboGraphAsset, InitialInput, true);
	if (!Task)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBTTask_RunComboGraph::ExecuteTask - Couldn't create gameplay task for owner %s"), *GetNameSafe(ControlledPawn))
		return EBTNodeResult::Failed;
	}

	Task->OnGraphEnd.AddDynamic(this, &UComboGraphBTTask_RunComboGraph::HandleGraphEnd);
	Task->EventReceived.AddDynamic(this, &UComboGraphBTTask_RunComboGraph::HandleEventReceived);
	Task->OnActivationFailed.AddDynamic(this, &UComboGraphBTTask_RunComboGraph::HandleActivationFailed);
	Task->ReadyForActivation();

	return EBTNodeResult::InProgress;
}

FString UComboGraphBTTask_RunComboGraph::GetStaticDescription() const
{

	TArray<FString> InputNames;
	for (UInputAction* Input : Inputs)
	{
		if (Input)
		{
			InputNames.Add("  " + Input->GetName());
		}
	}

	if (InputNames.Num() > 0)
	{
		InputNames.Insert("", 0);
	}

	const FString InputsDescription = FString::Join(InputNames, LINE_TERMINATOR);
	return FString::Printf(TEXT("%s: %s%s"), *Super::GetStaticDescription(), *GetNameSafe(ComboGraphAsset), *InputsDescription);
}

void UComboGraphBTTask_RunComboGraph::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
}

#if WITH_EDITOR
FName UComboGraphBTTask_RunComboGraph::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.RunBehavior.Icon");
}
#endif

EBTNodeResult::Type UComboGraphBTTask_RunComboGraph::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::AbortTask %s"), *GetNameSafe(OwnerComp.GetAIOwner()))
	CleanUp(OwnerComp);
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UComboGraphBTTask_RunComboGraph::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, const EBTNodeResult::Type TaskResult)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::OnTaskFinished %s"), *GetNameSafe(OwnerComp.GetAIOwner()))
	CleanUp(OwnerComp);
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphBTTask_RunComboGraph::CleanUp(UBehaviorTreeComponent& OwnerComp)
{
	if (Task)
	{
		Task->OnGraphEnd.RemoveAll(this);
		Task->EventReceived.RemoveAll(this);
		Task->OnActivationFailed.RemoveAll(this);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphBTTask_RunComboGraph::HandleGraphEnd(const FGameplayTag GameplayTag, FGameplayEventData GameplayEventData)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::HandleGraphEnd Tag: %s"), *GameplayTag.ToString())
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}

void UComboGraphBTTask_RunComboGraph::HandleEventReceived(const FGameplayTag GameplayTag, FGameplayEventData GameplayEventData)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::HandleEventReceived Tag: %s"), *GameplayTag.ToString())

	if (!ControlledPawn)
	{
		return;
	}

	// On Combo Begin event, schedule next input simulation on next frame
	if (GameplayTag == FComboGraphNativeTags::Get().ComboBegin && InputStack.Num() > 0)
	{
		ControlledPawn->GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UComboGraphBTTask_RunComboGraph::SimulateNextComboInput);
	}
}

void UComboGraphBTTask_RunComboGraph::SimulateNextComboInput()
{
	UInputAction* NextInput = InputStack.Pop();
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::HandleEventReceived SimulateNextComboInput with %s"), *GetNameSafe(NextInput))
	UComboGraphBlueprintLibrary::SimulateComboInput(ControlledPawn, NextInput);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphBTTask_RunComboGraph::HandleActivationFailed(const FGameplayTag GameplayTag, FGameplayEventData GameplayEventData)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBTTask_RunComboGraph::HandleActivationFailed Tag: %s"), *GameplayTag.ToString())
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Failed);
}
