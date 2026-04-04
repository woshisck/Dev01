#include "BuffFlow/Nodes/BFNode_OnBuffAdded.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_OnBuffAdded::UBFNode_OnBuffAdded(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnAdded")) };
}

void UBFNode_OnBuffAdded::ExecuteInput(const FName& PinName)
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
			BFC->OnBuffFlowStarted.AddDynamic(this, &UBFNode_OnBuffAdded::HandleBuffAdded);
			bBound = true;
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (bBound)
		{
			BFC->OnBuffFlowStarted.RemoveDynamic(this, &UBFNode_OnBuffAdded::HandleBuffAdded);
			bBound = false;
		}
	}
}

void UBFNode_OnBuffAdded::HandleBuffAdded(FGuid RuneGuid)
{
	TriggerOutput(TEXT("OnAdded"), false);
}

void UBFNode_OnBuffAdded::Cleanup()
{
	if (bBound)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			BFC->OnBuffFlowStarted.RemoveDynamic(this, &UBFNode_OnBuffAdded::HandleBuffAdded);
		}
		bBound = false;
	}

	Super::Cleanup();
}
