#include "BuffFlow/Nodes/BFNode_SpawnGameplayCueOnActor.h"
#include "AbilitySystemComponent.h"

UBFNode_SpawnGameplayCueOnActor::UBFNode_SpawnGameplayCueOnActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Visual");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnGameplayCueOnActor::ExecuteInput(const FName& PinName)
{
	if (!CueTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SpawnGameplayCueOnActor] CueTag 无效"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SpawnGameplayCueOnActor] Target 解析为 null"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SpawnGameplayCueOnActor] Target(%s) 没有 ASC"),
			*TargetActor->GetName());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FGameplayCueParameters Params;
	Params.Location = TargetActor->GetActorLocation();
	Params.Normal = FVector::UpVector;
	TargetASC->ExecuteGameplayCue(CueTag, Params);

	TriggerOutput(TEXT("Out"), true);
}
