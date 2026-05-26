#include "Story/Flow/Nodes/SNode_SetRoomRewardOverride.h"

#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "System/YogGameInstanceBase.h"

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
	UYogGameInstanceBase* GI = World ? Cast<UYogGameInstanceBase>(World->GetGameInstance()) : nullptr;

	ApplyRewardOverride(GM, GI);

	TriggerOutput(TEXT("Out"), true);
}

bool USNode_SetRoomRewardOverride::ApplyRewardOverride(AYogGameMode* GameMode, UYogGameInstanceBase* GameInstance) const
{
	if (!bClearOverride && LootOptions.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetRoomRewardOverride] LootOptions is empty and bClearOverride is false; skipped."));
		return false;
	}

	if (OverrideTarget == EStoryRewardOverrideTarget::NextRoom)
	{
		if (!GameInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SNode_SetRoomRewardOverride] GameInstance not found for NextRoom reward override; skipped."));
			return false;
		}

		if (bClearOverride)
		{
			GameInstance->ClearPendingRoomRewardOptionsOverride();
		}
		else
		{
			GameInstance->SetPendingRoomRewardOptionsOverride(LootOptions);
		}

		return true;
	}

	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetRoomRewardOverride] AYogGameMode not found for CurrentRoom reward override; skipped."));
		return false;
	}

	if (bClearOverride)
	{
		GameMode->ClearRoomRewardOptionsOverride();
	}
	else
	{
		GameMode->SetRoomRewardOptionsOverride(LootOptions);
	}

	return true;
}
