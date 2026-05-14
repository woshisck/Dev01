#include "BuffFlow/Nodes/BFNode_OnBuffRemoved.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_OnBuffRemoved::UBFNode_OnBuffRemoved(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnRemoved")) };
}

void UBFNode_OnBuffRemoved::ExecuteBuffFlowInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		return;
	}

	if (PinName == TEXT("In"))
	{
		if (!bBound)
		{
			BFC->OnBuffFlowStopped.AddDynamic(this, &UBFNode_OnBuffRemoved::HandleBuffRemoved);
			bBound = true;
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (bBound)
		{
			BFC->OnBuffFlowStopped.RemoveDynamic(this, &UBFNode_OnBuffRemoved::HandleBuffRemoved);
			bBound = false;
		}
	}
}

void UBFNode_OnBuffRemoved::HandleBuffRemoved(FGuid RuneGuid)
{
	TriggerOutput(TEXT("OnRemoved"), false);
}

void UBFNode_OnBuffRemoved::Cleanup()
{
	if (bBound)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			BFC->OnBuffFlowStopped.RemoveDynamic(this, &UBFNode_OnBuffRemoved::HandleBuffRemoved);
		}
		bBound = false;
	}

	Super::Cleanup();
}
