#include "BuffFlow/Nodes/BFNode_DoOnce.h"

UBFNode_DoOnce::UBFNode_DoOnce(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|FlowControl");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Reset")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_DoOnce::ExecuteBuffFlowInput(const FName& PinName)
{
	if (PinName == TEXT("Reset"))
	{
		bHasTriggered = false;
		return;
	}

	// "In" pin
	if (!bHasTriggered)
	{
		bHasTriggered = true;
		TriggerOutput(TEXT("Out"), true);
	}
}

void UBFNode_DoOnce::Cleanup()
{
	bHasTriggered = bStartClosed;
	Super::Cleanup();
}
