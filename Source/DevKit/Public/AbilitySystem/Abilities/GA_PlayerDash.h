#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_PlayerDash.generated.h"

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
 * - 位移：AnimRootMotionTranslationScale 驱动（Scale = DashMaxDistance / DashMontageRootMotionLength）
 * - 越障：前向+后向 SphereTrace 检测薄墙（≤MaxTraversableWallThickness），满足条件则瞬传到墙出口
 * - 穿透：冲刺期间 Capsule 对 Enemy / DashThrough 通道设为 Overlap
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

	/** 越障：可穿越墙体的最大厚度（cm）。超过此厚度则在墙前停止。*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.0"))
	float MaxTraversableWallThickness = 400.f;

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
	 * 计算冲刺目标：前向扫描找墙，后向扫描找墙出口，决定是越障还是在墙前停止。
	 * @param bOutIsTraversal  true=越障（需要 SetActorLocation + AnimScale=0）
	 * @param OutTraversalEnd  越障落点（仅 bOutIsTraversal=true 时有效）
	 * @param OutAnimScale     AnimRootMotionTranslationScale（越障时为 0）
	 */
	void ComputeDashTarget(
		const FVector& Start,
		const FVector& Direction,
		bool&    bOutIsTraversal,
		FVector& OutTraversalEnd,
		float&   OutAnimScale) const;

	/** 修改/恢复 Capsule 对 Enemy 和 DashThrough 通道的碰撞响应。*/
	void SetDashCollision(ACharacter* Character, ECollisionResponse Response) const;

	/** 每 0.5s 打印冲刺充能/CD 调试信息到屏幕 */
	void PrintDashDebugInfo();

	FTimerHandle DebugPrintTimer;

	/** ActivateAbility 时记录起始位置，EndAbility 时用于绘制真实冲刺距离线 */
	FVector DashDebugStartLocation;

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
