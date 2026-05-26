#include "Map/TutorialMobSpawner.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
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
	if (bIsActive && SpawnedMob.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] %s already active with mob %s."),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedMob.Get()));
		return;
	}
	bIsActive = true;
	UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] Activating %s."), *GetNameSafe(this));
	DoSpawnMob();
}

AEnemyCharacterBase* ATutorialMobSpawner::SpawnTutorialMobAtLocation(
	TSubclassOf<AEnemyCharacterBase> EnemyClass,
	const FVector& SpawnLocation)
{
	UWorld* World = GetWorld();
	if (!World || !EnemyClass)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);
	AEnemyCharacterBase* Mob = World->SpawnActorDeferred<AEnemyCharacterBase>(
		EnemyClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Mob)
	{
		return nullptr;
	}

	ConfigureSpawnedMob(Mob);
	Mob->FinishSpawning(SpawnTransform);

	if (!Mob->GetController())
	{
		Mob->SpawnDefaultController();
	}

	OnEnemySpawned(Mob, SpawnLocation);
	return Mob;
}

void ATutorialMobSpawner::ConfigureSpawnedMob(AEnemyCharacterBase* Mob) const
{
	if (!Mob)
	{
		return;
	}

	Mob->bCountsForLevelClear = false;
	if (ATrainingDummyCharacter* TrainingDummy = Cast<ATrainingDummyCharacter>(Mob))
	{
		TrainingDummy->bResetOnDeath = false;
	}
}

void ATutorialMobSpawner::ApplySpawnedMobHealthOverride(AEnemyCharacterBase* Mob) const
{
	if (!Mob || SpawnedMobMaxHealthOverride <= 0.0f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Mob->GetAbilitySystemComponent();
	if (!ASC
		|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute())
		|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()))
	{
		UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] %s: skipped health override on %s (missing ASC/base attributes)."),
			*GetNameSafe(this),
			*GetNameSafe(Mob));
		return;
	}

	const float OldMaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	const float OldHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), SpawnedMobMaxHealthOverride);
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), SpawnedMobMaxHealthOverride);

	UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] %s health override on %s: Max %.1f -> %.1f, Health %.1f -> %.1f."),
		*GetNameSafe(this),
		*GetNameSafe(Mob),
		OldMaxHealth,
		SpawnedMobMaxHealthOverride,
		OldHealth,
		SpawnedMobMaxHealthOverride);
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

	const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, SpawnZOffset);
	AEnemyCharacterBase* Mob = SpawnTutorialMobAtLocation(EnemySpawnClassis[0], SpawnLocation);
	if (!Mob)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialMobSpawner] %s: failed to spawn %s at %s."),
			*GetNameSafe(this),
			*GetNameSafe(EnemySpawnClassis[0].Get()),
			*SpawnLocation.ToCompactString());
		return;
	}

	ConfigureSpawnedMob(Mob);
	ApplySpawnedMobHealthOverride(Mob);

	UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] %s spawned %s at %s."),
		*GetNameSafe(this),
		*GetNameSafe(Mob),
		*SpawnLocation.ToCompactString());

	if (AYogGameMode* GM = GetWorld()->GetAuthGameMode<AYogGameMode>())
	{
		GM->UnregisterEnemy(Mob);
	}

	SpawnedMob = Mob;
	DeathDelegateHandle = Mob->OnCharacterDiedNative.AddUObject(this, &ATutorialMobSpawner::HandleMobDied);
}

void ATutorialMobSpawner::HandleMobDied(AYogCharacterBase* Mob)
{
	UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] %s observed death of %s. OnKillEncounterPoint=%s RespawnDelay=%.2f."),
		*GetNameSafe(this),
		*GetNameSafe(Mob),
		*GetNameSafe(OnKillEncounterPoint),
		RespawnDelay);

	if (AEnemyCharacterBase* DeadMob = Cast<AEnemyCharacterBase>(Mob))
	{
		DeadMob->OnCharacterDiedNative.Remove(DeathDelegateHandle);
	}
	DeathDelegateHandle.Reset();
	SpawnedMob.Reset();

	if (OnKillEncounterPoint)
	{
		bool bTriggeredKillPoint = false;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UStoryEncounterRuntimeSubsystem* Story = GI->GetSubsystem<UStoryEncounterRuntimeSubsystem>())
			{
				bTriggeredKillPoint = Story->TriggerEncounterPoint(OnKillEncounterPoint, Mob);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[TutorialMobSpawner] %s triggered kill encounter %s: %d."),
			*GetNameSafe(this),
			*GetNameSafe(OnKillEncounterPoint),
			bTriggeredKillPoint ? 1 : 0);
	}

	GetWorldTimerManager().SetTimer(
		RespawnTimer,
		this,
		&ATutorialMobSpawner::DoSpawnMob,
		RespawnDelay,
		false);
}
