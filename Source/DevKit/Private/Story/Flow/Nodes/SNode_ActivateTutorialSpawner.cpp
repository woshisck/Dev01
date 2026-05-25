#include "Story/Flow/Nodes/SNode_ActivateTutorialSpawner.h"

#include "EngineUtils.h"
#include "Map/TutorialMobSpawner.h"

USNode_ActivateTutorialSpawner::USNode_ActivateTutorialSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Tutorial");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_ActivateTutorialSpawner::ExecuteInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	if (World && !SpawnerActorTag.IsNone())
	{
		for (TActorIterator<ATutorialMobSpawner> It(World); It; ++It)
		{
			if (It->Tags.Contains(SpawnerActorTag))
			{
				It->Activate();
			}
		}
	}
	TriggerOutput(TEXT("Out"), true);
}
