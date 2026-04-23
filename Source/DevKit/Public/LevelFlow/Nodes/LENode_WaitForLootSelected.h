#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_WaitForLootSelected.generated.h"

/**
 * 关卡事件节点：等待玩家选定符文
 * In  → 开始监听 YogGameMode::OnLootSelected
 * OnSelected → 玩家选定符文后触发（用于串联后续节点，如显示提示浮窗）
 */
UCLASS(meta = (DisplayName = "Wait For Loot Selected"))
class DEVKIT_API ULENode_WaitForLootSelected : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FDelegateHandle LootSelectedHandle;
};
