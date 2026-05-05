#pragma once

#include "CoreMinimal.h"
#include "CommonTextBlock.h"
#include "YogCommonUITextBlock.generated.h"

/**
 * CommonUI text block used by generated Yog UI widgets.
 * This keeps designer/runtime text widgets on the project's CommonUI path.
 */
UCLASS(meta = (DisplayName = "Yog Common UI Text Block"))
class DEVKIT_API UYogCommonUITextBlock : public UCommonTextBlock
{
	GENERATED_BODY()
};
