// Fill out your copyright notice in the Description page of Project Settings.


#include "Mob/MobSpawner.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "BuffFlow/Actors/BuffFlowLifecycleProxy.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/LifecycleFlowAsset.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/EnemyWeaponDefinition.h"
#include "Components/SceneComponent.h"
#include "Controller/YogAIController.h"
#include "GameModes/YogGameMode.h"
#include "NavigationSystem.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnLua.h"
#include "UnLuaModule.h"

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

	SelectLuaSpawnRule();
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
    return SpawnMobAtLocationWithWeapon(EnemyClass, Location, nullptr);
}

AEnemyCharacterBase* AMobSpawner::SpawnMobAtLocationWithWeapon(TSubclassOf<AActor> EnemyClass, FVector Location, UEnemyWeaponDefinition* EnemyWeaponDefinition)
{
    UWorld* World = GetWorld();
    if (!World || Location == FVector::ZeroVector) return nullptr;

    AEnemyCharacterBase* Spawned = World->SpawnActorDeferred<AEnemyCharacterBase>(
        EnemyClass,
        FTransform(FRotator::ZeroRotator, Location),
        this,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (Spawned)
    {
        Spawned->SetPendingEnemyWeaponDefinition(EnemyWeaponDefinition);
        Spawned->FinishSpawning(FTransform(FRotator::ZeroRotator, Location));
        if (!Spawned->GetController())
            Spawned->SpawnDefaultController();
        if (AYogGameMode* GM = World->GetAuthGameMode<AYogGameMode>())
            GM->RegisterEnemy(Spawned);
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

static UEnemyData* ResolveEnemyDataFromClass(TSubclassOf<AEnemyCharacterBase> EnemyClass)
{
	const AEnemyCharacterBase* EnemyCDO = EnemyClass ? EnemyClass->GetDefaultObject<AEnemyCharacterBase>() : nullptr;
	const UCharacterDataComponent* CharacterDataComponent = EnemyCDO ? EnemyCDO->FindComponentByClass<UCharacterDataComponent>() : nullptr;
	return CharacterDataComponent ? Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()) : nullptr;
}

static UEnemyWeaponDefinition* ResolveEnemyWeapon(UEnemyData* EnemyData, UEnemyWeaponDefinition* OverrideWeapon)
{
	if (OverrideWeapon)
	{
		return OverrideWeapon;
	}
	if (!EnemyData)
	{
		return nullptr;
	}
	if (EnemyData->DefaultWeaponDefinition)
	{
		return EnemyData->DefaultWeaponDefinition.Get();
	}

	TArray<UEnemyWeaponDefinition*> ValidWeapons;
	for (UEnemyWeaponDefinition* Candidate : EnemyData->AllowedWeaponDefinitions)
	{
		if (Candidate)
		{
			ValidWeapons.Add(Candidate);
		}
	}

	return ValidWeapons.IsEmpty()
		? nullptr
		: ValidWeapons[FMath::RandRange(0, ValidWeapons.Num() - 1)];
}

void AMobSpawner::HandleLifecycleStoryEnemySpawned(AEnemyCharacterBase* SpawnedEnemy, FBuffFlowLifecycleContext& Context)
{
	if (!SpawnedEnemy || Context.bSpawnFinalized)
	{
		return;
	}

	ConfigureStorySpawnedMob(SpawnedEnemy, ActiveStorySpawnOptions);
	ApplyStoryHealthOverride(SpawnedEnemy, ActiveStorySpawnOptions);
	StorySpawnedMob = SpawnedEnemy;
	StoryDeathStartedDelegateHandle = SpawnedEnemy->OnCharacterDeathStartedNative.AddUObject(this, &AMobSpawner::HandleStoryMobDeathStarted);
	StoryDeathDelegateHandle = SpawnedEnemy->OnCharacterDiedNative.AddUObject(this, &AMobSpawner::HandleStoryMobDied);
	Context.bSpawnFinalized = true;

	UE_LOG(LogTemp, Log, TEXT("[MobSpawner][StorySpawn] %s lifecycle-spawned %s Class=%s CountsForClear=%d Respawn=%d Delay=%.2f."),
		*GetNameSafe(this),
		*GetNameSafe(SpawnedEnemy),
		*GetNameSafe(Context.EnemyClass.Get()),
		ActiveStorySpawnOptions.bCountsForLevelClear ? 1 : 0,
		ActiveStorySpawnOptions.bRespawnOnDeath ? 1 : 0,
		ActiveStorySpawnOptions.RespawnDelay);
}

void AMobSpawner::HandleLifecycleStoryEnemySpawnFailed(FBuffFlowLifecycleContext& Context)
{
	if (Context.bSpawnFinalized)
	{
		return;
	}

	Context.bSpawnFinalized = true;
	UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][StorySpawn] %s lifecycle spawn failed. Class=%s."),
		*GetNameSafe(this),
		*GetNameSafe(Context.EnemyClass.Get()));
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

	UEnemyData* EnemyData = ResolveEnemyDataFromClass(EnemyClass);
	UEnemyWeaponDefinition* StoryWeaponDefinition = ResolveEnemyWeapon(
		EnemyData,
		Options.EnemyWeaponDefinitionOverride.Get());
	if (EnemyData && EnemyData->SpawnLifecycleFlow)
    {
        ActiveStorySpawnOptions = Options;
        bStoryKillEncounterTriggered = false;

        FActorSpawnParameters ProxyParams;
        ProxyParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ABuffFlowLifecycleProxy* Proxy = GetWorld()->SpawnActor<ABuffFlowLifecycleProxy>(
            ABuffFlowLifecycleProxy::StaticClass(),
            SpawnLocation,
            GetActorRotation(),
            ProxyParams);
        UBuffFlowComponent* ProxyBuffFlow = Proxy ? Proxy->GetBuffFlowComponent() : nullptr;
        if (!Proxy || !ProxyBuffFlow)
        {
            if (Proxy)
            {
                Proxy->Destroy();
            }
            UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][StorySpawn] %s failed to start lifecycle spawn for %s."),
                *GetNameSafe(this),
                *GetNameSafe(EnemyClass.Get()));
            return nullptr;
        }

        FBuffFlowLifecycleContext Context;
        Context.Type = EBuffFlowLifecycleType::Spawn;
        Context.LifecycleTarget = Proxy;
        Context.Spawner = this;
        Context.EnemyData = EnemyData;
        Context.EnemyWeaponDefinition = StoryWeaponDefinition;
        Context.EnemyClass = EnemyClass;
        Context.SpawnTransform = FTransform(GetActorRotation(), SpawnLocation);
        Context.bStorySpawn = true;

        ProxyBuffFlow->SetLifecycleContext(Context);
        ProxyBuffFlow->StartBuffFlow(EnemyData->SpawnLifecycleFlow.Get(), FGuid::NewGuid(), this, true);
        return StorySpawnedMob.Get();
    }

    AEnemyCharacterBase* Spawned = SpawnMobAtLocationWithWeapon(
        EnemyClass,
        SpawnLocation,
        StoryWeaponDefinition);
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
    return SpawnMobWithWeapon(spawn_actor_class, nullptr);
}

