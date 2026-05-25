#include "Story/Flow/Nodes/SNode_SetPortalOverride.h"

#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"

USNode_SetPortalOverride::USNode_SetPortalOverride(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Level");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetPortalOverride::ExecuteInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	AYogGameMode* GM = World ? Cast<AYogGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
	if (!GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetPortalOverride] AYogGameMode not found — skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	if (bClearOverride)
	{
		GM->ClearForcedPortalOverride();
	}
	else
	{
		GM->SetForcedPortalOverride(PortalIndex);
	}

	TriggerOutput(TEXT("Out"), true);
}
