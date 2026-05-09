#include "BuffFlow/Nodes/BFNode_Fork.h"

UBFNode_Fork::UBFNode_Fork(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Flow");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	RebuildOutputPins();
}

void UBFNode_Fork::RebuildOutputPins()
{
	OutputPins.Reset();
	for (int32 i = 0; i < OutputCount; ++i)
	{
		OutputPins.Add(FFlowPin(FName(*FString::Printf(TEXT("Out %d"), i + 1))));
	}
}

void UBFNode_Fork::ExecuteInput(const FName& PinName)
{
	for (int32 i = 0; i < OutputCount; ++i)
	{
		const bool bFinish = (i == OutputCount - 1);
		TriggerOutput(FName(*FString::Printf(TEXT("Out %d"), i + 1)), bFinish);
	}
}

#if WITH_EDITOR
void UBFNode_Fork::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBFNode_Fork, OutputCount))
	{
		RebuildOutputPins();
	}
}
#endif
