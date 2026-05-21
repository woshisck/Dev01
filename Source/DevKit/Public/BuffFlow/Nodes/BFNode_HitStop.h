#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_HitStop.generated.h"

/**
 * 蒙太奇命中停顿节点（Tag 驱动）。
 *
 * 执行时读取 BuffOwner ASC 上的 HitStop Tag，决定触发哪些阶段：
 *   Buff.Status.HitStop.Freeze → 暂停蒙太奇 FrozenDuration 真实秒后恢复
 *   Buff.Status.HitStop.Slow  → 以 SlowRate 减速蒙太奇 SlowDuration 真实秒，
 *                               之后自动追帧（CatchUpRate）
 *
 * 两个 Tag 可同时存在，执行顺序：Freeze → Slow → CatchUp。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Hit Stop (Montage)", Category = "BuffFlow|Combat"))
class DEVKIT_API UBFNode_HitStop : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 冻结持续时间（真实秒）— 仅在 ASC 持有 Buff.Status.HitStop.Freeze 时生效
	UPROPERTY(EditAnywhere, Category = "HitStop|Freeze",
		meta = (ClampMin = 0.01f, ClampMax = 0.3f, DisplayName = "冻结持续时间（秒）"))
	float FrozenDuration = 0.06f;

	// 减速持续时间（真实秒）— 仅在 ASC 持有 Buff.Status.HitStop.Slow 时生效
	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 0.0f, ClampMax = 0.5f, DisplayName = "减速持续时间（秒）"))
	float SlowDuration = 0.12f;

	// 减速播放速率 — 减速阶段蒙太奇播放速率（0.01~1.0，例如 0.3 = 30%）
	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 0.01f, ClampMax = 1.0f, DisplayName = "减速播放速率"))
	float SlowRate = 0.3f;

	// 追帧播放速率 — 减速结束后追帧阶段的播放速率（> 1.0，例如 2.0）
	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 1.01f, ClampMax = 5.0f, DisplayName = "追帧播放速率"))
	float CatchUpRate = 2.0f;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
