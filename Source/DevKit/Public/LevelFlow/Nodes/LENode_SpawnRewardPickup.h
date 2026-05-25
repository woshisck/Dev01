#pragma once

#include "CoreMinimal.h"
#include "GameModes/LevelFlowTypes.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_SpawnRewardPickup.generated.h"

class ARewardPickup;

/**
 * Spawns a fixed reward pickup from the StoryFlowProxy context transform.
 */
UCLASS(meta = (DisplayName = "Spawn Reward Pickup"))
class DEVKIT_API ULENode_SpawnRewardPickup : public ULENode_Base
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

	bool SpawnRewardPickupAtContext(UWorld* World, const FTransform& ContextTransform) const;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	UWorld* ResolveWorld() const;
	FTransform ResolveContextTransform() const;
};
