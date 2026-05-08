#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.h"
#include "YogRuneFlowAsset.generated.h"

/**
 * Flow asset type used by the generic Rune Editor.
 *
 * Display name: Yog Rune Flow Graph
 */
UCLASS(BlueprintType, meta = (DisplayName = "Yog Rune Flow Graph"))
class DEVKIT_API UYogRuneFlowAsset : public UFlowAsset
{
	GENERATED_BODY()

public:
	UYogRuneFlowAsset(const FObjectInitializer& ObjectInitializer);
};
