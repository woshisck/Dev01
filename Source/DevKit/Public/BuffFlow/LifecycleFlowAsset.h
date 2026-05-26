#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.h"
#include "LifecycleFlowAsset.generated.h"

UCLASS(BlueprintType, meta = (DisplayName = "Spawn Lifecycle Flow Asset"))
class DEVKIT_API USpawnLifecycleFlowAsset : public UFlowAsset
{
	GENERATED_BODY()
};

UCLASS(BlueprintType, meta = (DisplayName = "Death Lifecycle Flow Asset"))
class DEVKIT_API UDeathLifecycleFlowAsset : public UFlowAsset
{
	GENERATED_BODY()
};
