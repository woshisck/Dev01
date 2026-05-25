#include "Map/TutorialMobSpawner.h"

#include "Character/EnemyCharacterBase.h"
#include "Character/TrainingDummyCharacter.h"
#include "Character/YogCharacterBase.h"
#include "GameModes/YogGameMode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

ATutorialMobSpawner::ATutorialMobSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATutorialMobSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (bActivateOnBeginPlay)
	{
		Activate();
	}
}

void ATutorialMobSpawner::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	if (AEnemyCharacterBase* Mob = SpawnedMob.Get())
	{
		Mob->OnCharacterDiedNative.Remove(DeathDelegateHandle);
	}
	Super::EndPlay(Reason);
}

void ATutorialMobSpawner::Activate()
{
	if (bIsActive)
	{
		return;
	}
	bIsActive = true;
	DoSpawnMob();
}

void ATutorialMobSpawner::DoSpawnMob()
{
	if (AEnemyCharacterBase* OldMob = SpawnedMob.Get())
	{
		OldMob->OnCharacterDiedNative.Remove(DeathDelegateHandle);
		OldMob->Destroy();
	}
	DeathDelegateHandle.Reset();
	SpawnedMob.Reset();

	if (EnemySpawnClassis.IsEmpty() || !EnemySpawnClassis[0])
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialMobSpawner] %s: no class in EnemySpawnClassis[0]."), *GetNameSafe(this));
		return;
	}

	AEnemyCharacterBase* Mob = SpawnMob(EnemySpawnClassis[0]);
	if (!Mob)
	{
		return;
	}

	Mob->bCountsForLevelClear = false;

	if (ATrainingDummyCharacter* TrainingDummy = Cast<ATrainingDummyCharacter>(Mob))
	{
		TrainingDummy->bResetOnDeath = false;
	}

	if (AYogGameMode* GM = GetWorld()->GetAuthGameMode<AYogGameMode>())
	{
		GM->UnregisterEnemy(Mob);
	}

	SpawnedMob = Mob;
	DeathDelegateHandle = Mob->OnCharacterDiedNative.AddUObject(this, &ATutorialMobSpawner::HandleMobDied);
}

void ATutorialMobSpawner::HandleMobDied(AYogCharacterBase* Mob)
{
	if (AEnemyCharacterBase* DeadMob = Cast<AEnemyCharacterBase>(Mob))
	{
		DeadMob->OnCharacterDiedNative.Remove(DeathDelegateHandle);
	}
	DeathDelegateHandle.Reset();
	SpawnedMob.Reset();

	if (OnKillEncounterPoint)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UStoryEncounterRuntimeSubsystem* Story = GI->GetSubsystem<UStoryEncounterRuntimeSubsystem>())
			{
				Story->TriggerEncounterPoint(OnKillEncounterPoint, Mob);
			}
		}
	}

	GetWorldTimerManager().SetTimer(
		RespawnTimer,
		this,
		&ATutorialMobSpawner::DoSpawnMob,
		RespawnDelay,
		false);
}
