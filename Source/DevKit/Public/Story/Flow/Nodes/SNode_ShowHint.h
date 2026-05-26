#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_ShowHint.generated.h"

/**
 * 在底部提示条显示一行提示文字（非阻塞，立即触发 Out）。
 */
UCLASS(meta = (DisplayName = "Show Hint"))
class DEVKIT_API USNode_ShowHint : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 可选标题。空 = 无标题（等价于 WeakHint 样式）；有内容 = 带标题的 InfoHint。 */
	UPROPERTY(EditAnywhere, Category = "Hint")
	FText HintTitle;

	UPROPERTY(EditAnywhere, Category = "Hint", meta = (MultiLine = true))
	FText HintText;

	UPROPERTY(EditAnywhere, Category = "Hint|Input")
	bool bUseInputTextVariants = false;

	UPROPERTY(EditAnywhere, Category = "Hint|Input", meta = (MultiLine = true, EditCondition = "bUseInputTextVariants"))
	FText KeyboardMouseHintText;

	UPROPERTY(EditAnywhere, Category = "Hint|Input", meta = (MultiLine = true, EditCondition = "bUseInputTextVariants"))
	FText GamepadHintText;

	/** 显示时长（秒）。0 = 常驻，直到被新提示替换。 */
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (ClampMin = "0.0"))
	float Duration = 3.0f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
