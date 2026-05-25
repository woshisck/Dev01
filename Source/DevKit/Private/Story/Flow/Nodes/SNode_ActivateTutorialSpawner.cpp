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
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_ActivateTutorialSpawner] No world; skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	if (SpawnerActorTag.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_ActivateTutorialSpawner] SpawnerActorTag is None; skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	int32 MatchedCount = 0;
	for (TActorIterator<ATutorialMobSpawner> It(World); It; ++It)
	{
		if (It->Tags.Contains(SpawnerActorTag))
		{
			++MatchedCount;
			UE_LOG(LogTemp, Log, TEXT("[SNode_ActivateTutorialSpawner] Activating %s with tag %s."),
				*GetNameSafe(*It),
				*SpawnerActorTag.ToString());
			It->Activate();
		}
	}

	if (MatchedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_ActivateTutorialSpawner] No ATutorialMobSpawner matched tag %s."),
			*SpawnerActorTag.ToString());
	}

	TriggerOutput(TEXT("Out"), true);
}
