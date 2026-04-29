// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "Abilities/Tasks/ComboGraphAbilityTask_StartGraph.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ComboGraphDelegates.h"
#include "ComboGraphLog.h"
#include "EnhancedInputComponent.h"
#include "GameplayCue_Types.h"
#include "TimerManager.h"
#include "Abilities/ComboGraphNativeTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_PlayMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/ComboGraphGameplayTasksComponent.h"
#include "Graph/ComboGraph.h"
#include "Graph/ComboGraphEdge.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "Graph/ComboGraphNodeConduit.h"
#include "Graph/ComboGraphNodeEntry.h"
#include "Graph/ComboGraphNodeMontage.h"
#include "Graph/ComboGraphNodeSequence.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Utils/ComboGraphUtils.h"

UComboGraphAbilityTask_StartGraph* UComboGraphAbilityTask_StartGraph::CreateStartComboGraph(UGameplayAbility* OwningAbility, UComboGraph* ComboGraph, UInputAction* InitialInput, const bool bBroadcastInternalEvents)
{
	CG_RUNTIME_LOG(
		Log,
		TEXT("UComboGraphAbilityTask_StartGraph::CreateStartComboGraph ... for Graph: %s (Initial Input: %s), BroadcastInternalEvents: %s"),
		*GetNameSafe(ComboGraph),
		*GetNameSafe(InitialInput),
		*GetBoolText(bBroadcastInternalEvents)
	)

	UComboGraphAbilityTask_StartGraph* Task = NewAbilityTask<UComboGraphAbilityTask_StartGraph>(OwningAbility);
	if (Task)
	{
		Task->RunningGraph = ComboGraph;
		Task->InitialInput = InitialInput;
		Task->bBroadcastInternalEvents = bBroadcastInternalEvents;
	}

	return Task;
}

void UComboGraphAbilityTask_StartGraph::ExternalCancel()
{
	Super::ExternalCancel();
}

FString UComboGraphAbilityTask_StartGraph::GetDebugString() const
{
	return FString::Printf(TEXT("%s (Combo Asset: %s)"), *GetName(), *GetNameSafe(RunningGraph));
}

void UComboGraphAbilityTask_StartGraph::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}

AActor* UComboGraphAbilityTask_StartGraph::GetAvatarActorFromActorInfo() const
{
	check(Ability)
	const FGameplayAbilityActorInfo ActorInfo = Ability->GetActorInfo();
	return ActorInfo.AvatarActor.IsValid() ? ActorInfo.AvatarActor.Get() : nullptr;
}

UAnimInstance* UComboGraphAbilityTask_StartGraph::GetAnimInstanceFromActorInfo() const
{
	check(Ability)
	const FGameplayAbilityActorInfo ActorInfo = Ability->GetActorInfo();
	return ActorInfo.AnimInstance.IsValid() ? ActorInfo.AnimInstance.Get() : nullptr;
}

APlayerController* UComboGraphAbilityTask_StartGraph::GetPlayerControllerFromActorInfo() const
{
	check(Ability)
	const FGameplayAbilityActorInfo ActorInfo = Ability->GetActorInfo();
	return ActorInfo.PlayerController.IsValid() ? ActorInfo.PlayerController.Get() : nullptr;
}

UAnimMontage* UComboGraphAbilityTask_StartGraph::GetCurrentMontageFromAbility() const
{
	check(Ability)
	return Ability->GetCurrentMontage();
}

void UComboGraphAbilityTask_StartGraph::Activate()
{
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphAbilityTask_StartGraph::Activate ... for Graph: %s"), *GetNameSafe(RunningGraph))

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (ActorInfo->AvatarActor.IsValid())
	{
		InputComponent = Cast<UEnhancedInputComponent>(ActorInfo->AvatarActor.Get()->InputComponent);
	}

	// Check and spawns helper component for this task, mainly to be able to send replicated gameplay event and transition from a node to another on input press
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphAbilityTask_StartGraph: AvatarActor is invalid"));
		EndGraph();
		return;
	}

	UComboGraphGameplayTasksComponent* Component = AvatarActor->FindComponentByClass<UComboGraphGameplayTasksComponent>();
	if (Component == nullptr && AvatarActor->GetLocalRole() == ROLE_Authority)
	{
		CG_RUNTIME_LOG(Log, TEXT("UComboGraphAbilityTask_StartGraph: spawning UComboGraphGameplayTasksComponent.."));
		Component = NewObject<UComboGraphGameplayTasksComponent>(AvatarActor, TEXT("ComboGraphGameplayTasksComponent_Spawned"));
		Component->RegisterComponent();
	}

	// Schedule activation on next frame (trying to get rid of ensure condition
	// errors with Enhanced Input component about array changed during range-for iteration, might be 4.26 only)
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UComboGraphAbilityTask_StartGraph::OnNextFrame);
}

void UComboGraphAbilityTask_StartGraph::OnNextFrame()
{
	FString FailReason;
	if (!StartComboGraph(FailReason))
	{
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphAbilityTask_StartGraph::Activate - StartComboGraph failed. Reason: %s"), *FailReason)
		EndGraph();
	}
}

bool UComboGraphAbilityTask_StartGraph::StartComboGraph(FString& FailReason)
{
	check(Ability);

	CG_RUNTIME_LOG(Log, TEXT("UComboGraphAbilityTask_StartGraph::StartComboGraph ... for Graph: %s"), *GetNameSafe(RunningGraph))
	if (!RunningGraph)
	{
		FailReason = TEXT("StartComboGraph - RunningGraph is nullptr");
		return false;
	}

	UComboGraphNodeBase* FirstRootNode = RunningGraph->FirstNode;
	if (!FirstRootNode)
	{
		FailReason = TEXT("StartComboGraph - Graph entry node is invalid.");
		return false;
	}

	CG_RUNTIME_LOG(Verbose, TEXT("StartComboGraph: FirstRootNode: %s (%s)"), *GetNameSafe(FirstRootNode), *GetNameSafe(FirstRootNode->GetClass()))

	NodeIndex = 0;
	InputHandles.Empty();

	PreviousNode = RunningGraph->EntryNode;
	ConduitNode = Cast<UComboGraphNodeConduit>(FirstRootNode);

	if (ConduitNode)
	{
		CG_RUNTIME_LOG(Verbose, TEXT("StartComboGraph: ConduitNode: %s (%s)"), *GetNameSafe(ConduitNode), *GetNameSafe(ConduitNode->GetClass()))

		if (!InitialInput)
		{
			FailReason = FString::Printf(TEXT("Graph `%s` is using a Conduit Node but task is missing an Initial Input Action parameter"), *GetNameSafe(RunningGraph));
			return false;
		}

		// If we have a conduit, determine which is the correct "first" node based on its edges and Initial Input if provided
		const UComboGraphEdge* Edge = ConduitNode->GetEdgeWithInput(InitialInput);
		if (!Edge)
		{
			FailReason = FString::Printf(
				TEXT("Graph `%s` using a Conduit and Initial Input `%s` but none of the edges connected to conduit are using provided Initial Input"),
				*GetNameSafe(RunningGraph),
				*GetNameSafe(InitialInput)
			);
			return false;
		}

		FirstRootNode = Edge->EndNode;
	}

	CurrentNode = Cast<UComboGraphNodeAnimBase>(FirstRootNode);
	if (!CurrentNode)
	{
		FailReason = FString::Printf(TEXT("Graph entry node is invalid (not animation node) - %s"), *GetNameSafe(FirstRootNode));
		return false;
	}

	SetupInputEvents();

	// Initialize entry node here via task interface
	if (RunningGraph->EntryNode)
	{
		RunningGraph->EntryNode->OnGameplayTaskInitialized(*this);
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FComboGraphDelegates::OnComboGraphStarted.Broadcast(*this, *RunningGraph);

		OnGraphStart.Broadcast(FGameplayTag(), FGameplayEventData());
	}

	return AdvanceComboGraph(FailReason);
}

