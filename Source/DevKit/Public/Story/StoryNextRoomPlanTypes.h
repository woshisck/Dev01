#pragma once

#include "CoreMinimal.h"
#include "Data/EnemyData.h"
#include "GameModes/LevelFlowTypes.h"
#include "StoryNextRoomPlanTypes.generated.h"

class URoomDataAsset;
class UNiagaraSystem;

/**
 * Story-authored plan for the next room offered by portals.
 * It lets StoryFlow keep portal choice, room data, rewards, and pre-rolled buffs together.
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FStoryNextRoomPlan
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room")
	bool bForceSinglePortal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room", meta = (ClampMin = "0"))
	int32 PortalIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room")
	TObjectPtr<URoomDataAsset> RoomDataOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Reward")
	bool bOverrideRewardOptions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Reward", meta = (EditCondition = "bOverrideRewardOptions"))
	TArray<FLootOption> RewardOptionsOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Buffs")
	bool bOverrideBuffs = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Buffs", meta = (EditCondition = "bOverrideBuffs"))
	TArray<FBuffEntry> BuffsOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Reward")
	bool bSuppressRoomClearRewardPickup = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Special Reward Enemy")
	bool bMarkLastEnemyAsSpecialRewardEnemy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Special Reward Enemy", meta = (EditCondition = "bMarkLastEnemyAsSpecialRewardEnemy"))
	TArray<FLootOption> SpecialRewardEnemyLootOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Next Room|Special Reward Enemy", meta = (EditCondition = "bMarkLastEnemyAsSpecialRewardEnemy"))
	TObjectPtr<UNiagaraSystem> SpecialRewardEnemyAuraFX = nullptr;

	bool HasAnyOverride() const
	{
		return bForceSinglePortal
			|| RoomDataOverride != nullptr
			|| bOverrideRewardOptions
			|| bOverrideBuffs
			|| bSuppressRoomClearRewardPickup
			|| bMarkLastEnemyAsSpecialRewardEnemy;
	}
};
