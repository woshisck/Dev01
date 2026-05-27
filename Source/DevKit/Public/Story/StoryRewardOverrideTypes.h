#pragma once

#include "CoreMinimal.h"
#include "StoryRewardOverrideTypes.generated.h"

UENUM(BlueprintType)
enum class EStoryRewardOverrideTarget : uint8
{
	CurrentRoom UMETA(DisplayName = "Current Room"),
	NextRoom UMETA(DisplayName = "Next Room"),
};
