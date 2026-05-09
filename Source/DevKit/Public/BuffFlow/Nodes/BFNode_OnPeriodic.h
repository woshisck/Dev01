#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnPeriodic.generated.h"

/**
 * 周期触发节点
 * In   — 开始计时（每隔 Interval 秒触发一次 Tick 输出）
 * Stop — 停止计时
 * Tick — 每个周期触发一次（不结束 Flow，持续循环直到 Stop 或 Flow 结束）
 *
 * 适用场景：流血移动扣血检测、持续燃烧伤害等需要轮询的效果
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Periodic", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_OnPeriodic : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 触发间隔（秒）— 每隔多少秒触发一次 Tick 输出
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (ClampMin = "0.1", DisplayName = "触发间隔（秒）"))
	float Interval = 1.0f;

	// 立即触发第一次 — true=In 触发时立即执行一次，false=等待第一个间隔后再执行
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "立即触发第一次"))
	bool bFireImmediately = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	UFUNCTION()
	void OnTimerTick();

	FTimerHandle TimerHandle;
};