bool UComboGraphAbilityTask_StartGraph::AdvanceComboGraph(FString& FailReason)
{
	if (!RunningGraph)
	{
		FailReason = TEXT("AdvanceComboGraph - RunningGraph is nullptr");
		return false;
	}

	const int32 CurrentIndex = NodeIndex;
	NodeIndex++;

	// On First pass, take current node or first valid child
	UComboGraphNodeBase* NextNode = CurrentIndex == 0 ? CurrentNode :
		CurrentNode->ChildrenNodes.IsValidIndex(0) ? CurrentNode->ChildrenNodes[0] :
		nullptr;

	if (!NextNode)
	{
		FailReason = TEXT("Can't determine next node. It has most likely no child nodes.");
		return false;
	}

	// Only advance if it's an anim node
	UComboGraphNodeAnimBase* NextAnimNode = Cast<UComboGraphNodeAnimBase>(NextNode);
	if (!NextAnimNode)
	{
		FailReason = TEXT("Trying to advance to node that is not animation based.");
		return false;
	}

	return AdvanceNextNode(NextAnimNode, FailReason);
}

bool UComboGraphAbilityTask_StartGraph::AdvanceNextNode(UComboGraphNodeAnimBase* NextNode, FString& FailReason)
{
	check(Ability);

	if (!RunningGraph)
	{
		FailReason = TEXT("AdvanceNextNode - RunningGraph is nullptr");
		return false;
	}

	// Reset combo states
	QueuedNode = nullptr;
	bComboQueued = false;
	bComboWindowOpened = false;

	if (!NextNode)
	{
		FailReason = TEXT("AdvanceNextNode - Passed in next node is invalid.");
		return false;
	}

	// Initialize task interface for node here
	NextNode->OnGameplayTaskInitialized(*this);
	UGameplayTasksComponent* TaskOwnerComponent = NextNode->GetGameplayTasksComponent(*this);
	if (TaskOwnerComponent)
	{
		TaskOwnerComponent->OnGameplayTaskInitialized(*this);
	}

	CG_RUNTIME_LOG(Verbose, TEXT("AdvanceNextNode: %s (%s)"), *GetNameSafe(NextNode), *GetNameSafe(NextNode->GetClass()))

	// Check BP implementable CanActivate
	if (!NextNode->K2_CanActivateNode())
	{
		FailReason = FString::Printf(TEXT("CanActivateNode() for %s prevented activation"), *NextNode->GetName());
		return false;
	}

	// Check and commit cost
	if (!CommitAbilityCostForNode(NextNode, FailReason))
	{
		return false;
	}

	// Notify current node about deactivate task interface here
	if (CurrentNode)
	{
		CurrentNode->OnGameplayTaskDeactivated(*this);
	}

	CurrentNode = NextNode;

	// Broadcast state change to Blueprints (BP can use task GetPreviousNode() / GetCurrentNode() to know transition infos)
	if (ShouldBroadcastInternalEvents())
	{
		FGameplayEventData EventData = FGameplayEventData();
		EventData.EventTag = FComboGraphNativeTags::Get().StateChange;
		EventReceived.Broadcast(EventData.EventTag, EventData);
	}

	// Activate task interface here for next node
	CurrentNode->OnGameplayTaskActivated(*this);

	// Inputs are only handled client side
	//
	// Currently using replicated Gameplay Events (sent both on server / client when client register an input to communicate the intent on server too).
	// Seems to work but implies doing RPC calls in an ActorComponent.
	SetupInputBindings();

	// Warn users about using Node Sequences in networked environment
	const bool bIsSequence = CurrentNode->IsA(UComboGraphNodeSequence::StaticClass());
	const bool bShouldWarn = FComboGraphUtils::GetPluginProjectSettings()->bSequencesNetworkedWarning;
	const bool bHasAuthority = Ability->K2_HasAuthority();

	const bool bLocallyControlled = IsLocallyControlled();
	CG_RUNTIME_LOG(Log, TEXT("Test: bLocallyControlled ? %s (%s)"), bLocallyControlled ? TEXT("true") : TEXT("false"), *GetAvatarActorFromActorInfo()->GetName())

	if (bIsSequence && bShouldWarn && bHasAuthority)
	{
		const bool bIsStandalone = IsStandalone();
		CG_RUNTIME_LOG(Log, TEXT("Test: NetMode Standalone ? %s (%s)"), bIsStandalone ? TEXT("true") : TEXT("false"), *GetAvatarActorFromActorInfo()->GetName())
		if (!bIsStandalone)
		{
			CG_RUNTIME_SLOG(Error, TEXT("Combo Ability: Using Animation Sequences in networked environment is not supported. Try using Animation Montages instead."))
		}
	}

	UAnimMontage* Montage = CurrentNode->GetMontage();
	if (!Montage)
	{
		CG_RUNTIME_LOG(Error, TEXT("AdvanceNextNode - Current Node GetMontage() returns invalid Montage (%s)"), *CurrentNode->GetName())
		return false;
	}

	// Debug:
	// const FString CurrentInputActionInstanceDebug = FString::Printf(TEXT("Elapsed Time: %f, Triggered Time: %f"), CurrentInputActionInstance.GetElapsedTime(), CurrentInputActionInstance.GetTriggeredTime());
	// CG_RUNTIME_SLOG(Display, TEXT("[%s] CG_TEST CurrentInputActionInstance: %s"), *GetDebugStringForWorld(GetWorld()), *CurrentInputActionInstanceDebug)

	// Play montage task
	CreatePlayMontageTask(Montage);
	CurrentNode->OnMontagePlay(Montage);

	return true;
}

