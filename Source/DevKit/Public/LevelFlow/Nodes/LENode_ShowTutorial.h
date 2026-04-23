#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_ShowTutorial.generated.h"

/**
 * 关卡事件节点：显示教程弹窗
 * In  → 调 TutorialManager::ShowByEventID，弹窗显示（游戏暂停）
 * OnClosed → 玩家关闭弹窗后触发（用于串联后续节点）
 */
UCLASS(meta = (DisplayName = "Show Tutorial Popup"))
class DEVKIT_API ULENode_ShowTutorial : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	// 对应 DialogContentDA 里配置的 EventID
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName EventID = TEXT("WeaponTutorial");

	// 取消勾选 = 纯信息浮窗（不暂停游戏，不屏蔽输入）
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	bool bPauseGame = true;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FDelegateHandle PopupClosedHandle;
};
