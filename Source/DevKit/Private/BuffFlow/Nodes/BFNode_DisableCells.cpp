#include "BuffFlow/Nodes/BFNode_DisableCells.h"
#include "Character/YogCharacterBase.h"
#include "Component/BackpackGridComponent.h"

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
	if (CellsToDisable.IsEmpty()) return;

	if (AYogCharacterBase* Owner = GetBuffOwner())
	{
		if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
		{
			BGC->DisableCells(CellsToDisable);
			UE_LOG(LogTemp, Log, TEXT("[BFNode_DisableCells] 禁用 %d 个格子"), CellsToDisable.Num());
		}
	}
}

void UBFNode_DisableCells::ApplyEnable()
{
	if (CellsToDisable.IsEmpty()) return;

	if (AYogCharacterBase* Owner = GetBuffOwner())
	{
		if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
		{
			BGC->EnableCells(CellsToDisable);
			UE_LOG(LogTemp, Log, TEXT("[BFNode_DisableCells] 恢复 %d 个格子"), CellsToDisable.Num());
		}
	}
}
