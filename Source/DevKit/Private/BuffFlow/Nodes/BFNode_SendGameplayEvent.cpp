#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

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
	if (!EventTag.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag  = EventTag;
	EventData.Instigator = ResolveTarget(Instigator);
	EventData.Target     = TargetActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, EventData);

	TriggerOutput(TEXT("Out"), true);
}
