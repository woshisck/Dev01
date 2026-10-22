#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "YogLuaSpawnLibrary.generated.h"

class AActor;
class AEnemyCharacterBase;
class APawn;
class UEnemyData;
class UWorld;

UCLASS()
class DEVKIT_API UYogLuaSpawnLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool FindReachableSpawnLocation(
		UWorld* World,
		const FVector& Origin,
		float SpawnRadius,
		float MinSpawnDistance,
		int32 MaxAttempts,
		FVector& OutLocation);

	static AEnemyCharacterBase* SpawnEnemyAtLocation(
		UWorld* World,
		AActor* SpawnOwner,
		APawn* InstigatorPawn,
		TSubclassOf<AEnemyCharacterBase> EnemyClass,
		const FVector& SpawnLocation);

	/**
	 * Spawns one enemy of EnemyData->EnemyClass on a reachable NavMesh point inside Radius around Origin.
	 * If Origin is an AController its pawn is used. Returns null when no point could be found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Yog|Lua Spawn")
	static AEnemyCharacterBase* SpawnEnemyAroundActor(
		AActor* Origin,
		UEnemyData* EnemyData,
		float Radius,
		float MinDistance = 0.0f,
		float ZOffset = 96.0f,
		bool bCountsForLevelClear = true);
};
