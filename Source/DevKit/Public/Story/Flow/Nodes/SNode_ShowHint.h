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
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (MultiLine = true))
	FText HintText;

	/** 显示时长（秒）。0 = 无限，直到玩家离开区域。 */
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (ClampMin = "0.0"))
	float Duration = 3.0f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
