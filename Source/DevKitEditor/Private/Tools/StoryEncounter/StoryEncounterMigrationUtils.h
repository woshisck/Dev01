#pragma once

#include "CoreMinimal.h"

struct FStoryEncounterMigrationResult
{
	int32 ScannedTriggers = 0;
	int32 MigratedTriggers = 0;
	int32 SkippedTriggers = 0;
	int32 FailedTriggers = 0;
	FString GraphPath;
	TArray<FString> Messages;

	FText ToStatusText() const;
};

class FStoryEncounterMigrationUtils
{
public:
	static FStoryEncounterMigrationResult MigrateCurrentLevelTriggers();
};
