#include "LevelFlow/Nodes/LENode_WaitForLootSelected.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"

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
	AYogGameMode* GM = Cast<AYogGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM)
	{
		TriggerOutput(TEXT("OnSelected"), true);
		return;
	}

	LootSelectedHandle = GM->OnLootSelected.AddLambda([this]()
	{
		TriggerOutput(TEXT("OnSelected"), true);
	});
}

void ULENode_WaitForLootSelected::Cleanup()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(
		UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->OnLootSelected.Remove(LootSelectedHandle);
	}
	LootSelectedHandle.Reset();
	Super::Cleanup();
}
