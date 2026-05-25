#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_UnlockFeature.generated.h"

/**
 * 解锁一个游戏功能（冲刺、背包、特定界面等），写入元进度存档。
 * 等价于 EStoryEncounterActionKind::UnlockFeature 在 Story FA 中的节点形式。
 */
UCLASS(meta = (DisplayName = "Unlock Feature"))
class DEVKIT_API USNode_UnlockFeature : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 要解锁的功能 Tag（例如 Feature.Sprint / Feature.Inventory）。 */
	UPROPERTY(EditAnywhere, Category = "Feature", meta = (Categories = "Feature"))
	FGameplayTag FeatureTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
