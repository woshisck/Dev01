#pragma once

#include "CoreMinimal.h"
#include "Map/FloorGen/FloorLayout.h"

DECLARE_LOG_CATEGORY_EXTERN(LogYogFloorGen, Log, All);

class UFloorGenConfig;

enum class EFloorGenFailure : uint8
{
	None,
	InvalidConfig,
	NotEnoughRooms,
	NoNodeForRule,
	NoRoomForRule,
	InvalidFootprint,
	InvalidSocket,
	PlacementBudgetExceeded,
	NoPlacementFound,
	ValidationFailed
};

struct DEVKIT_API FFloorGenStats
{
	// Seeds tried, the requested one included.
	int32 SeedAttempts = 0;

	// Grid placement attempts spent on the last seed tried.
	int32 PlacementAttempts = 0;

	double ElapsedMs = 0.0;

	// Reason the last seed tried failed. None when generation succeeded.
	EFloorGenFailure Failure = EFloorGenFailure::None;
	FString FailureDetail;

	FString DescribeFailure() const;
};

class DEVKIT_API FFloorGenerator
{
public:
	/**
	 * Builds a floor layout: room graph, grid placement, validation. The same seed and config always
	 * produce the same layout. Failed seeds are retried with derived seeds up to Config->MaxSeedRetries.
	 */
	static bool Generate(const UFloorGenConfig* Config, int32 Seed, FFloorLayout& OutLayout, FFloorGenStats& OutStats);
};