bool UComboGraphAbilityTask_StartGraph::CheckCostForNode(UComboGraphNodeAnimBase* Node, FString& FailReason)
{
	check(Node);
	check(Ability);

	const UGameplayEffect* CostGE = GetCostGameplayEffectForNode(Node);
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	const FGameplayAbilitySpecHandle Handle = Ability->GetCurrentAbilitySpecHandle();

	if (!ActorInfo)
	{
		FailReason = FString::Printf(TEXT("CheckCost for node %s failed. ActorInfo invalid"), *Node->GetName());
		return false;
	}

	if (!Handle.IsValid())
	{
		FailReason = FString::Printf(TEXT("CheckCost for node %s failed. Handle invalid"), *Node->GetName());
		return false;
	}

	if (CostGE)
	{
		if (!CanApplyAttributeModifiers(CostGE, Ability->GetAbilityLevel(Handle, ActorInfo), Ability->MakeEffectContext(Handle, ActorInfo)))
		{
			FailReason = FString::Printf(TEXT("CheckCost for node `%s` and Gameplay Effect `%s` failed. CanApplyAttributeModifiers failed to apply"), *Node->GetName(), *CostGE->GetName());
			return false;
		}
	}

	return true;
}

bool UComboGraphAbilityTask_StartGraph::CommitAbilityCostForNode(UComboGraphNodeAnimBase* Node, FString& FailReason)
{
	check(Ability);

	if (UAbilitySystemGlobals::Get().ShouldIgnoreCosts())
	{
		return true;
	}

	// Last chance to fail (maybe we no longer have resources to commit since we after we started this ability activation)
	if (!CheckCostForNode(Node, FailReason))
	{
		return false;
	}

	const UGameplayEffect* CostGE = GetCostGameplayEffectForNode(Node);
	if (CostGE)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		const FGameplayAbilitySpecHandle Handle = Ability->GetCurrentAbilitySpecHandle();
		const FGameplayAbilityActivationInfo ActivationInfo = Ability->GetCurrentActivationInfo();

		if (!ActorInfo)
		{
			FailReason = FString::Printf(TEXT("CommitAbilityCostForNode %s failed. ActorInfo invalid"), *Node->GetName());
			return false;
		}

		if (!Handle.IsValid())
		{
			FailReason = FString::Printf(TEXT("CommitAbilityCostForNode %s failed. Handle invalid"), *Node->GetName());
			return false;
		}

		// ReSharper disable once CppExpressionWithoutSideEffects
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CostGE, Ability->GetAbilityLevel(Handle, ActorInfo));
	}

	return true;
}

UGameplayEffect* UComboGraphAbilityTask_StartGraph::GetCostGameplayEffectForNode(UComboGraphNodeAnimBase* Node) const
{
	const TSubclassOf<UGameplayEffect> CostGEClass = Node->CostGameplayEffect;
	return CostGEClass ? CostGEClass->GetDefaultObject<UGameplayEffect>() : nullptr;
}

void UComboGraphAbilityTask_StartGraph::SetupInputEvents()
{
	check(Ability);

	CG_RUNTIME_LOG(Log, TEXT("SetupInputEvents: Node %s"), CurrentNode ? *CurrentNode->GetName() : TEXT("None"))

	// Setup events bindings for each edge if it has a valid transition input action
	const FGameplayTag Tag = FComboGraphNativeTags::Get().Input;
	CG_RUNTIME_LOG(Log, TEXT("SetupInputEvents: Wait event for %s and node (%s)"), *Tag.ToString(), CurrentNode ? *CurrentNode->GetName() : TEXT("None"))
	UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(Ability, Tag);
	Task->EventReceived.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::OnEventInputReceived);
	Task->ReadyForActivation();
}

void UComboGraphAbilityTask_StartGraph::SetupInputBindings()
{
	CG_RUNTIME_LOG(Log, TEXT("SetupInputBindings: Node %s"), CurrentNode ? *CurrentNode->GetName() : TEXT("None"))
	CG_RUNTIME_LOG(Log, TEXT("SetupInputBindings (ReceivedInput): Node %s"), CurrentNode ? *CurrentNode->GetName() : TEXT("None"))

	if (!InputComponent)
	{
		// AI Controlled pawn or on server
		const FString Prefix = FComboGraphUtils::GetWorldLogPrefix(GetWorld());
		CG_RUNTIME_LOG(Log, TEXT("%sSetupInputBindings: No Input Component"), *Prefix)
		return;
	}

	ClearInputBindings();

	// Setup input bindings for each edge if it has a valid transition input action
	for (UComboGraphNodeBase* ChildNode : CurrentNode->ChildrenNodes)
	{
		UComboGraphEdge* Edge = CurrentNode->GetEdge(ChildNode);
		if (!Edge)
		{
			continue;
		}

		if (Edge->TransitionInput)
		{
			const UInputAction* InputAction = Edge->TransitionInput;

			// Convert out internal enum to the real Input Trigger Event for Enhanced Input
			const ETriggerEvent TriggerEvent = Edge->GetEnhancedInputTriggerEvent();

			CG_RUNTIME_LOG(
				Verbose,
				TEXT("SetupInputBindings for Node %s: Edge %s with Transition Event: %s"),
				*GetNameSafe(CurrentNode),
				*Edge->GetName(),
				*UEnum::GetValueAsString(TriggerEvent)
			)

			// Reset back to false when we start listening for input in case it was switched previously.
			Edge->SetIsConfirmed(false);

			uint32 ConfirmHandle = InputComponent->BindAction(InputAction, TriggerEvent, this, &UComboGraphAbilityTask_StartGraph::ReceivedInputConfirm, Edge).GetHandle();
			InputHandles.Add(ConfirmHandle);
		}
	}
}

