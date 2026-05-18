#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "HitStopManager.generated.h"

class UAnimInstance;

UENUM(BlueprintType)
enum class EHitStopScope : uint8
{
	SelfMontage    UMETA(DisplayName = "Self Montage"),
	GlobalDilation UMETA(DisplayName = "Global Dilation"),
};

/**
 * 命中停顿管理器（WorldSubsystem，自动随世界创建）。
 *
 * 操作玩家蒙太奇的播放状态，分三阶段：
 *   Frozen  → Montage_Pause，持续 FrozenDuration 真实秒后恢复
 *   Slow    → Montage_SetPlayRate(SlowRate)，持续 SlowDuration 真实秒
 *   CatchUp → Montage_SetPlayRate(CatchUpRate)，自动计算时长后恢复 1.0
 * 任意阶段时长为 0 则跳过。
 *
 * 调用方式（通过 BFNode_HitStop FA 节点间接调用）：
 *   World->GetSubsystem<UHitStopManager>()->RequestMontageHitStop(...)
 */
UCLASS()
class DEVKIT_API UHitStopManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 请求蒙太奇命中停顿。
	 * @param AnimInst       目标角色的 AnimInstance
	 * @param FrozenDuration 暂停时长（真实秒）。0 = 跳过冻结阶段
	 * @param SlowDuration   减速时长（真实秒）。0 = 跳过减速阶段
	 * @param SlowRate       减速阶段播放速率（0.01~1.0）
	 * @param CatchUpRate    追帧阶段播放速率（>1），持续时间自动计算
	 */
	void RequestMontageHitStop(UAnimInstance* InAnimInst,
		float FrozenDuration, float SlowDuration = 0.f,
		float SlowRate = 0.3f, float CatchUpRate = 2.0f,
		EHitStopScope Scope = EHitStopScope::SelfMontage);

	// USubsystem
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UHitStopManager, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return Phase != EPhase::None; }

private:
	enum class EPhase : uint8 { None, Frozen, Slow, CatchUp };

	EPhase  Phase              = EPhase::None;
	double  PhaseStartRealTime = 0.0;
	float   CachedFrozenDur   = 0.f;
	float   CachedSlowDur     = 0.f;
	float   CachedSlowRate    = 0.3f;
	float   CachedCatchUpRate = 2.0f;
	float   CachedCatchUpDur  = 0.f;

	TWeakObjectPtr<UAnimInstance> AnimInst;
	EHitStopScope CachedScope = EHitStopScope::SelfMontage;

	void TransitionToSlow();
	void TransitionToCatchUp();
	void EndHitStop();
	void SetPlayRate(float Rate);
	void PauseMontage();
	void ResumeMontage();
};
