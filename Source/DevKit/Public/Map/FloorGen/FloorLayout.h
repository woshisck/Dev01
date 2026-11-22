#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/RoomDataAsset.h"
#include "FloorLayout.generated.h"

/** One placed room. Cells are absolute grid cells. */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorRoomInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	TObjectPtr<URoomDataAsset> Room = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	FGameplayTag RoomType;

	// Grid cell that the room's footprint cell (0,0) sits on.
	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	FIntPoint Origin = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	TArray<FIntPoint> Cells;

	// 0 = Start, 1 = Boss. Side branch rooms share the depth of the main path room they hang from.
	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	float Depth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	bool bMainPath = false;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	int32 ParentIndex = INDEX_NONE;
};

/** A doorway between two rooms. CellA / CellB are the footprint cells the door sits on, DirA / DirB the edges. */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorDoorLink
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	int32 RoomA = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	int32 RoomB = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	FIntPoint CellA = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	ERoomDoorDir DirA = ERoomDoorDir::PosX;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	FIntPoint CellB = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	ERoomDoorDir DirB = ERoomDoorDir::NegX;
};

/** Pure data output of the floor generator. Room 0 is Start; the last entry of MainPath is Boss. */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorLayout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	int32 Seed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	FIntPoint GridSize = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	TArray<FFloorRoomInstance> Rooms;

	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	TArray<FFloorDoorLink> Doors;

	// Room indices from Start to Boss.
	UPROPERTY(BlueprintReadOnly, Category = "FloorLayout")
	TArray<int32> MainPath;

	int32 CountDoors(int32 RoomIndex) const
	{
		int32 Count = 0;
		for (const FFloorDoorLink& Door : Doors)
		{
			if (Door.RoomA == RoomIndex || Door.RoomB == RoomIndex)
			{
				++Count;
			}
		}
		return Count;
	}
};

struct FFloorGrid
{
	static FIntPoint DirOffset(ERoomDoorDir Dir)
	{
		switch (Dir)
		{
		case ERoomDoorDir::PosX:
			return FIntPoint(1, 0);
		case ERoomDoorDir::NegX:
			return FIntPoint(-1, 0);
		case ERoomDoorDir::PosY:
			return FIntPoint(0, 1);
		default:
			return FIntPoint(0, -1);
		}
	}

	static ERoomDoorDir Opposite(ERoomDoorDir Dir)
	{
		switch (Dir)
		{
		case ERoomDoorDir::PosX:
			return ERoomDoorDir::NegX;
		case ERoomDoorDir::NegX:
			return ERoomDoorDir::PosX;
		case ERoomDoorDir::PosY:
			return ERoomDoorDir::NegY;
		default:
			return ERoomDoorDir::PosY;
		}
	}
};
