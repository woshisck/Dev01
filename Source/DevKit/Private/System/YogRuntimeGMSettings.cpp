#include "System/YogRuntimeGMSettings.h"

UYogRuntimeGMSettings::UYogRuntimeGMSettings()
{
	CategoryName = TEXT("Game");
}

FName UYogRuntimeGMSettings::GetCategoryName() const
{
	return TEXT("Game");
}
