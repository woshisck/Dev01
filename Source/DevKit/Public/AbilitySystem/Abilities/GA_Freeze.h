#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_Freeze.generated.h"

/**
 * 冻结 GA（施加在目标身上）
 *
 * - 激活时立即施加 ChillEffect（减速 GE）
 * - 记录触发位置 OriginLocation
 * - 3 秒后检查：若距 OriginLocation < RequiredDistance(800)
 *   → 移除 ChillEffect，施加 FrozenStunEffect（眩晕 + 惩罚伤害）
 * - 若成功逃脱（>= 800 单位）→ 仅移除 ChillEffect，GA 结束
 *
 * Blueprint 子类需配置：
 *   ChillEffect      = GE_Chill（减速，Infinite，结束时自动移除）
 *   FrozenStunEffect = GE_FrozenStun（眩晕 + DamageBuff，HasDuration）
 *   PenaltyDamageEffect = GE 惩罚伤害（可复用 FrozenStunEffect）
 */
UCLASS()
class DEVKIT_API UGA_Freeze : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Freeze(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** 减速 GE（施加在目标上，Infinite，EndAbility 时移除） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Freeze")
	TSubclassOf<UGameplayEffect> ChillEffect;

	/** 冻结眩晕 GE（HasDuration，GrantedTags=Buff.Status.Frozen+Character.State.Stunned） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Freeze")
	TSubclassOf<UGameplayEffect> FrozenStunEffect;

	/** 需要逃离的最小距离（cm，默认 800） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Freeze")
	float RequiredDistance = 800.f;

	/** 检测时间（秒，默认 3s） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Freeze")
	float FreezeDuration = 3.f;

	/** 惩罚伤害量（从 TriggerEventData.EventMagnitude 读取） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Freeze")
	float DefaultPenaltyDamage = 30.f;

private:
	FVector OriginLocation;
	float PenaltyDamage = 0.f;
	FActiveGameplayEffectHandle ChillHandle;
	FTimerHandle CheckTimerHandle;

	void OnFreezeTimeExpired();
};
