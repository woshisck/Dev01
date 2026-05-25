#include "Story/Flow/Nodes/SNode_EnablePortal.h"

#include "EngineUtils.h"
#include "Map/Portal.h"

USNode_EnablePortal::USNode_EnablePortal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Level");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_EnablePortal::ExecuteInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	const bool bFilterByTag = !PortalActorTag.IsNone();
	const bool bFilterByIndex = (PortalIndex >= 0);
	const bool bShouldOpen = !SelectedLevel.IsNone() && SelectedRoom != nullptr;

	for (TActorIterator<APortal> It(World); It; ++It)
	{
		APortal* Portal = *It;
		if (!Portal) continue;

		if (bFilterByTag)
		{
			if (!Portal->Tags.Contains(PortalActorTag)) continue;
		}
		else if (bFilterByIndex)
		{
			if (Portal->Index != PortalIndex) continue;
		}

		Portal->EnablePortal();

		if (bShouldOpen)
		{
			Portal->Open(SelectedLevel, SelectedRoom, {});
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
