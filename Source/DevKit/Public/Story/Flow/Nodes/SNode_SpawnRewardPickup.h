#pragma once

#include "CoreMinimal.h"
#include "GameModes/LevelFlowTypes.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_SpawnRewardPickup.generated.h"

class ARewardPickup;

/**
 * 在 AStoryFlowProxy 的 ContextTransform 位置生成奖励拾取物（Story FA 版本的 LENode_SpawnRewardPickup）。
 */
UCLASS(meta = (DisplayName = "Spawn Reward Pickup"))
class DEVKIT_API USNode_SpawnRewardPickup : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Reward")
	TSubclassOf<ARewardPickup> RewardPickupClass;

	UPROPERTY(EditAnywhere, Category = "Reward")
	TArray<FLootOption> RewardLootOptions;

	UPROPERTY(EditAnywhere, Category = "Reward", meta = (ClampMin = "1"))
	int32 RewardPickupCount = 1;

	UPROPERTY(EditAnywhere, Category = "Reward")
	FVector RewardSpawnOffset = FVector(120.f, 0.f, 20.f);

	UPROPERTY(EditAnywhere, Category = "Reward")
	bool bAllowPickupOutsideArrangement = true;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	FTransform ResolveContextTransform() const;
};
