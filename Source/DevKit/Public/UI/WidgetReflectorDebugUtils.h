#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"

class UWidget;

namespace YogWidgetReflectorDebug
{
	DEVKIT_API bool ShouldMakeWidgetsPickable();
	DEVKIT_API ESlateVisibility GetInspectableVisibility(ESlateVisibility Visibility);
	DEVKIT_API void ApplyToWidgetTree(UWidget* Widget);
}
