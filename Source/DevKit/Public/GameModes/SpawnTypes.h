// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/EnemyData.h"
#include "SpawnTypes.generated.h"

/** One mob an AMobSpawner may produce. */
USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyEntry
{
	GENERATED_BODY()

	// Enemy data asset (Actor class, stats).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyData> EnemyData;

	// How many of this enemy may appear in the level, summed over every spawn. -1 means unlimited.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy",
		meta = (ClampMin = "-1"))
	int32 MaxCountPerLevel = -1;
};
