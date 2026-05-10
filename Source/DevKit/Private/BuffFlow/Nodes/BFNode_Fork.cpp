#include "BuffFlow/Nodes/BFNode_Fork.h"

UBFNode_Fork::UBFNode_Fork(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Internal");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Out2")) };
}

void UBFNode_Fork::ExecuteInput(const FName& PinName)
{
	for (int32 Index = 0; Index < OutputPins.Num(); ++Index)
	{
		const bool bFinish = (Index == OutputPins.Num() - 1);
		TriggerOutput(OutputPins[Index].PinName, bFinish);
	}
}