AEnemyCharacterBase* AMobSpawner::SpawnMobWithWeapon(TSubclassOf<AActor> SpawnActorClass, UEnemyWeaponDefinition* EnemyWeaponDefinition)
{
    return SpawnMobAtLocationWithWeapon(SpawnActorClass, PrepareSpawnLocation(), EnemyWeaponDefinition);
}

FVector AMobSpawner::GetRandomReachablePoint()
{
    UWorld* World = GetWorld();
    if (!World) return FVector::ZeroVector;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!NavSys) return FVector::ZeroVector;

    // Reachability, not just projection: a plain ProjectPointToNavigation can snap onto a
    // disconnected nav island (a crate top, the far side of a wall), and an enemy spawned
    // there can never path to the player.
    FNavLocation NavLocation;
    if (!NavSys->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation))
    {
        return FVector::ZeroVector;
    }

    return NavLocation.Location + FVector(0.f, 0.f, SpawnZOffset);
}

void AMobSpawner::SelectLuaSpawnRule()
{
    ActiveLuaRuleModule.Reset();

    // Module names are typed into a text field, and Lua turns every character into part of the
    // require path, so surrounding whitespace silently resolves to a different file.
    TArray<TPair<FString, float>> Candidates;
    Candidates.Reserve(LuaSpawnRulePool.Num());

    float TotalWeight = 0.f;
    for (const FYogLuaSpawnRule& Rule : LuaSpawnRulePool)
    {
        FString ModuleName = Rule.ModuleName.TrimStartAndEnd();
        if (ModuleName.IsEmpty() || Rule.Weight <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        TotalWeight += Rule.Weight;
        Candidates.Emplace(MoveTemp(ModuleName), Rule.Weight);
    }

    if (Candidates.IsEmpty())
    {
        return;
    }

    float Roll = FMath::FRandRange(0.f, TotalWeight);
    for (const TPair<FString, float>& Candidate : Candidates)
    {
        // Assigning every pass leaves the last entry selected when float accumulation keeps Roll
        // just above zero.
        ActiveLuaRuleModule = Candidate.Key;
        Roll -= Candidate.Value;
        if (Roll <= 0.f)
        {
            break;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MobSpawner][Lua] %s selected rule %s from %d candidates (budget %d, pool %d)."),
        *GetNameSafe(this),
        *ActiveLuaRuleModule,
        LuaSpawnRulePool.Num(),
        SpawnDifficultyBudget,
        MobPool.Num());
}

int32 AMobSpawner::GetMobPoolSize() const
{
    return MobPool.Num();
}

int32 AMobSpawner::GetMobPoolDifficultyScore(int32 PoolIndex) const
{
    const int32 Index = PoolIndex - 1;
    if (!MobPool.IsValidIndex(Index) || !MobPool[Index].EnemyData)
    {
        return 0;
    }

    return MobPool[Index].EnemyData->DifficultyScore;
}

AEnemyCharacterBase* AMobSpawner::SpawnFromMobPool(int32 PoolIndex, FVector Location)
{
    const int32 Index = PoolIndex - 1;
    if (!MobPool.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %s got out of range pool index %d (pool %d)."),
            *GetNameSafe(this),
            PoolIndex,
            MobPool.Num());
        return nullptr;
    }

    const FEnemyEntry& Entry = MobPool[Index];
    UEnemyData* EnemyData = Entry.EnemyData;
    if (!EnemyData || !EnemyData->EnemyClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %s pool entry %d has no EnemyData or EnemyClass."),
            *GetNameSafe(this),
            PoolIndex);
        return nullptr;
    }

    const int32 SpawnedSoFar = MobPoolSpawnCounts.FindRef(Index);
    if (Entry.MaxCountPerLevel >= 0 && SpawnedSoFar >= Entry.MaxCountPerLevel)
    {
        UE_LOG(LogTemp, Log, TEXT("[MobSpawner][Lua] %s refused pool entry %d (%s): hit MaxCountPerLevel %d."),
            *GetNameSafe(this),
            PoolIndex,
            *GetNameSafe(EnemyData),
            Entry.MaxCountPerLevel);
        return nullptr;
    }

    // Reserve before spawning: OnEnemySpawned can run Blueprint that re-enters this function, and a
    // held TMap reference would dangle across the rehash.
    MobPoolSpawnCounts.Add(Index, SpawnedSoFar + 1);

    AEnemyCharacterBase* Spawned = SpawnMobAtLocationWithWeapon(
        EnemyData->EnemyClass,
        Location,
        ResolveEnemyWeapon(EnemyData, nullptr));
    if (!Spawned)
    {
        MobPoolSpawnCounts.Add(Index, MobPoolSpawnCounts.FindRef(Index) - 1);
    }

    return Spawned;
}

