#pragma once

#include "CoreMinimal.h"
#include "GameModes/LevelFlowTypes.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "Story/StoryRewardOverrideTypes.h"
#include "SNode_SetRoomRewardOverride.generated.h"

class AYogGameMode;
class UYogGameInstanceBase;

/**
 * 覆盖当前房间的奖励池，或清除已有的覆盖。
 * 等价于 EStoryEncounterActionKind::SetRoomRewardOverride 在 Story FA 中的节点形式。
 */
UCLASS(meta = (DisplayName = "Set Room Reward Override"))
class DEVKIT_API USNode_SetRoomRewardOverride : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Reward")
	EStoryRewardOverrideTarget OverrideTarget = EStoryRewardOverrideTarget::CurrentRoom;

	/** true = 清除已有覆盖，恢复默认奖励池（忽略 LootOptions）。 */
	UPROPERTY(EditAnywhere, Category = "Reward")
	bool bClearOverride = false;

	/** 覆盖后的奖励选项列表（bClearOverride = false 时生效）。 */
	UPROPERTY(EditAnywhere, Category = "Reward", meta = (EditCondition = "!bClearOverride"))
	TArray<FLootOption> LootOptions;

	bool ApplyRewardOverride(AYogGameMode* GameMode, UYogGameInstanceBase* GameInstance) const;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
