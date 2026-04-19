#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_Delay.generated.h"

/**
 * 关卡事件节点：真实时间延迟（不受 TimeDilation 影响）
 * In → 等待 Duration 真实秒 → Out
 */
UCLASS(meta = (DisplayName = "Delay (Real Time)"))
class DEVKIT_API ULENode_Delay : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	// 等待的真实秒数（不受全局时间膨胀影响）
	UPROPERTY(EditAnywhere, Category = "Delay", meta = (ClampMin = "0.01"))
	float Duration = 1.0f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FTSTicker::FDelegateHandle TickerHandle;
};
