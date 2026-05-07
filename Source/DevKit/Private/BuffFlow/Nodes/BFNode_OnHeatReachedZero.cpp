#include "BuffFlow/Nodes/BFNode_OnHeatReachedZero.h"

UBFNode_OnHeatReachedZero::UBFNode_OnHeatReachedZero(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("OnReachedZero")), FFlowPin(TEXT("OnAboveZero")) };
}

void UBFNode_OnHeatReachedZero::ExecuteInput(const FName& PinName)
{
}

void UBFNode_OnHeatReachedZero::Cleanup()
{
    Super::Cleanup();
}
