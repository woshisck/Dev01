// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CameraShakeLevelRow.generated.h"

class UCameraShakeBase;

/**
 * One camera-shake intensity level, keyed by integer Level (heavy attack = 1, crit = 2, ...).
 * Rows live in a UDataTable referenced from Project Settings (UYogSettings::CameraShakeLevelTable),
 * played through AYogPlayerCameraManager::PlayShakeLevel.
 */
USTRUCT(BlueprintType)
struct FCameraShakeLevelRow : public FTableRowBase
{
	GENERATED_BODY()

	// Integer level this row represents. Looked up by AYogPlayerCameraManager::PlayShakeLevel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0"))
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> ShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0.0"))
	float Scale = 1.f;
};
