#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_TimeDilation.generated.h"

/**
 * 关卡事件节点：全局时间膨胀
 * In → 设置 TimeDilation，Duration 真实秒后恢复 → Out
 */
UCLASS(meta = (DisplayName = "Time Dilation"))
class DEVKIT_API ULENode_TimeDilation : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	// 目标时间膨胀倍率（0.1 = 慢动作 10%）
	UPROPERTY(EditAnywhere, Category = "TimeDilation", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float DilationScale = 0.1f;

	// 保持多少真实秒后恢复正常（使用 FTSTicker，不受 TimeDilation 影响）
	UPROPERTY(EditAnywhere, Category = "TimeDilation", meta = (ClampMin = "0.05"))
	float Duration = 0.35f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FTSTicker::FDelegateHandle TickerHandle;
};
