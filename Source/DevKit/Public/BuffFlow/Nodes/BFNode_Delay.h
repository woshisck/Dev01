#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_Delay.generated.h"

/**
 * 等待 N 秒后继续执行。
 *
 * 不继承 BFNode_Base（无需 GAS），直接继承 UFlowNode。
 *
 * 输入引脚：
 *   In     — 启动计时
 *   Cancel — 取消计时（如果计时器仍在运行）
 *
 * 输出引脚：
 *   Completed — 等待时间结束后触发
 *   Cancelled — Cancel 引脚触发时触发
 *
 * Duration 支持数据引脚连线（可从 GetAttribute、LiteralFloat 等节点动态传入）。
 *
 * 典型用途：
 *   [OnKill] → [Delay: 2s] → [ApplyAttributeModifier: ...]  （延迟2秒后触发效果）
 *   [OnDash] → [Delay]
 *                 Completed → [ApplyAttributeModifier: SpeedBoost]
 *   触发 Cancel 引脚可提前中止计时。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Delay", Category = "BuffFlow|Utility"))
class DEVKIT_API UBFNode_Delay : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	/**
	 * 等待时间（秒）。
	 * 支持数据引脚连线——可从 LiteralFloat 或 GetAttribute 动态传入。
	 */
	UPROPERTY(EditAnywhere, Category = "Delay", meta = (ClampMin = "0.0"))
	FFlowDataPinInputProperty_Float Duration;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FTimerHandle DelayTimer;
};
