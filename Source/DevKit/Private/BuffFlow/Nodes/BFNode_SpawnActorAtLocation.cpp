#include "BuffFlow/Nodes/BFNode_SpawnActorAtLocation.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_SpawnActorAtLocation::UBFNode_SpawnActorAtLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Spawn");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_SpawnActorAtLocation::ExecuteInput(const FName& PinName)
{
	if (PinName != TEXT("In"))
	{
		return;
	}

	if (!ActorClass)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	UWorld* World = BFC ? BFC->GetWorld() : nullptr;
	if (!World)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	FVector SpawnLocation = LocationOffset;
	if (bUseKillLocation && BFC)
	{
		SpawnLocation = BFC->LastKillLocation + LocationOffset;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World->SpawnActor<AActor>(ActorClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	TriggerOutput(TEXT("Out"), true);
}
