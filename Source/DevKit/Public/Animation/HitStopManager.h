#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "HitStopManager.generated.h"

/**
 * 命中停顿 & 全局时间缩放管理器。
 * 作为 UTickableWorldSubsystem 自动随世界创建，无需手动管理生命周期。
 *
 * 工作流：
 *   冻结阶段（dilation ≈ 0）→ 慢动作阶段（dilation = SlowAmount）→ 恢复
 *   任一阶段时长为 0 时跳过。
 *
 * 调用方式：
 *   World->GetSubsystem<UHitStopManager>()->RequestHitStop(...)
 */
UCLASS()
class DEVKIT_API UHitStopManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 请求命中停顿。
	 * @param FrozenDuration  冻结阶段时长（真实秒）。0 = 跳过，直接进慢动作
	 * @param SlowDuration    慢动作阶段时长（真实秒）。0 = 跳过，直接恢复
	 * @param SlowDilation    慢动作阶段时间缩放（0.1 = 10% 速度）
	 */
	void RequestHitStop(float FrozenDuration, float SlowDuration, float SlowDilation);

	// USubsystem
	virtual void Deinitialize() override;

	// FTickableGameObject — 每帧用真实时间检查阶段转换
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UHitStopManager, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return CurrentPhase != EPhase::None; }

private:
	enum class EPhase : uint8 { None, Frozen, Slow };

	EPhase  CurrentPhase       = EPhase::None;
	double  PhaseStartRealTime = 0.0;
	float   CachedFrozenDur   = 0.0f;
	float   CachedSlowDur     = 0.0f;
	float   CachedSlowDilation = 0.3f;
	float   SavedTimeDilation  = 1.0f;

	void TransitionToSlow();
	void EndHitStop();
	void ApplyDilation(float Dilation);
};
