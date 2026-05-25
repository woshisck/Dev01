#include "Story/Flow/Nodes/SNode_SetRoomRewardOverride.h"

#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"

USNode_SetRoomRewardOverride::USNode_SetRoomRewardOverride(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Level");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetRoomRewardOverride::ExecuteInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	AYogGameMode* GM = World ? Cast<AYogGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
	if (!GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetRoomRewardOverride] AYogGameMode not found — skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	if (bClearOverride)
	{
		GM->ClearRoomRewardOptionsOverride();
	}
	else if (!LootOptions.IsEmpty())
	{
		GM->SetRoomRewardOptionsOverride(LootOptions);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetRoomRewardOverride] LootOptions is empty and bClearOverride is false — skipped."));
	}

	TriggerOutput(TEXT("Out"), true);
}
