#pragma once

#include "CoreMinimal.h"

class UWorld;

namespace YogLookDev
{
    // Creates only a missing, serialized LookDev light map. Authored maps are never replaced.
    bool BuildLightingLevel(UWorld* World, const FString& PresetId, FText& OutMessage);
}
