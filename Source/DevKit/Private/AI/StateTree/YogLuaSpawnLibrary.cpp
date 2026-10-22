#include "AI/StateTree/YogLuaSpawnLibrary.h"

#include "Character/EnemyCharacterBase.h"
#include "Data/EnemyData.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameModes/YogGameMode.h"
#include "Mob/MobSpawner.h"
#include "NavigationSystem.h"

bool UYogLuaSpawnLibrary::FindReachableSpawnLocation(
	UWorld* World,
	const FVector& Origin,
	float SpawnRadius,
	float MinSpawnDistance,
	int32 MaxAttempts,
	FVector& OutLocation)
{
	if (!World || SpawnRadius <= 0.0f)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return false;
	}

	const float SafeMinDistance = FMath::Clamp(MinSpawnDistance, 0.0f, SpawnRadius);
	const int32 SafeMaxAttempts = FMath::Max(MaxAttempts, 1);
	for (int32 Attempt = 0; Attempt < SafeMaxAttempts; ++Attempt)
	{
		FNavLocation NavLocation;
		if (!NavSys->GetRandomReachablePointInRadius(Origin, SpawnRadius, NavLocation))
		{
			continue;
		}

		if (SafeMinDistance > 0.0f
			&& FVector::DistSquared2D(Origin, NavLocation.Location) < FMath::Square(SafeMinDistance))
		{
			continue;
		}

		OutLocation = NavLocation.Location;
		return true;
	}

	return false;
}

AEnemyCharacterBase* UYogLuaSpawnLibrary::SpawnEnemyAtLocation(
	UWorld* World,
	AActor* SpawnOwner,
	APawn* InstigatorPawn,
	TSubclassOf<AEnemyCharacterBase> EnemyClass,
	const FVector& SpawnLocation)
{
	if (!World || !EnemyClass)
	{
		return nullptr;
	}

	if (AMobSpawner* MobSpawner = Cast<AMobSpawner>(SpawnOwner))
	{
		return MobSpawner->SpawnMobAtLocation(EnemyClass, SpawnLocation);
	}

	AEnemyCharacterBase* Spawned = World->SpawnActorDeferred<AEnemyCharacterBase>(
		EnemyClass,
		FTransform(FRotator::ZeroRotator, SpawnLocation),
		SpawnOwner,
		InstigatorPawn,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Spawned)
	{
		return nullptr;
	}

	Spawned->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnLocation));
	if (!Spawned->GetController())
	{
		Spawned->SpawnDefaultController();
	}
	if (AYogGameMode* GM = World->GetAuthGameMode<AYogGameMode>())
	{
		GM->RegisterEnemy(Spawned);
	}

	return Spawned;
}

AEnemyCharacterBase* UYogLuaSpawnLibrary::SpawnEnemyAroundActor(
	AActor* Origin,
	UEnemyData* EnemyData,
	float Radius,
	float MinDistance,
	float ZOffset,
	bool bCountsForLevelClear)
{
	if (const AController* Controller = Cast<AController>(Origin))
	{
		Origin = Controller->GetPawn();
	}

	if (!Origin || !EnemyData || !EnemyData->EnemyClass)
	{
		return nullptr;
	}

	UWorld* World = Origin->GetWorld();
	FVector SpawnLocation = FVector::ZeroVector;
	if (!FindReachableSpawnLocation(World, Origin->GetActorLocation(), Radius, MinDistance, 24, SpawnLocation))
	{
		return nullptr;
	}

	SpawnLocation.Z += ZOffset;

	APawn* InstigatorPawn = Cast<APawn>(Origin);
	AEnemyCharacterBase* Spawned = SpawnEnemyAtLocation(World, Origin, InstigatorPawn, EnemyData->EnemyClass, SpawnLocation);
	if (Spawned)
	{
		Spawned->bCountsForLevelClear = bCountsForLevelClear;
	}

	return Spawned;
}
