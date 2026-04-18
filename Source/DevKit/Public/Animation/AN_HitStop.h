#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_HitStop.generated.h"

/**
 * 命中停顿 + 全局时间缩放 AnimNotify。
 * 放在蒙太奇命中帧，执行两阶段效果：
 *   1. 冻结阶段：全局时间缩放 ≈ 0，持续 FrozenDuration 真实秒
 *   2. 慢动作阶段：全局时间缩放 = SlowTimeDilation，持续 SlowDuration 真实秒
 * 任一阶段设为 0 即跳过。
 *
 * 典型配置：
 *   轻击  → FrozenDuration=0.05, SlowDuration=0,    SlowTimeDilation=0.3
 *   重击  → FrozenDuration=0.08, SlowDuration=0.12, SlowTimeDilation=0.25
 *   暴击  → FrozenDuration=0.06, SlowDuration=0.15, SlowTimeDilation=0.2
 */
UCLASS(meta = (DisplayName = "AN Hit Stop"))
class DEVKIT_API UAN_HitStop : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** 冻结阶段时长（真实秒）。0 = 跳过冻结 */
	UPROPERTY(EditAnywhere, Category = "HitStop", meta = (ClampMin = 0.0f, ClampMax = 0.3f))
	float FrozenDuration = 0.06f;

	/** 慢动作阶段时长（真实秒）。0 = 无慢动作 */
	UPROPERTY(EditAnywhere, Category = "HitStop", meta = (ClampMin = 0.0f, ClampMax = 0.5f))
	float SlowDuration = 0.0f;

	/** 慢动作阶段时间缩放（0.1 = 10% 速度，1.0 = 正常）*/
	UPROPERTY(EditAnywhere, Category = "HitStop", meta = (ClampMin = 0.01f, ClampMax = 1.0f))
	float SlowTimeDilation = 0.3f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