void UComboGraphAbilityTask_StartGraph::ReceivedInputConfirm(const FInputActionInstance& InputActionInstance, UComboGraphEdge* Edge)
{
	if (!Edge)
	{
		CG_RUNTIME_LOG(Error, TEXT("ReceivedInputConfirm for Node %s - Invalid Edge variadic argument"), *GetNameSafe(CurrentNode))
		return;	
	}

	const AActor* OwningActor = GetAvatarActorFromActorInfo();
	if (!OwningActor)
	{
		CG_RUNTIME_LOG(Error, TEXT("ReceivedInputConfirm %s with InputAction: %s but owning actor is not set"), *GetName(), *GetNameSafe(Edge->TransitionInput))
		return;
	}
	
	CG_RUNTIME_LOG(VeryVerbose, TEXT("ReceivedInputConfirm for Node %s - OwningActor: %s (Window Opened: %s)"), *GetNameSafe(CurrentNode), *GetNameSafe(OwningActor), IsComboWindowOpened() ? TEXT("true") : TEXT("false"))
	
	// Only consider if combo window is currently active / opened (except cancel trigger event, we always want to transition regardless of combo window)
	if (!Edge->IsUsingCanceledTriggerEvent() && !IsComboWindowOpened())
	{
		return;
	}

	// Bookkeeping triggers as we only want to transition once, and default behavior with no triggers (or just the down one) will trigger every frame
	if (Edge->IsConfirmed())
	{
		return;
	}

	CG_RUNTIME_LOG(Verbose, TEXT("\t ReceivedInputConfirm for Node %s - OwningActor: %s - Confirmed"), *GetNameSafe(CurrentNode), *GetNameSafe(OwningActor))

	// Mark this edge as "confirmed", preventing further confirmation if Enhanced Input callback is triggered multiple times
	Edge->SetIsConfirmed(true);

	// Update value of replicated edge enhanced input action instance
	Edge->SetCurrentInputActionInstance(InputActionInstance);

	const FGameplayTag Tag = FComboGraphNativeTags::Get().Input;
	FGameplayEventData EventData;
	EventData.OptionalObject = Edge->TransitionInput;

	// We only need the trigger event type, consider using either instigator / target tags to hold this info, or even EventTag.
	EventData.OptionalObject2 = Edge;

	CG_RUNTIME_LOG(
		Verbose,
		TEXT("UComboGraphEdge::ReceivedInputConfirm %s (%d) - %s for %s.\nSend Event %s. Behavior: %s, TriggerEvent: %s"),
		*GetName(),
		GetUniqueID(),
		*GetNameSafe(Edge->TransitionInput),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*Tag.ToString(),
		*UEnum::GetValueAsString(Edge->TransitionBehavior),
		*UEnum::GetValueAsString(Edge->TriggerEvent)
	)

	// TODO: Rework sending client input data via TargetData (instead of replicated gameplay event) - see OnEventInputReceived
	UComboGraphGameplayTasksComponent* Component = OwningActor->FindComponentByClass<UComboGraphGameplayTasksComponent>();
	if (Component)
	{
		Component->SendGameplayEventReplicated(Tag, EventData);
	}
}

void UComboGraphAbilityTask_StartGraph::ClearInputBindings()
{
	for (const uint32 InputHandle : InputHandles)
	{
		if (InputComponent)
		{
			InputComponent->RemoveBindingByHandle(InputHandle);
		}
	}

	InputHandles.Empty();
}

// ReSharper disable once CppPassValueParameterByConstReference
void UComboGraphAbilityTask_StartGraph::OnEventInputReceived(const FGameplayEventData EventData)
{
	// TODO: Rework sending client input data via TargetData (instead of replicated gameplay event) - see ReceivedInputConfirm
	const FGameplayTag Tag = EventData.EventTag;
	const UInputAction* InputAction = Cast<UInputAction>(EventData.OptionalObject);
	const UComboGraphEdge* InputEdge = Cast<UComboGraphEdge>(EventData.OptionalObject2);

	if (!InputAction)
	{
		CG_RUNTIME_LOG(Error, TEXT("OnEventInputReceived: InputAction from Event Data is invalid"))
		return;
	}

	// Handle InputEdge invalid, most likely when using SimulateComboInput
	const EComboGraphTransitionInputEvent TriggerEvent = InputEdge ? InputEdge->TriggerEvent : EComboGraphTransitionInputEvent::Triggered;

	const FString LogPrefix = FComboGraphUtils::GetWorldLogPrefix(GetWorld());
	CG_RUNTIME_LOG(Verbose, TEXT("===> [%s] OnEventInputReceived: %s InputAction: %s, Edge: %s"), *LogPrefix, *Tag.ToString(), *GetNameSafe(InputAction), *GetNameSafe(InputEdge))

	// Get the child node from current node that has the TransitionEvent the same as the event tag
	for (UComboGraphNodeBase* ChildNode : CurrentNode->ChildrenNodes)
	{
		if (UComboGraphNodeAnimBase* ChildAnimNode = Cast<UComboGraphNodeAnimBase>(ChildNode))
		{
			const UComboGraphEdge* Edge = CurrentNode->GetEdge(ChildAnimNode);
			CG_RUNTIME_LOG(Verbose, TEXT("OnEventInputReceived: Found edge %s"), *GetNameSafe(Edge))
			if (Edge && Edge->TransitionInput == InputAction && Edge->TriggerEvent == TriggerEvent)
			{
				CG_RUNTIME_LOG(Verbose, TEXT("OnEventInputReceived: Handle input confirm for edge and tag %s (Child: %s)"), *Tag.ToString(), *ChildAnimNode->GetName())

				if (InputEdge)
				{
					// Update local value here of current Enhanced Input action instance for use in GetNodeMagnitudeForContainer
					CurrentInputActionInstance = InputEdge->GetCurrentInputActionInstance();
				}
				
				HandleInputConfirmed(ChildAnimNode, Edge);
				break;
			}
		}
	}
}

void UComboGraphAbilityTask_StartGraph::HandleInputConfirmed(UComboGraphNodeAnimBase* NextNode, const UComboGraphEdge* Edge)
{
	check(Ability);

	if (!Edge)
	{
		CG_RUNTIME_LOG(Error, TEXT("HandleInputConfirmed: Provided Edge is invalid"))
		return;
	}

	if (!Edge->IsUsingCanceledTriggerEvent() && !IsComboWindowOpened())
	{
		CG_RUNTIME_LOG(Verbose, TEXT("HandleInputConfirmed: Montage has notify and Combo window closed, prevent queue input"))
		return;
	}

	bComboQueued = true;
	QueuedNode = NextNode;
	CG_RUNTIME_LOG(
		Log,
		TEXT("HandleInputConfirmed: Set combo queued: %s (Node: %s)"),
		bComboQueued ? TEXT("true") : TEXT("false"),
		QueuedNode ? *QueuedNode->GetName() : TEXT("NONE")
	)

	if (CurrentNode && NextNode)
	{
		if (Edge->TransitionBehavior == EComboGraphTransitionBehavior::Immediately)
		{
			CG_RUNTIME_LOG(Log, TEXT("HandleInputConfirmed: HandleComboTransition immediately (Edge %s, Node: %s)"), *GetNameSafe(Edge), *GetNameSafe(QueuedNode))
			HandleComboTransition();
		}
		else if (Edge->TransitionBehavior == EComboGraphTransitionBehavior::OnAnimNotifyClass && !Edge->TransitionAnimNotify.IsNull())
		{
			const TSubclassOf<UAnimNotify> NotifyClass = Edge->TransitionAnimNotify.LoadSynchronous();
			const float TriggerTime = CurrentNode->GetAnimNotifyTriggerTime(NotifyClass);
			CG_RUNTIME_LOG(Verbose, TEXT("HandleInputConfirmed OnAnimNotifyClass %s %f"), *GetNameSafe(NotifyClass), TriggerTime)
			if (TriggerTime > 0.f)
			{
				HandleComboTransitionForNotifyTriggerTime(TriggerTime);
			}
		}
		else if (Edge->TransitionBehavior == EComboGraphTransitionBehavior::OnAnimNotifyName && !Edge->TransitionAnimNotifyName.IsNone())
		{
			const float TriggerTime = CurrentNode->GetAnimNotifyTriggerTime(Edge->TransitionAnimNotifyName);
			CG_RUNTIME_LOG(Verbose, TEXT("HandleInputConfirmed OnAnimNotifyName %s %f"), *Edge->TransitionAnimNotifyName.ToString(), TriggerTime)
			if (TriggerTime > 0.f)
			{
				HandleComboTransitionForNotifyTriggerTime(TriggerTime);
			}
		}
	}
}

