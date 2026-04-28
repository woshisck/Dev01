#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"

#include "AbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"
#include "FlowTypes.h"

UBFNode_SendGameplayEvent::UBFNode_SendGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SendGameplayEvent::ExecuteInput(const FName& PinName)
{
	// 快速连击会导致同一 Flow 被重新启动（Abort 旧实例），此时节点不再 Active，直接退出
	if (GetActivationState() != EFlowNodeState::Active)
		return;

	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Failed: invalid EventTag"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Failed: Target resolved null for %s"),
			*EventTag.ToString());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Failed: Target(%s) has no ASC"),
			*GetNameSafe(TargetActor));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* InstigatorActor = ResolveTarget(Instigator);
	AActor* PayloadTargetActor = ResolveTarget(PayloadTarget);
	if (!PayloadTargetActor)
	{
		PayloadTargetActor = TargetActor;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Send Event Tag=%s -> Receiver=%s | PayloadTarget=%s | Instigator=%s"),
		*EventTag.ToString(),
		*GetNameSafe(TargetActor),
		*GetNameSafe(PayloadTargetActor),
		*GetNameSafe(InstigatorActor));

	float ResolvedMagnitude = Magnitude.Value;
	FFlowDataPinResult_Float PinResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_SendGameplayEvent, Magnitude));
	if (PinResult.Result == EFlowDataPinResolveResult::Success)
	{
		ResolvedMagnitude = PinResult.Value;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = InstigatorActor;
	EventData.Target = PayloadTargetActor;
	EventData.EventMagnitude = ResolvedMagnitude;

	// Finish this node before dispatching, because the event can synchronously restart this Flow.
	TriggerOutput(TEXT("Out"), true);
	TargetASC->HandleGameplayEvent(EventTag, &EventData);
}
