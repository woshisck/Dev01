#include "BuffFlow/Nodes/BFNode_FinishBuff.h"
#include "FlowAsset.h"

UBFNode_FinishBuff::UBFNode_FinishBuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins = {};
}

void UBFNode_FinishBuff::ExecuteInput(const FName& PinName)
{
	// Finish the entire flow (terminal node)
	Finish();
}