void UComboGraphAbilityTask_StartGraph::CreatePlayMontageTask(UAnimMontage* Montage)
{
	check(Ability);

	if (!Montage)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphAbilityTask::CreatePlayMontageTask - Montage is invalid"));
		EndGraph();
		return;
	}

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask::CreatePlayMontageTask - Node: %s - Montage: %s - Index: %d"), *CurrentNode->GetName(), Montage ? *Montage->GetName() : TEXT("NONE"), NodeIndex)

	FGameplayTagContainer EventTags = FGameplayTagContainer(CurrentNode->EventTags);

	// Check effect containers event setup and add those
	TArray<FGameplayTag> EffectContainerTags;
	if (CurrentNode->DamageHandlingMethod == EComboGraphDamageHandlingMethod::AbilitySystem)
	{
		CurrentNode->EffectsContainerMap.GetKeys(EffectContainerTags);
	}
	else if (CurrentNode->DamageHandlingMethod == EComboGraphDamageHandlingMethod::DamageSystem)
	{
		CurrentNode->DamagesContainerMap.GetKeys(EffectContainerTags);
	}
	EventTags.AppendTags(FGameplayTagContainer::CreateFromArray(EffectContainerTags));

	// Same for Cues Containers
	TArray<FGameplayTag> CueContainerTags;
	CurrentNode->CuesContainerMap.GetKeys(CueContainerTags);
	EventTags.AppendTags(FGameplayTagContainer::CreateFromArray(CueContainerTags));

	EventTags.AddTag(FComboGraphNativeTags::Get().ComboBegin);
	EventTags.AddTag(FComboGraphNativeTags::Get().ComboEnd);

	CG_RUNTIME_LOG(Log, TEXT("UComboGraphAbilityTask::CreatePlayMontageTask - Event Tags: %s"), *EventTags.ToString())
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphAbilityTask::CreatePlayMontageTask - Play Montage: %s (%s | %s)"),
		*Montage->GetName(),
		IsLocallyControlled() ? TEXT("locally controlled") : TEXT("remotely controllled"),
		Ability->K2_HasAuthority() ? TEXT("Authority") : TEXT("Remote")
	)

	// Build the list of state auto setups based on Project's Setting Map and Node one
	TMap<TSoftClassPtr<UAnimNotifyState>, FComboGraphNotifyStateAutoSetup> NotifiesAutoSetup = FComboGraphUtils::GetPluginProjectSettings()->NotifyStates;
	NotifiesAutoSetup.Append(CurrentNode->NotifyStatesOverrides);

	if (RunningTask)
	{
		// End and cleanup previous montage task (mostly to prevent from being triggered OnMontageInterrupted when we transition from one combo montage
		// to the next in the same graph, while still being able to handle ending graph if we're interrupted / cancelled from outside sources or abilities)
		RunningTask->UnbindAllDelegate();
		RunningTask->EndTask();
	}

	FName StartSection = NAME_None;
	if (const UComboGraphNodeMontage* NodeMontage = Cast<UComboGraphNodeMontage>(CurrentNode))
	{
		StartSection = NodeMontage->GetMontageStartSection();
	}

	UComboGraphAbilityTask_PlayMontage* Task = UComboGraphAbilityTask_PlayMontage::PlayComboGraphMontage(
		Ability,
		NAME_None,
		Montage,
		NotifiesAutoSetup,
		EventTags,
		CurrentNode->MontagePlayRate,
		StartSection,
		CurrentNode->bStopAnimationWhenAbilityEnds,
		CurrentNode->RootMotionScale
	);

	Task->OnBlendOut.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::OnMontageBlendOut);
	Task->OnCompleted.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::OnMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::OnMontageCancelled);
	Task->EventReceived.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::HandleEventReceived);
	Task->OnTriggerTransition.AddUObject(this, &UComboGraphAbilityTask_StartGraph::HandleTriggerTransitionEvent);

	RunningTask = Task;

	Task->ReadyForActivation();
}

void UComboGraphAbilityTask_StartGraph::EndGraph()
{
	if (CurrentNode)
	{
		CurrentNode->OnGameplayTaskDeactivated(*this);
	}

	CG_RUNTIME_LOG(Log, TEXT("UComboGraphAbilityTask_StartGraph ... Ending graph. CurrentNode: %s (%s)"), *GetNameSafe(CurrentNode), CurrentNode ? *CurrentNode->GetNodeTitle().ToString() : TEXT("NONE"))

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FComboGraphDelegates::OnComboGraphEnded.Broadcast(*this, *RunningGraph);
		OnGraphEnd.Broadcast(FGameplayTag(), FGameplayEventData());
	}

	EndTask();
}

void UComboGraphAbilityTask_StartGraph::OnMontageCompleted(const FGameplayTag EventTag, FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("OnMontageCompleted - Tag: %s"), *EventTag.GetTagName().ToString())
	EndGraph();
}

void UComboGraphAbilityTask_StartGraph::OnMontageBlendOut(const FGameplayTag EventTag, FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("OnMontageBlendOut - Tag: %s"), *EventTag.GetTagName().ToString())
	ClearInputBindings();
}

void UComboGraphAbilityTask_StartGraph::OnMontageCancelled(const FGameplayTag EventTag, FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("OnMontageCancelled - Tag: %s"), *EventTag.GetTagName().ToString())
	EndGraph();
}

void UComboGraphAbilityTask_StartGraph::OnMontageInterrupted(const FGameplayTag EventTag, FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("OnMontageInterrupted - Tag: %s"), *EventTag.GetTagName().ToString())
	EndGraph();
}

