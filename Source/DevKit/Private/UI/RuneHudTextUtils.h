#pragma once

#include "CoreMinimal.h"

class ULevelInfoPopupDA;
struct FRuneConfig;

namespace RuneHudTextUtils
{
	FText GetRuneHudSummary(const FRuneConfig& Config, int32 MaxCharacters);
	FText GetLevelInfoHudSummary(const ULevelInfoPopupDA& PopupData, int32 MaxCharacters);
}
