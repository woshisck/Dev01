#include "BuffFlow/Nodes/BFNode_OnPhaseUpReady.h"

UBFNode_OnPhaseUpReady::UBFNode_OnPhaseUpReady(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("OnPhaseUp")) };
}

void UBFNode_OnPhaseUpReady::ExecuteInput(const FName& PinName)
{
}

void UBFNode_OnPhaseUpReady::Cleanup()
{
    Super::Cleanup();
}