void UComboGraphAbilityTask_StartGraph::HandleEventReceived(const FGameplayTag EventTag, const FGameplayEventData EventData)
{
	CG_RUNTIME_LOG(Log, TEXT("%sUComboGraphAbilityTask::EventReceived - Event Tags: %s"), *FComboGraphUtils::GetWorldLogPrefix(GetWorld()), *EventTag.ToString())

	const FGameplayTag ComboBegin = FComboGraphNativeTags::Get().ComboBegin;
	const FGameplayTag ComboEnd = FComboGraphNativeTags::Get().ComboEnd;

	if (EventTag == ComboBegin)
	{
		// Broadcast every event received even internal combo window events (sent from notify)
		if (ShouldBroadcastInternalEvents())
		{
			// Broadcast task delegate
			EventReceived.Broadcast(EventTag, EventData);

			// Broadcast BP specific Combo Node event
			CurrentNode->OnEventReceived(EventTag, EventData);
		}

		HandleComboBeginEvent();
	}
	else if(EventTag == ComboEnd)
	{
		// Broadcast every event received even internal combo window events (sent from notify)
		if (ShouldBroadcastInternalEvents())
		{
			// Broadcast task delegate
			EventReceived.Broadcast(EventTag, EventData);

			// Broadcast BP specific Combo Node event
			CurrentNode->OnEventReceived(EventTag, EventData);
		}

		HandleComboEndEvent();
	}
	else
	{
		if (!CurrentNode)
		{
			CG_RUNTIME_LOG(Error, TEXT("EventReceived: Cannot apply effect container, current node is invalid"))
			return;
		}

		// Broadcast every event received that are not internal combo window events (sent from notify)
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			// Broadcast task delegate
			EventReceived.Broadcast(EventTag, EventData);

			// Broadcast BP specific Combo Node event
			CurrentNode->OnEventReceived(EventTag, EventData);
		}
		
		// Make sure Ability is still valid, both effect / damage and cue containers application rely on it
		// Prevent the container application in case it's null, and prevent further assertions check (and crashes)
		//
		// Reason this might happen is in the EventReceived event broadcasted just above, where users could manually invoke K2_EndAbility()
		if (Ability)
		{
			CG_RUNTIME_LOG(Log, TEXT("EventReceived: Apply effect or damage container for tag %s"), *EventTag.ToString())
			ApplyEffectOrDamageContainer(CurrentNode, EventTag, EventData);

			CG_RUNTIME_LOG(Log, TEXT("EventReceived: Apply cues container for tag %s"), *EventTag.ToString())
			CurrentNode->ApplyCuesContainerForTag(EventTag, EventData);
		}
	}
}

void UComboGraphAbilityTask_StartGraph::HandleTriggerTransitionEvent()
{
	CG_RUNTIME_LOG(Verbose, TEXT("HandleTriggerTransitionEvent ..."))
	HandleComboTransition();
}

void UComboGraphAbilityTask_StartGraph::OnServerSyncAdvanceNextNode()
{
	// Update prev node pointer here
	PreviousNode = CurrentNode;

	FString FailReason;
	if (!AdvanceNextNode(QueuedNode, FailReason))
	{
		CG_RUNTIME_LOG(Warning, TEXT("OnServerSyncAdvanceNextNode ... Advance Next node failed: %s"), *FailReason)
		EndGraph();
	}
}

void UComboGraphAbilityTask_StartGraph::HandleComboBeginEvent()
{
	bComboWindowOpened = true;
	CG_RUNTIME_LOG(Log, TEXT("EventReceived - HandleComboBeginEvent ... Open Combo Window: %s"), bComboWindowOpened ? TEXT("true") : TEXT("false"))
}

void UComboGraphAbilityTask_StartGraph::HandleComboEndEvent()
{
	bComboWindowOpened = false;
	CG_RUNTIME_LOG(Log, TEXT("EventReceived - HandleComboEndEvent ... Close Combo Window: %s (Queued node: %s)"), *GetBoolText(bComboWindowOpened), *GetNameSafe(QueuedNode))

	if (bComboQueued && CurrentNode && QueuedNode)
	{
		const UComboGraphEdge* Edge = CurrentNode->GetEdge(QueuedNode);
		if (Edge->TransitionBehavior == EComboGraphTransitionBehavior::OnComboWindowEnd)
		{
			HandleComboTransition();
		}
	}
}

void UComboGraphAbilityTask_StartGraph::HandleComboTransition()
{
	UComboGraphAbilityTask_NetworkSyncPoint* Task = UComboGraphAbilityTask_NetworkSyncPoint::WaitNetSync(Ability, EComboGraphAbilityTaskNetSyncType::OnlyServerWait);
	// Wait for execution synchronization and only advance next node when server is ready
	Task->OnSync.AddDynamic(this, &UComboGraphAbilityTask_StartGraph::OnServerSyncAdvanceNextNode);
	Task->ReadyForActivation();
}

void UComboGraphAbilityTask_StartGraph::HandleComboTransitionForNotifyTriggerTime(const float TriggerTime)
{
	// Early out if no running play montage task
	if (!RunningTask)
	{
		return;
	}

	// Early out if Trigger Time couldn't be determined
	if (TriggerTime < 0.f)
	{
		return;
	}

	const float ElapsedTime = RunningTask->GetElapsedTime();
	// We are passed the trigger time for the notify, meaning we received input after is has been triggered
	if (ElapsedTime >= TriggerTime)
	{
		HandleComboTransition();
	}
	// We have not yet reached trigger time for the notify, wait for the specific trigger time to transition
	else
	{
		// Setting up trigger time will make play montage task check for it, and test if elapsed time has reached it, triggering the appropriate transition delegate
		RunningTask->SetTransitionTriggerTime(TriggerTime);
	}
}

FComboGraphGameplayEffectContainerSpec UComboGraphAbilityTask_StartGraph::MakeEffectContainerSpecFromContainer(const FComboGraphGameplayEffectContainer& Container, FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	check(Ability);
	check(CurrentNode);

	// First figure out our actor info
	FComboGraphGameplayEffectContainerSpec ReturnSpec;
	const UAbilitySystemComponent* OwningASC = Ability->GetAbilitySystemComponentFromActorInfo();

	if (OwningASC)
	{
		const TSubclassOf<UComboGraphTargetType> TargetType = UComboGraphTargetType::StaticClass();
		const UComboGraphTargetType* TargetTypeCDO = TargetType.GetDefaultObject();

		TArray<AActor*> TargetActors;
		TArray<FHitResult> HitResults;
		AActor* AvatarActor = GetAvatarActorFromActorInfo();
		TargetTypeCDO->GetTargets(AvatarActor, EventData, HitResults, TargetActors);
		ReturnSpec.AddTargets(HitResults, TargetActors);

		// If we don't have an override level, use the default on the ability itself
		if (OverrideGameplayLevel == INDEX_NONE)
		{
			OverrideGameplayLevel = Ability->GetAbilityLevel();
		}

		// Build GameplayEffectSpecs for each applied effect
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Container.TargetGameplayEffectClasses)
		{
			FGameplayEffectSpecHandle SpecHandle = Ability->MakeOutgoingGameplayEffectSpec(EffectClass, OverrideGameplayLevel);

			FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
			if (Spec && Container.bUseSetByCallerMagnitude)
			{
				// Pass in to BP overridable to let users adjust magnitude if desired
				const float Magnitude = GetNodeMagnitudeForContainer(CurrentNode, Container.SetByCallerMagnitude, ContainerTag);
				Spec->SetSetByCallerMagnitude(Container.SetByCallerDataTag, Magnitude);
			}
			ReturnSpec.TargetGameplayEffectSpecs.Add(SpecHandle);
		}
	}
	return ReturnSpec;
}

