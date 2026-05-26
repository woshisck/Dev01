#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_TutorialAreaHint.generated.h"

/**
 * 在 HUD 底部显示一条持续性区域提示（Duration = 0 表示常驻，直到被清除）。
 * 适用于"进入某区域后持续展示的操作说明"场景。
 * 等价于 EStoryEncounterActionKind::TutorialAreaHint 在 Story FA 中的节点形式。
 */
UCLASS(meta = (DisplayName = "Tutorial Area Hint"))
class DEVKIT_API USNode_TutorialAreaHint : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 提示文本（多行）。 */
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (MultiLine = true))
	FText HintText;

	UPROPERTY(EditAnywhere, Category = "Hint|Input")
	bool bUseInputTextVariants = false;

	UPROPERTY(EditAnywhere, Category = "Hint|Input", meta = (MultiLine = true, EditCondition = "bUseInputTextVariants"))
	FText KeyboardMouseHintText;

	UPROPERTY(EditAnywhere, Category = "Hint|Input", meta = (MultiLine = true, EditCondition = "bUseInputTextVariants"))
	FText GamepadHintText;

	/**
	 * 显示时长（秒）。
	 * 0 = 常驻，直到同一 HUD 槽位被新提示替换或手动清除；>0 = 到时自动消失。
	 */
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
