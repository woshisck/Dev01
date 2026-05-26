#include "Story/Flow/Nodes/SNode_ActivateTutorialSpawner.h"

#include "Character/EnemyCharacterBase.h"
#include "EngineUtils.h"
#include "Mob/MobSpawner.h"

USNode_ActivateTutorialSpawner::USNode_ActivateTutorialSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Spawner");
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
	int32 SpawnedCount = 0;
	for (TActorIterator<AMobSpawner> It(World); It; ++It)
	{
		if (It->Tags.Contains(SpawnerActorTag))
		{
			++MatchedCount;
			FStoryMobSpawnOptions Options;
			Options.EnemyClassOverride = EnemyClassOverride;
			Options.bSpawnAtSpawnerLocation = bSpawnAtSpawnerLocation;
			Options.bCountsForLevelClear = bCountsForLevelClear;
			Options.bUnregisterFromEnemyAwareness = bUnregisterFromEnemyAwareness;
			Options.MaxHealthOverride = MaxHealthOverride;
			Options.OnKillEncounterPoint = OnKillEncounterPoint;
			Options.bRespawnOnDeath = bRespawnOnDeath;
			Options.RespawnDelay = RespawnDelay;

			UE_LOG(LogTemp, Log, TEXT("[SNode_ActivateTutorialSpawner] Story spawning from %s with tag %s. EnemyOverride=%s"),
				*GetNameSafe(*It),
				*SpawnerActorTag.ToString(),
				*GetNameSafe(EnemyClassOverride.Get()));
			if (It->SpawnMobForStory(Options))
			{
				++SpawnedCount;
			}
		}
	}

	if (MatchedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_ActivateTutorialSpawner] No AMobSpawner matched tag %s."),
			*SpawnerActorTag.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[SNode_ActivateTutorialSpawner] Matched=%d Spawned=%d Tag=%s."),
			MatchedCount,
			SpawnedCount,
			*SpawnerActorTag.ToString());
	}

	TriggerOutput(TEXT("Out"), true);
}
