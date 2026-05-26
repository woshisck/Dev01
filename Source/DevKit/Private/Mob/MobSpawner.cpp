// Fill out your copyright notice in the Description page of Project Settings.


#include "Mob/MobSpawner.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Components/SceneComponent.h"
#include "Controller/YogAIController.h"
#include "GameModes/YogGameMode.h"
#include "NavigationSystem.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AMobSpawner::AMobSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

// Called when the game starts or when spawned
void AMobSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMobSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearStorySpawnedMob();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AMobSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector AMobSpawner::PrepareSpawnLocation()
{
    return GetRandomReachablePoint();
}

AEnemyCharacterBase* AMobSpawner::SpawnMobAtLocation(TSubclassOf<AActor> EnemyClass, FVector Location)
{
    UWorld* World = GetWorld();
    if (!World || Location == FVector::ZeroVector) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEnemyCharacterBase* Spawned = World->SpawnActor<AEnemyCharacterBase>(EnemyClass, Location, FRotator::ZeroRotator, Params);
    if (Spawned)
    {
        if (!Spawned->GetController())
            Spawned->SpawnDefaultController();
        OnEnemySpawned(Spawned, Location);
    }
    return Spawned;
}

void AMobSpawner::ConfigureStorySpawnedMob(AEnemyCharacterBase* Mob, const FStoryMobSpawnOptions& Options) const
{
    if (!Mob)
    {
        return;
    }

    Mob->bCountsForLevelClear = Options.bCountsForLevelClear;
    if (Options.bUnregisterFromEnemyAwareness)
    {
        if (AYogGameMode* GM = GetWorld()->GetAuthGameMode<AYogGameMode>())
        {
            GM->UnregisterEnemy(Mob);
        }
    }
}

void AMobSpawner::ApplyStoryHealthOverride(AEnemyCharacterBase* Mob, const FStoryMobSpawnOptions& Options) const
{
    if (!Mob || Options.MaxHealthOverride <= 0.0f)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Mob->GetAbilitySystemComponent();
    if (!ASC
        || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute())
        || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()))
    {
        UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s skipped health override on %s (missing ASC/base attributes)."),
            *GetNameSafe(this),
            *GetNameSafe(Mob));
        return;
    }

    const float OldMaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
    const float OldHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
    ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), Options.MaxHealthOverride);
    ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), Options.MaxHealthOverride);

    UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s health override on %s: Max %.1f -> %.1f, Health %.1f -> %.1f."),
        *GetNameSafe(this),
        *GetNameSafe(Mob),
        OldMaxHealth,
        Options.MaxHealthOverride,
        OldHealth,
        Options.MaxHealthOverride);
}

AEnemyCharacterBase* AMobSpawner::SpawnMobForStory(const FStoryMobSpawnOptions& Options)
{
    if (Options.bDestroyExistingStoryMob)
    {
        ClearStorySpawnedMob();
    }
    else if (StorySpawnedMob.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s already has story mob %s."),
            *GetNameSafe(this),
            *GetNameSafe(StorySpawnedMob.Get()));
        return StorySpawnedMob.Get();
    }

    TSubclassOf<AEnemyCharacterBase> EnemyClass = Options.EnemyClassOverride;
    if (!EnemyClass)
    {
        if (EnemySpawnClassis.IsEmpty() || !EnemySpawnClassis[0])
        {
            UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][StorySpawn] %s skipped: no EnemyClassOverride or EnemySpawnClassis[0]."),
                *GetNameSafe(this));
            return nullptr;
        }
        EnemyClass = EnemySpawnClassis[0];
    }

    const FVector SpawnLocation = Options.bSpawnAtSpawnerLocation
        ? GetActorLocation() + FVector(0.f, 0.f, SpawnZOffset)
        : PrepareSpawnLocation();
    if (SpawnLocation == FVector::ZeroVector)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][StorySpawn] %s skipped: invalid spawn location."), *GetNameSafe(this));
        return nullptr;
    }

    AEnemyCharacterBase* Spawned = SpawnMobAtLocation(EnemyClass, SpawnLocation);
    if (!Spawned)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][StorySpawn] %s failed to spawn %s at %s."),
            *GetNameSafe(this),
            *GetNameSafe(EnemyClass.Get()),
            *SpawnLocation.ToCompactString());
        return nullptr;
    }

    ActiveStorySpawnOptions = Options;
    bStoryKillEncounterTriggered = false;
    ConfigureStorySpawnedMob(Spawned, Options);
    ApplyStoryHealthOverride(Spawned, Options);
    StorySpawnedMob = Spawned;
    StoryDeathStartedDelegateHandle = Spawned->OnCharacterDeathStartedNative.AddUObject(this, &AMobSpawner::HandleStoryMobDeathStarted);
    StoryDeathDelegateHandle = Spawned->OnCharacterDiedNative.AddUObject(this, &AMobSpawner::HandleStoryMobDied);

    UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s spawned %s Class=%s CountsForClear=%d Respawn=%d Delay=%.2f."),
        *GetNameSafe(this),
        *GetNameSafe(Spawned),
        *GetNameSafe(EnemyClass.Get()),
        Options.bCountsForLevelClear ? 1 : 0,
        Options.bRespawnOnDeath ? 1 : 0,
        Options.RespawnDelay);
    return Spawned;
}

