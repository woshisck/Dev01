// Fill out your copyright notice in the Description page of Project Settings.


#include "Mob/MobSpawner.h"
#include "Controller/YogAIController.h"
#include "Character/EnemyCharacterBase.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AMobSpawner::AMobSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMobSpawner::BeginPlay()
{
	Super::BeginPlay();
	
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
