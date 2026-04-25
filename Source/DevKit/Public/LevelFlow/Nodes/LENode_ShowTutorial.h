#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "UI/GameDialogWidget.h"
#include "LENode_ShowTutorial.generated.h"

/**
 * 关卡事件节点：显示教程弹窗
 * In  → 显示弹窗（优先用 InlinePages，为空则查 EventID + DialogContentDA）
 * OnClosed → 玩家关闭弹窗后触发（用于串联后续节点）
 *
 * 两种填写方式（互斥，InlinePages 优先）：
 *   A) InlinePages 里直接填写标题/正文 — 不依赖 DA，适合关卡内一次性提示
 *   B) EventID → DialogContentDA  — 内容复用，适合多处引用同一套文字
 */
UCLASS(meta = (DisplayName = "Show Tutorial Popup"))
class DEVKIT_API ULENode_ShowTutorial : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	/**
	 * 内联页面内容（优先级高于 EventID）。
	 * 直接在此数组里填写 Title / Body / SubText / Illustration。
	 * 数组非空时忽略 EventID，不查 DialogContentDA。
	 */
	UPROPERTY(EditAnywhere, Category = "Tutorial|内容")
	TArray<FTutorialPage> InlinePages;

	/**
	 * DialogContentDA 里的 Key（InlinePages 为空时生效）。
	 * 对应 DA 中 Pages Map 的 EventID 字段。
	 */
	UPROPERTY(EditAnywhere, Category = "Tutorial|内容")
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
