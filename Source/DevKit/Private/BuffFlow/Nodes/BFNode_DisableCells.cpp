#include "BuffFlow/Nodes/BFNode_DisableCells.h"
#include "Character/YogCharacterBase.h"

UBFNode_DisableCells::UBFNode_DisableCells(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Backpack");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_DisableCells::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("In"))
	{
		ApplyDisable();
		TriggerOutput(TEXT("Out"), true);
	}
	else if (PinName == TEXT("Stop"))
	{
		ApplyEnable();
	}
}

void UBFNode_DisableCells::Cleanup()
{
	ApplyEnable();
	Super::Cleanup();
}

void UBFNode_DisableCells::ApplyDisable()
{
}

void UBFNode_DisableCells::ApplyEnable()
{
}
