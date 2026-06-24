#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_HitStop.generated.h"

/**
 * Tag-driven montage hit stop node.
 *
 * Reads hit-stop tags on the BuffOwner ASC:
 *   Buff.HitStop.Freeze -> pause montage for FrozenDuration real seconds.
 *   Buff.HitStop.Slow -> slow montage by SlowRate for SlowDuration, then catch up.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Hit Stop (Montage)", Category = "BuffFlow|Combat"))
class DEVKIT_API UBFNode_HitStop : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "HitStop|Freeze",
		meta = (ClampMin = 0.01f, DisplayName = "Freeze Duration"))
	float FrozenDuration = 0.06f;

	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 0.0f, ClampMax = 0.5f, DisplayName = "Slow Duration"))
	float SlowDuration = 0.12f;

	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 0.01f, ClampMax = 1.0f, DisplayName = "Slow Rate"))
	float SlowRate = 0.3f;

	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 1.01f, ClampMax = 5.0f, DisplayName = "Catch Up Rate"))
	float CatchUpRate = 2.0f;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
