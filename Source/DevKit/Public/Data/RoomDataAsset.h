// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RoomDataAsset.generated.h"

class UWorld;

UENUM(BlueprintType)
enum class ERoomDoorDir : uint8
{
	PosX,
	NegX,
	PosY,
	NegY
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRoomDoorSocket
{
	GENERATED_BODY()

	// A footprint cell of the room.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	FIntPoint Cell = FIntPoint::ZeroValue;

	// Must be an outer edge: no footprint cell on the other side.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	ERoomDoorDir Direction = ERoomDoorDir::PosX;

	bool operator==(const FRoomDoorSocket& Other) const
	{
		return Cell == Other.Cell && Direction == Other.Direction;
	}
};

/**
 * URoomDataAsset - identity of one room map.
 *
 * Naming: DA_Room_<scene>_<type>, for example DA_Room_Prison_Normal.
 *
 * What a room spawns and when its doors open is decided by that map's AYogRoomDirector and Lua module,
 * not by this asset. APortal hands it to AYogGameMode::TransitionToLevel so the next level knows where
 * the player came from.
 */
UCLASS(BlueprintType)
class DEVKIT_API URoomDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Explicit editor association. Existing assets retain RoomName-based runtime loading. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room", meta = (DisplayName = "关卡资产"))
	TSoftObjectPtr<UWorld> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FName RoomName;

	// Player-facing room name (portal float panel, HUD). Empty falls back to RoomName.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FText DisplayName;

	// Type and layer tags, for example Room.Type.Normal / Room.Type.Elite / Room.Layer.L1.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FGameplayTagContainer RoomTags;

	// Cells the room occupies, relative to its origin cell (0,0). Empty means a single (0,0) cell.
	// Must contain (0,0) and be 4-connected.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	TArray<FIntPoint> FootprintCells;

	// Where doors may sit. Empty means every outer edge of the footprint.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	TArray<FRoomDoorSocket> DoorSockets;

	// Relative chance among rooms of the same type. 0 means never picked.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "0.0"))
	float GenWeight = 1.0f;

	// Max copies on one floor. 0 means unlimited.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = "0"))
	int32 MaxPerFloor = 0;
};
