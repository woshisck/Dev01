#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_ShowTutorial.generated.h"

/**
 * 关卡事件节点：显示教程弹窗
 * In → 触发 TutorialManager 按 EventID 显示弹窗 → （弹窗关闭后无回调，直接 Out）
 */
UCLASS(meta = (DisplayName = "Show Tutorial Popup"))
class DEVKIT_API ULENode_ShowTutorial : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	// 对应 DialogContentDA 里配置的 EventID
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName EventID = TEXT("WeaponTutorial");

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