FComboGraphGameplayEffectContainerSpec UComboGraphAbilityTask_StartGraph::MakeEffectContainerSpec(TMap<FGameplayTag, FComboGraphGameplayEffectContainer> EffectContainerMap, const FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	const FComboGraphGameplayEffectContainer* FoundContainer = EffectContainerMap.Find(ContainerTag);
	if (FoundContainer)
	{
		return MakeEffectContainerSpecFromContainer(*FoundContainer, ContainerTag, EventData, OverrideGameplayLevel);
	}

	return FComboGraphGameplayEffectContainerSpec();
}

TArray<FActiveGameplayEffectHandle> UComboGraphAbilityTask_StartGraph::ApplyEffectContainerSpec(const FComboGraphGameplayEffectContainerSpec& ContainerSpec)
{
	check(Ability);

	TArray<FActiveGameplayEffectHandle> AllEffects;

	const FGameplayAbilitySpecHandle CurrentSpecHandle = Ability->GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* CurrentActorInfo = Ability->GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo CurrentActivationInfo = Ability->GetCurrentActivationInfo();

	// Iterate list of effect specs and apply them to their target data
	for (const FGameplayEffectSpecHandle& SpecHandle : ContainerSpec.TargetGameplayEffectSpecs)
	{
		// Check that owner can apply effects, since combo graph could be run via gameplay task on non IAbilitySystemInterface owners (won't be relevant in ue5 anymore)
		const FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
		const bool bCanApplyEffects = Spec->GetContext().IsValid() && Spec->GetContext().GetInstigatorAbilitySystemComponent();
		if (!bCanApplyEffects)
		{
			CG_RUNTIME_SLOG(Error, TEXT("Combo Graph: Trying to apply Gameplay Effects container from Source without an Ability System Component (doesn't implement IAbilitySystemInterface). Use Unreal's Damage System instead."))
			break;
		}

		FString FailReason;
		if (!CanApplyEffectsToTargets(ContainerSpec.TargetData, FailReason))
		{
			CG_RUNTIME_SLOG(Error, TEXT("%s"), *FailReason)
			break;
		}

		AllEffects.Append(ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle, ContainerSpec.TargetData));
	}

	return AllEffects;
}

TArray<FActiveGameplayEffectHandle> UComboGraphAbilityTask_StartGraph::ApplyGameplayEffectSpecToTarget(const FGameplayAbilitySpecHandle AbilityHandle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEffectSpecHandle SpecHandle, const FGameplayAbilityTargetDataHandle& TargetData) const
{
	check(Ability);

	TArray<FActiveGameplayEffectHandle> EffectHandles;

	if (SpecHandle.IsValid() && Ability->HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		for (TSharedPtr<FGameplayAbilityTargetData> Data : TargetData.Data)
		{
			if (Data.IsValid())
			{
				EffectHandles.Append(Data->ApplyGameplayEffectSpec(*SpecHandle.Data.Get(), ActorInfo->AbilitySystemComponent->GetPredictionKeyForNewAction()));
			}
			else
			{
				CG_RUNTIME_LOG(Warning, TEXT("UGameplayAbility::ApplyGameplayEffectSpecToTarget invalid target data passed in. Ability: %s"), *GetNameSafe(Ability));
			}
		}
	}
	return EffectHandles;
}

TArray<FActiveGameplayEffectHandle> UComboGraphAbilityTask_StartGraph::ApplyEffectContainer(TMap<FGameplayTag, FComboGraphGameplayEffectContainer> EffectContainerMap, FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	const FComboGraphGameplayEffectContainerSpec Spec = MakeEffectContainerSpec(EffectContainerMap, ContainerTag, EventData, OverrideGameplayLevel);
	return ApplyEffectContainerSpec(Spec);
}

void UComboGraphAbilityTask_StartGraph::ApplyEffectOrDamageContainer(UComboGraphNodeAnimBase* ComboNode, FGameplayTag EventTag, FGameplayEventData EventData)
{
	const EComboGraphDamageHandlingMethod DamageMethod = ComboNode->DamageHandlingMethod;
	if (DamageMethod == EComboGraphDamageHandlingMethod::AbilitySystem)
	{
		ApplyEffectContainer(ComboNode->EffectsContainerMap, EventTag, EventData);
	}
	else if (DamageMethod == EComboGraphDamageHandlingMethod::DamageSystem)
	{
		ApplyDamageContainer(ComboNode, EventTag, EventData);
	}
}

void UComboGraphAbilityTask_StartGraph::ApplyDamageContainer(UComboGraphNodeAnimBase* ComboNode, FGameplayTag EventTag, FGameplayEventData EventData) const
{
	check(Ability);

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_StartGraph::ApplyDamageContainer (Tag: %s)"), *EventTag.ToString())

	if (!Ability->K2_HasAuthority())
	{
		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_StartGraph::ApplyDamageContainer No Authority do nothing"))
		return;
	}

	// Make Effect Container Spec
	FComboGraphDamageSystemContainer* FoundContainer = ComboNode->DamagesContainerMap.Find(EventTag);
	if (FoundContainer)
	{
		float BaseDamage = GetNodeMagnitudeForContainer(ComboNode, FoundContainer->BaseDamage, EventTag);
		TSubclassOf<UDamageType> DamageTypeClass = FoundContainer->DamageTypeClass;

		AActor* DamageCauser = GetAvatarActorFromActorInfo();
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		if (IsValid(EventData.Instigator.Get()))
		{
			DamageCauser = const_cast<AActor*>(EventData.Instigator.Get());
		}
#else
		if (IsValid(EventData.Instigator))
		{
			DamageCauser = const_cast<AActor*>(EventData.Instigator);
		}
#endif	

		UComboGraphTargetType* TargetType = UComboGraphTargetType::StaticClass()->GetDefaultObject<UComboGraphTargetType>();
		TArray<AActor*> TargetActors;
		TArray<FHitResult> HitResults;
		TargetType->GetTargets(DamageCauser, EventData, HitResults, TargetActors);

		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_StartGraph::ApplyDamageContainer from %s (Target Actors: %d, Hit Results: %d)"), *GetNameSafe(DamageCauser), TargetActors.Num(), HitResults.Num())

		for (FHitResult HitResult : HitResults)
		{
#if ENGINE_MAJOR_VERSION == 5
			AActor* TargetActor = HitResult.GetActor();
#else
			AActor* TargetActor = HitResult.Actor.IsValid() ? HitResult.GetActor() : nullptr;
#endif
			CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_StartGraph::ApplyDamageContainer to %s (Damage: %f, Type: %s)"), *GetNameSafe(TargetActor), BaseDamage, *GetNameSafe(DamageTypeClass))

			APlayerController* Instigator = GetPlayerControllerFromActorInfo();
			float AppliedDamage = UGameplayStatics::ApplyPointDamage(TargetActor, BaseDamage, HitResult.Normal, HitResult, Instigator, DamageCauser, DamageTypeClass);
			CG_RUNTIME_LOG(Verbose, TEXT("%sUComboGraphAbilityTask_StartGraph::ApplyDamageContainer Damage Applied: %f"), *FComboGraphUtils::GetWorldLogPrefix(GetWorld()), AppliedDamage)
		}
	}
}

