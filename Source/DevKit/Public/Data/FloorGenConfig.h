#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FloorGenConfig.generated.h"

class AActor;
class URoomDataAsset;
class UWeaponDefinition;

UENUM(BlueprintType)
enum class EFloorRoomPlacement : uint8
{
	Start,
	End,
	MainPath,
	SideBranch,
	Any
};

USTRUCT(BlueprintType)
struct DEVKIT_API FFloorRoomRule
{
	GENERATED_BODY()

	// A Room.Type.* tag. A room from the pool carrying this tag is picked by GenWeight.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule", meta = (Categories = "Room.Type"))
	FGameplayTag RoomType;

	// Optional. Use exactly this room instead of picking from the pool.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule")
	TObjectPtr<URoomDataAsset> OverrideRoom = nullptr;

	// How many to place. Start and End rules must use 1.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule")
	EFloorRoomPlacement Placement = EFloorRoomPlacement::MainPath;

	// Allowed depth, 0..1. Ignored for Start and End.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule")
	FVector2D DepthRange = FVector2D(0.0, 1.0);

	// The room gets exactly one door.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule")
	bool bDeadEnd = false;
};

/** One actor spawned inside a room at generation time. */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorSpawnEntry
{
	GENERATED_BODY()

	// Usually BP_WeaponSpawner, but any actor class works.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AActor> ActorClass;

	// Applied only when ActorClass is an AWeaponSpawner. Set before the actor finishes spawning so
	// its display mesh and info widget are built from it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<UWeaponDefinition> WeaponDefinition = nullptr;

	// Offset from the room centre in world axes. Z is the height above the floor surface. The Start
	// room centre is the player spawn point, so keep weapons a few hundred units away from it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	FVector Offset = FVector(400.0, 0.0, 0.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float Yaw = 0.f;
};

/** Everything spawned into every room that carries RoomType. */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorRoomPopulation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (Categories = "Room.Type"))
	FGameplayTag RoomType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
	TArray<FFloorSpawnEntry> Entries;
};

/**
 * UFloorGenConfig - rules for generating one floor layout.
 *
 * Naming: DA_FloorGen_<Layer>, for example DA_FloorGen_L1.
 */
UCLASS(BlueprintType)
class DEVKIT_API UFloorGenConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// World size of one grid cell. Match the size of the smallest room.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FVector2D CellSize = FVector2D(2400.0, 2400.0);

	// Max floor area in cells.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1"))
	FIntPoint GridSize = FIntPoint(10, 10);

	// Total rooms, required rooms included.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "2"))
	int32 RoomCountMin = 18;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "2"))
	int32 RoomCountMax = 22;

	// Rooms on the Start -> Boss route, both ends included.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "2"))
	int32 MainPathMin = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "2"))
	int32 MainPathMax = 10;

	// Longest side branch, in rooms.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "1"))
	int32 BranchMaxLength = 3;

	// Rooms the generator may use. Normal rooms fill everything not covered by a rule.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	TArray<TObjectPtr<URoomDataAsset>> RoomPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	TArray<FFloorRoomRule> RequiredRooms;

	// Actors spawned into rooms by type, e.g. weapon spawners in the Start room.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	TArray<FFloorRoomPopulation> RoomPopulation;

	// Backtracking budget per seed during grid placement.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retry", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 200;

	// Derived seeds tried before giving up.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retry", meta = (ClampMin = "0"))
	int32 MaxSeedRetries = 20;
};
