#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/FloorGen/FloorLayout.h"
#include "YogFloorDirector.generated.h"

class UFloorGenConfig;
class ULevelStreamingDynamic;
class APortalLinkedDoor;
class UStaticMesh;

/**
 * Generates a floor layout, draws it as debug boxes and streams each room's LevelAsset in at its
 * grid position. Spawns nothing else and opens no doors yet. The actor's location is the world
 * position of grid cell (0,0).
 */
UCLASS()
class DEVKIT_API AYogFloorDirector : public AActor
{
	GENERATED_BODY()

public:
	AYogFloorDirector();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen")
	TObjectPtr<UFloorGenConfig> Config;

	// 0 picks a random seed on every generation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorGen")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorGen|Debug")
	bool bDrawDebug = true;

	// Rooms without a LevelAsset are skipped and only appear as debug boxes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Streaming")
	bool bStreamRooms = true;

	/** Generates with InSeed (0 = random). Returns false when every seed retry failed. */
	UFUNCTION(BlueprintCallable, Category = "FloorGen")
	bool GenerateFloor(int32 InSeed = 0);

	UFUNCTION(BlueprintCallable, Category = "FloorGen|Debug")
	void SetDebugDrawEnabled(bool bEnabled);

	// Spawns wall segments on every outer room edge and a door actor on every door link.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls")
	bool bBuildWalls = true;

	// Unit cube (100 units) that is scaled into wall segments.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls")
	TObjectPtr<UStaticMesh> WallMesh;

	// Optional. When empty, door links are left as open gaps.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls")
	TSubclassOf<APortalLinkedDoor> DoorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls", meta = (ClampMin = "10.0"))
	float WallHeight = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls", meta = (ClampMin = "10.0"))
	float WallThickness = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls", meta = (ClampMin = "10.0"))
	float DoorWidth = 400.f;

	// Height of the room floor surface; walls and doors stand on it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Walls")
	float FloorTopZ = 50.f;

	// Moves every PlayerStart and every possessed pawn to the centre of the Start room.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Spawn")
	bool bMovePlayerToStart = true;

	// Height above the floor surface the player is placed at.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Spawn")
	float PlayerSpawnHeight = 120.f;

	// Spawns the actors listed in the config's RoomPopulation into matching rooms.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloorGen|Population")
	bool bPopulateRooms = true;

	// Raised at the end of a successful GenerateFloor, after rooms, walls, population and player
	// placement. Hook for Blueprint or Lua logic that goes beyond the config's RoomPopulation.
	UFUNCTION(BlueprintImplementableEvent, Category = "FloorGen")
	void OnFloorGenerated();

	// Destroyed together with the walls when the floor is regenerated or unloaded.
	UFUNCTION(BlueprintCallable, Category = "FloorGen|Population")
	void RegisterSpawnedPart(AActor* Part);

	UFUNCTION(BlueprintCallable, Category = "FloorGen|Streaming")
	void UnloadRooms();

	const FFloorLayout& GetLayout() const
	{
		return Layout;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StreamRooms();
	void MovePlayerToStart();
	void BuildWallsAndDoors();
	void PopulateRooms();
	FVector GetRoomCenter(const FFloorRoomInstance& Room) const;
	void SpawnWallSegment(const FVector& Center, const FVector& Size);
	void RedrawDebug();
	void ClearDebug();
	FVector CellCenter(const FIntPoint& Cell) const;
	FVector DoorPosition(const FIntPoint& Cell, ERoomDoorDir Dir) const;

	UPROPERTY(Transient)
	FFloorLayout Layout;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelStreamingDynamic>> StreamedRooms;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedParts;

	FString StatsText;
	bool bHasLayout = false;
};