/**
 * Dispatcher lives in Lua rather than C++ so designers can add shared rule helpers without a rebuild.
 * Required directly instead of through a global set by the startup module, so a missing or reordered
 * Main.lua cannot silently break spawning.
 */
static bool MobSpawner_CallLuaRule(AMobSpawner* Spawner, const FString& ModuleName)
{
    static const char* DispatcherModule = "Mob.SpawnDispatcher";

    UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(Spawner);
    lua_State* L = Env ? Env->GetMainState() : nullptr;
    if (!L)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %s has no Lua env; rule %s skipped."),
            *GetNameSafe(Spawner),
            *ModuleName);
        return false;
    }

    const int32 Top = lua_gettop(L);

    lua_pushcfunction(L, UnLua::ReportLuaCallError);
    const int32 MsgHandlerIdx = lua_gettop(L);

    lua_getglobal(L, "require");
    lua_pushstring(L, DispatcherModule);
    if (lua_pcall(L, 1, 1, MsgHandlerIdx) != LUA_OK || !lua_istable(L, -1))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %s could not require %hs."),
            *GetNameSafe(Spawner),
            DispatcherModule);
        lua_settop(L, Top);
        return false;
    }

    if (lua_getfield(L, -1, "Run") != LUA_TFUNCTION)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %hs does not expose a Run function."), DispatcherModule);
        lua_settop(L, Top);
        return false;
    }
    lua_remove(L, -2); // drop the dispatcher table, keep Run

    UnLua::PushUObject(L, Spawner);
    lua_pushstring(L, TCHAR_TO_UTF8(*ModuleName));

    const bool bOk = lua_pcall(L, 2, 0, MsgHandlerIdx) == LUA_OK;
    if (!bOk)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %s rule %s raised an error."),
            *GetNameSafe(Spawner),
            *ModuleName);
    }

    lua_settop(L, Top);
    return bOk;
}

bool AMobSpawner::RunLuaSpawnRule()
{
    if (ActiveLuaRuleModule.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MobSpawner][Lua] %s has no active rule; nothing spawned."),
            *GetNameSafe(this));
        return false;
    }

    return MobSpawner_CallLuaRule(this, ActiveLuaRuleModule);
}
