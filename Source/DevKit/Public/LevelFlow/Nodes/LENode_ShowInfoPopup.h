#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_ShowInfoPopup.generated.h"

class ULevelInfoPopupDA;

/**
 * 关卡事件节点：显示轻量信息提示浮窗（不暂停游戏）
 * In       → 立即显示浮窗，同时触发 Out（非阻塞）
 * Out      → 立即触发，可串联后续节点（不等弹窗关闭）
 * OnClosed → 弹窗自动关闭后触发（需等待 DA.DisplayDuration 秒）
 *
 * 两个输出引脚各有用途：
 *   非阻塞流程 → 接 Out
 *   等弹窗结束再继续 → 接 OnClosed
 */
UCLASS(meta = (DisplayName = "Show Info Popup"))
class DEVKIT_API ULENode_ShowInfoPopup : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	/** 提示内容 DA（DA_InfoPopup_XXX，只需填文字） */
	UPROPERTY(EditAnywhere, Category = "InfoPopup")
	TObjectPtr<ULevelInfoPopupDA> PopupDA;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FDelegateHandle ClosedHandle;
};
