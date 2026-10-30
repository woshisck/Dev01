#include "LevelFlow/Nodes/LENode_WaitForLootSelected.h"
#include "System/YogLootSubsystem.h"

ULENode_WaitForLootSelected::ULENode_WaitForLootSelected(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("OnSelected")) };
}

void ULENode_WaitForLootSelected::ExecuteInput(const FName& PinName)
{
	UYogLootSubsystem* Loot = GetWorld() ? GetWorld()->GetSubsystem<UYogLootSubsystem>() : nullptr;
	if (!Loot)
	{
		TriggerOutput(TEXT("OnSelected"), true);
		return;
	}

	LootSelectedHandle = Loot->OnLootSelected.AddLambda([this]()
	{
		TriggerOutput(TEXT("OnSelected"), true);
	});
}

void ULENode_WaitForLootSelected::Cleanup()
{
	if (UYogLootSubsystem* Loot = GetWorld() ? GetWorld()->GetSubsystem<UYogLootSubsystem>() : nullptr)
	{
		Loot->OnLootSelected.Remove(LootSelectedHandle);
	}
	LootSelectedHandle.Reset();
	Super::Cleanup();
}