void AMobSpawner::ClearStorySpawnedMob()
{
    GetWorldTimerManager().ClearTimer(StoryRespawnTimer);

    if (AEnemyCharacterBase* Mob = StorySpawnedMob.Get())
    {
        Mob->OnCharacterDeathStartedNative.Remove(StoryDeathStartedDelegateHandle);
        Mob->OnCharacterDiedNative.Remove(StoryDeathDelegateHandle);
        Mob->Destroy();
    }
    StoryDeathStartedDelegateHandle.Reset();
    StoryDeathDelegateHandle.Reset();
    StorySpawnedMob.Reset();
    bStoryKillEncounterTriggered = false;
}

void AMobSpawner::HandleStoryMobDeathStarted(AYogCharacterBase* Mob)
{
    if (AEnemyCharacterBase* DeadMob = Cast<AEnemyCharacterBase>(Mob))
    {
        DeadMob->OnCharacterDeathStartedNative.Remove(StoryDeathStartedDelegateHandle);
    }
    StoryDeathStartedDelegateHandle.Reset();

    UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s observed death start of %s. OnKillEncounterPoint=%s."),
        *GetNameSafe(this),
        *GetNameSafe(Mob),
        *GetNameSafe(ActiveStorySpawnOptions.OnKillEncounterPoint));

    TriggerStoryKillEncounter(Mob);
}

void AMobSpawner::HandleStoryMobDied(AYogCharacterBase* Mob)
{
    if (AEnemyCharacterBase* DeadMob = Cast<AEnemyCharacterBase>(Mob))
    {
        DeadMob->OnCharacterDeathStartedNative.Remove(StoryDeathStartedDelegateHandle);
        DeadMob->OnCharacterDiedNative.Remove(StoryDeathDelegateHandle);
    }
    StoryDeathStartedDelegateHandle.Reset();
    StoryDeathDelegateHandle.Reset();
    StorySpawnedMob.Reset();

    UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s observed death of %s. OnKillEncounterPoint=%s."),
        *GetNameSafe(this),
        *GetNameSafe(Mob),
        *GetNameSafe(ActiveStorySpawnOptions.OnKillEncounterPoint));

    TriggerStoryKillEncounter(Mob);

    if (ActiveStorySpawnOptions.bRespawnOnDeath)
    {
        GetWorldTimerManager().SetTimer(
            StoryRespawnTimer,
            this,
            &AMobSpawner::RespawnStoryMob,
            ActiveStorySpawnOptions.RespawnDelay,
            false);
    }
}

void AMobSpawner::TriggerStoryKillEncounter(AYogCharacterBase* Mob)
{
    if (bStoryKillEncounterTriggered)
    {
        return;
    }

    if (ActiveStorySpawnOptions.OnKillEncounterPoint)
    {
        bool bTriggered = false;
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UStoryEncounterRuntimeSubsystem* Story = GI->GetSubsystem<UStoryEncounterRuntimeSubsystem>())
            {
                bTriggered = Story->TriggerEncounterPoint(ActiveStorySpawnOptions.OnKillEncounterPoint, Mob);
            }
        }

        UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s triggered kill encounter %s: %d."),
            *GetNameSafe(this),
            *GetNameSafe(ActiveStorySpawnOptions.OnKillEncounterPoint),
            bTriggered ? 1 : 0);
    }

    bStoryKillEncounterTriggered = true;
}

void AMobSpawner::RespawnStoryMob()
{
    SpawnMobForStory(ActiveStorySpawnOptions);
}

AEnemyCharacterBase* AMobSpawner::SpawnMob(TSubclassOf<AActor> spawn_actor_class)
{
    return SpawnMobAtLocation(spawn_actor_class, PrepareSpawnLocation());
}

FVector AMobSpawner::GetRandomReachablePoint()
{
    UWorld* World = GetWorld();
    if (!World) return FVector::ZeroVector;

    //UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return FVector::ZeroVector;
    // Pick random point in circle
    FVector Origin = GetActorLocation();
    // 只在 XY 平面随机，避免测试点落入地下导致 NavMesh 投影失败
    float Angle = FMath::FRandRange(0.f, 2.f * PI);
    float Dist  = FMath::FRandRange(0.f, SpawnRadius);
    FVector RandomPoint = FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
    FVector TestLocation = FVector(Origin.X + RandomPoint.X, Origin.Y + RandomPoint.Y, Origin.Z);
    FNavLocation NavLocation;
    bool bFound = NavSys->ProjectPointToNavigation(TestLocation, NavLocation);
    return bFound ? NavLocation.Location + FVector(0.f, 0.f, SpawnZOffset) : FVector::ZeroVector;

}
