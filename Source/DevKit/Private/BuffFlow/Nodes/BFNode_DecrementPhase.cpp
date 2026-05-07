#include "BuffFlow/Nodes/BFNode_DecrementPhase.h"

UBFNode_DecrementPhase::UBFNode_DecrementPhase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_DecrementPhase::ExecuteInput(const FName& PinName)
{
    TriggerOutput(TEXT("Out"), true);
}
