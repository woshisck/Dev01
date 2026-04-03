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

AEnemyCharacterBase* AMobSpawner::SpawnMob(TSubclassOf<AActor> spawn_actor_class)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    //if (EnemySpawnClassis.Num() <= 0)
    //{
    //    return nullptr;
    //}
    //TSubclassOf<AEnemyCharacterBase> RandomClass;
    //if (SingleSpawn == false)
    //{
    //    int32 RandomIndex = FMath::RandRange(0, EnemySpawnClassis.Num() - 1);
    //    RandomClass = EnemySpawnClassis[RandomIndex];
    //}
    //else
    //{
    //    RandomClass = EnemySpawnClassis[0];
    //}



    FVector Location = GetRandomReachablePoint();
    if (Location != FVector::ZeroVector)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AEnemyCharacterBase* Spawned_Enemy = World->SpawnActor<AEnemyCharacterBase>(spawn_actor_class, Location, FRotator::ZeroRotator, Params);
        return Spawned_Enemy;
    }
    else
    {
        return nullptr;
    }
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
    return bFound ? NavLocation.Location : FVector::ZeroVector;

}
