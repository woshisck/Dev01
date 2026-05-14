#include "BuffFlow/Nodes/BFNode_SpawnGameplayCueAtLocation.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_SpawnGameplayCueAtLocation::UBFNode_SpawnGameplayCueAtLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Visual");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnGameplayCueAtLocation::ExecuteBuffFlowInput(const FName& PinName)
{
	if (!CueTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SpawnGameplayCueAtLocation] CueTag 无效"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UYogAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!OwnerASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SpawnGameplayCueAtLocation] OwnerASC 为 null"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FVector Location = LocationOffset;

	if (bUseKillLocation)
	{
		UBuffFlowComponent* BFC = GetBuffFlowComponent();
		if (BFC)
		{
			Location = BFC->LastKillLocation + LocationOffset;
		}
	}
	else
	{
		AActor* SourceActor = ResolveTarget(LocationSource);
		if (!SourceActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_SpawnGameplayCueAtLocation] LocationSource 解析为 null"));
			TriggerOutput(TEXT("Failed"), true);
			return;
		}
		Location = SourceActor->GetActorLocation() + LocationOffset;
	}

	FGameplayCueParameters Params;
	Params.Location = Location;
	Params.Normal = FVector::UpVector;
	OwnerASC->ExecuteGameplayCue(CueTag, Params);

	TriggerOutput(TEXT("Out"), true);
}
