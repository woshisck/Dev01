#include "BuffFlow/Nodes/BFNode_IncrementPhase.h"

UBFNode_IncrementPhase::UBFNode_IncrementPhase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_IncrementPhase::ExecuteInput(const FName& PinName)
{
    TriggerOutput(TEXT("Out"), true);
}
