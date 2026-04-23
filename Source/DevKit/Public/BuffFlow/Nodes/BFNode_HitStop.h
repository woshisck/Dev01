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
 * 消费（移除）Tag 在实际触发前完成，保证单次命中只触发一次。
 *
 * 典型 FA 接线：
 *   [OnDamageDealt] → [BFNode_HitStop]
 *
 * 典型暴击 FA 写 Tag：
 *   [OnCritHit] → [AddTag: Buff.Status.HitStop.Freeze]
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Hit Stop (Montage)", Category = "BuffFlow|Combat"))
class DEVKIT_API UBFNode_HitStop : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// ── 冻结阶段 ──────────────────────────────────────────────────────

	/** 蒙太奇暂停时长（真实秒）。仅在 ASC 持有 Buff.Status.HitStop.Freeze 时生效 */
	UPROPERTY(EditAnywhere, Category = "HitStop|Freeze",
		meta = (ClampMin = 0.01f, ClampMax = 0.3f))
	float FrozenDuration = 0.06f;

	// ── 减速阶段 ──────────────────────────────────────────────────────

	/** 蒙太奇减速持续时长（真实秒）。仅在 ASC 持有 Buff.Status.HitStop.Slow 时生效 */
	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 0.0f, ClampMax = 0.5f))
	float SlowDuration = 0.12f;

	/** 减速阶段蒙太奇播放速率（0.01~1.0，例如 0.3 = 30%）*/
	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 0.01f, ClampMax = 1.0f))
	float SlowRate = 0.3f;

	/** 追帧阶段播放速率（>1，例如 2.0）；持续时间由引擎自动计算以精确补偿慢放丢失帧 */
	UPROPERTY(EditAnywhere, Category = "HitStop|Slow",
		meta = (ClampMin = 1.01f, ClampMax = 5.0f))
	float CatchUpRate = 2.0f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