bool UComboGraphAbilityTask_StartGraph::CanApplyEffectsToTargets(const FGameplayAbilityTargetDataHandle TargetDataHandle, FString& FailReason) const
{
	for (const TSharedPtr<FGameplayAbilityTargetData>& TargetData : TargetDataHandle.Data)
	{
		TArray<TWeakObjectPtr<AActor>> Actors = TargetData->GetActors();

		for (TWeakObjectPtr<AActor>& TargetActor : Actors)
		{
			const UAbilitySystemComponent* TargetComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());
			if (!TargetComponent)
			{
				FailReason = FString::Printf(
					TEXT("Combo Graph: Trying to apply Gameplay Effects container to target %s without an Ability System Component (doesn't implement IAbilitySystemInterface). Use Unreal's Damage System instead."),
					*GetNameSafe(TargetActor.Get())
				);
				return false;
			}
		}
	}

	return true;
}


bool UComboGraphAbilityTask_StartGraph::IsStandalone() const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	check(AbilitySystemComponent.IsValid());
#else
	check(AbilitySystemComponent);
#endif	
	return AbilitySystemComponent->GetNetMode() == NM_Standalone;
}

FActiveGameplayEffectHandle UComboGraphAbilityTask_StartGraph::ApplyGameplayEffectToOwner(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const UGameplayEffect* GameplayEffect, float GameplayEffectLevel, int32 Stacks) const
{
	check(Ability);

	if (GameplayEffect && (Ability->HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo)))
	{
		const FGameplayEffectSpecHandle SpecHandle = Ability->MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, GameplayEffect->GetClass(), GameplayEffectLevel);
		if (SpecHandle.IsValid())
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			SpecHandle.Data->SetStackCount(Stacks);
#else
			SpecHandle.Data->StackCount = Stacks;
#endif
			return ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	// We cannot apply GameplayEffects in this context. Return an empty handle.
	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UComboGraphAbilityTask_StartGraph::ApplyGameplayEffectSpecToOwner(const FGameplayAbilitySpecHandle AbilityHandle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEffectSpecHandle SpecHandle) const
{
	check(Ability);

	// This batches all created cues together
	FScopedGameplayCueSendContext GameplayCueSendContext;

	if (SpecHandle.IsValid() && (Ability->HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo)))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get(), ASC->GetPredictionKeyForNewAction());
	}

	return FActiveGameplayEffectHandle();
}

bool UComboGraphAbilityTask_StartGraph::CanApplyAttributeModifiers(const UGameplayEffect* GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext) const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	check(AbilitySystemComponent.IsValid());
#else
	check(AbilitySystemComponent);
#endif	

	FGameplayEffectSpec	Spec(GameplayEffect, EffectContext, Level);

	Spec.CalculateModifierMagnitudes();

	for(int32 ModIdx = 0; ModIdx < Spec.Modifiers.Num(); ++ModIdx)
	{
		const FGameplayModifierInfo& ModDef = Spec.Def->Modifiers[ModIdx];
		const FModifierSpec& ModSpec = Spec.Modifiers[ModIdx];

		// It only makes sense to check additive operators
		if (ModDef.ModifierOp == EGameplayModOp::Additive)
		{
			if (!ModDef.Attribute.IsValid())
			{
				continue;
			}

			if (!AbilitySystemComponent->HasAttributeSetForAttribute(ModDef.Attribute))
			{
				// Should we log / warn users there ?
				continue;
			}

			const UAttributeSet* Set = GetAttributeSubobject(ModDef.Attribute.GetAttributeSetClass());
			const float CurrentValue = ModDef.Attribute.GetNumericValueChecked(Set);
			const float CostValue = ModSpec.GetEvaluatedMagnitude();

			// TODO: Option to allow value to be negative and prevent activation if it is 0 or below 0
			// if (CurrentValue <= 0.f)
			// {
			// 	return false;
			// }

			if (CurrentValue + CostValue < 0.f)
			{
				return false;
			}
		}
	}

	return true;


}

const UAttributeSet* UComboGraphAbilityTask_StartGraph::GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	check(AbilitySystemComponent.IsValid());
#else
	check(AbilitySystemComponent);
#endif	

	for (const UAttributeSet* Set : AbilitySystemComponent->GetSpawnedAttributes())
	{
		if (Set && Set->IsA(AttributeClass))
		{
			return Set;
		}
	}

	return nullptr;
}

bool UComboGraphAbilityTask_StartGraph::ShouldBroadcastInternalEvents() const
{
	return bBroadcastInternalEvents && ShouldBroadcastAbilityTaskDelegates();
}

float UComboGraphAbilityTask_StartGraph::GetNodeMagnitudeForContainer(const UComboGraphNodeAnimBase* ComboNode, const float OriginalMagnitude, const FGameplayTag ContainerTag) const
{
	check(ComboNode);

	const float ElapsedTime = CurrentInputActionInstance.GetElapsedTime();
	const float TriggeredTime = CurrentInputActionInstance.GetTriggeredTime();
	const float AnimationLength = CurrentNode->GetAnimationLengthMinusStartSection();

	const UAnimInstance* AnimInstance = GetAnimInstanceFromActorInfo();

	float MontagePlayTime = 0.f;
	if (AnimInstance)
	{
		const FAnimMontageInstance* AnimMontageInstance = AnimInstance->GetActiveMontageInstance();
		MontagePlayTime = AnimMontageInstance ? AnimMontageInstance->GetPosition() : -1.f;
	}

	// Compute normalized input magnitude ranging from 0 to 1 based on Enhanced Input Elapsed Time and Animation Length
	const float InputMagnitude = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, AnimationLength),
		FVector2D(0, 1),
		ElapsedTime != 0 ? ElapsedTime : AnimationLength
	);

	// Pass in to BP overridable method to let users adjust magnitude if desired
	const float Magnitude = CurrentNode->GetMagnitudeForContainer(
		OriginalMagnitude,
		ElapsedTime,
		TriggeredTime,
		AnimationLength,
		MontagePlayTime,
		InputMagnitude,
		ContainerTag
	);

	return Magnitude;
}
