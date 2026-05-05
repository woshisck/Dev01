#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_PlayerDash.generated.h"

class AActor;
class ACharacter;
class UPrimitiveComponent;
struct FCollisionQueryParams;
struct FCollisionShape;

/**
 * 玩家冲刺 GA（C++ 实现）。
 *
 * 策划工作流：
 * 1. 新建 Blueprint GA，Parent Class 选 GA_PlayerDash
 * 2. GAS Tag 配置（在 Blueprint Class Defaults 中填写）：
 *      AbilityTags                 = PlayerState.AbilityCast.Dash.Dash1
 *      ActivationOwnedTags         = Buff.Status.DashInvincible
 *      ActivationBlockedTags       = Buff.Status.Dead, PlayerState.AbilityCast.Dash
 *      CancelAbilitiesWithTag      = PlayerState.AbilityCast（取消当前攻击等动作 GA）
 * 3. 蒙太奇在角色 CharacterData → AbilityData 里配置（Key = PlayerState.AbilityCast.Dash.Dash1）
 * 4. 充能次数 / CD 通过 GAS 属性控制（PlayerAttributeSet.MaxDashCharge / DashCooldownDuration）
 *    初始值在角色 CharacterData 的属性表中填写
 *
 * 功能：
 * - 方向：冲刺前 Controller 已将角色旋转至摇杆方向，GA 直接沿 ActorForwardVector 冲
 * - 位移：AnimRootMotionTranslationScale 驱动（Scale = EffectiveDist / DashMontageRootMotionLength）
 * - 越障：从冲刺终点逐步向前延伸（6步×50cm），找到无 DashTrace 阻挡的落点则延伸冲刺穿越
 * - 穿透：冲刺期间 Capsule 对 Enemy / DashThrough 通道设为 Overlap，全程无刚体阻挡
 * - 无敌帧：ActivationOwnedTags 授予 Buff.Status.DashInvincible，GA 结束自动移除
 * - 次数/CD：走 SkillChargeComponent（不走 GAS CommitAbility），支持多段冲刺和符文改 CD
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_PlayerDash : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PlayerDash();

	// ── 策划配置 ─────────────────────────────────────────────────────────────

	/** 最远冲刺距离（cm）。*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
	float DashMaxDistance = 600.f;

	/** SphereTrace 半径（应接近角色 Capsule 半径）。*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "10.0", ClampMax = "100.0"))
	float DashCapsuleRadius = 35.f;

	/** 冲刺蒙太奇根运动总长度（cm）。Scale = DashMaxDistance / DashMontageRootMotionLength。*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "1.0"))
	float DashMontageRootMotionLength = 600.f;


	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

private:
	/**
	 * 计算实际冲刺距离。从满距终点向前逐步延伸寻找可落点（越障），
	 * 或在终点附近遇硬墙时返回停止距离。返回值用于计算 AnimScale。
	 */
	float GetFurthestValidDashDistance(const FVector& Start, const FVector& End);

	bool FindFirstDashBlockingHit(
		const FVector& SweepStart,
		const FVector& SweepEnd,
		const FCollisionShape& Shape,
		const FCollisionQueryParams& Params,
		FHitResult& OutHit) const;

	float FindAirWallExitDistance(
		const FVector& SweepStart,
		const FVector& DashDirection,
		const UPrimitiveComponent* AirWallComponent) const;

	float GetDashStopDistance(float HitDistance) const;

	/**
	 * 判断该位置是否为有效冲刺落点：命中列表为空，或所有命中体对 DashTrace 均非 Block。
	 */
	/** 修改/恢复 Capsule 对 Enemy 和 DashThrough 通道的碰撞响应。*/
	void SetDashCollision(ACharacter* Character, ECollisionResponse Response) const;

	void ApplyDashMoveIgnores(ACharacter* Character);
	void ClearDashMoveIgnores(ACharacter* Character);
	bool HasEnemyOverlapAt(ACharacter* Character, const FVector& Location) const;
	void ResolveEnemyOverlapAfterDash(ACharacter* Character) const;

	/** 每 0.1s 打印冲刺充能/CD 调试信息到屏幕 */
	void PrintDashDebugInfo();

	FTimerHandle DebugPrintTimer;

	/** ActivateAbility 时记录起始位置，EndAbility 时用于绘制真实冲刺距离线 */
	FVector DashDebugStartLocation;

	/** ActivateAbility 时记录 AnimScale，EndAbility 时用于 Z 下沉诊断 */
	float DashAnimScale = 1.f;

	FVector LastDashDirection = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> DashIgnoredActors;

	/**
	 * CanActivateAbility 检测到处于 X-1 招位时缓存应注入的连招 Tag。
	 * mutable：const 方法内写入，ActivateAbility/EndAbility 时读取。
	 * 非桥接位冲刺时为空容器。
	 */
	mutable FGameplayTagContainer PendingSaveComboTags;

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
