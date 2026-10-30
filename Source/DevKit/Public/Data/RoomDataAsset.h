// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RoomDataAsset.generated.h"

class UWorld;

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
};
