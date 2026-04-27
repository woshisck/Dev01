#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "UI/GameDialogWidget.h"
#include "LENode_ShowTutorial.generated.h"

/**
 * 关卡事件节点：显示教程弹窗
 * In  → 显示弹窗（优先用 InlinePages，为空则查 EventID 在 TutorialRegistry 里的内容 DA）
 * OnClosed → 玩家关闭弹窗后触发（用于串联后续节点）
 *
 * 两种填写方式（互斥，InlinePages 优先）：
 *   A) InlinePages 里直接填写标题/正文 — 不依赖任何 DA，适合关卡内一次性提示
 *   B) EventID → 由 BP_HUD 上的 TutorialRegistry 查表获取内容 DA — 内容复用 / 集中管理
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
	 * 在 TutorialRegistry 里查找内容 DA 的 Key（InlinePages 为空时生效）。
	 * 对应 BP_HUD → TutorialRegistry → Entries 的 TMap Key。
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
