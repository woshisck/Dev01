#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_Fork.generated.h"

/**
 * 分叉节点：一个输入触发多个并行输出。
 * 所有输出在同一帧内按顺序触发，适用于从一个时机启动多条独立异步流（如多个 WaitGameplayEvent 循环）。
 * 输出数量可在 Details 面板调整（2-4），调整后需重新连线。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Fork", Category = "BuffFlow|Flow"))
class DEVKIT_API UBFNode_Fork : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 输出引脚数量（调整后重新连线）
	UPROPERTY(EditAnywhere, Category = "Fork", meta = (ClampMin = "2", ClampMax = "4", DisplayName = "输出数量"))
	int32 OutputCount = 2;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void RebuildOutputPins();
};
