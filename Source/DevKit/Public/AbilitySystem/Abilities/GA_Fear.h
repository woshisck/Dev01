#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_Fear.generated.h"

/**
 * 恐惧 GA（施加在敌人身上）
 *
 * - 激活时记录触发位置 OriginLocation
 * - 每 0.2s 强制敌人向远离 Instigator 方向移动（AI MoveToLocation）
 * - 2 秒后检查：若距 OriginLocation < RequiredDistance(800) → 施加惩罚伤害
 * - 不适用于玩家（ApplicationBlockedTags = Character.State.IsPlayer，如有的话）
 *
 * Blueprint 子类需配置：
 *   PenaltyDamageEffect = GE 惩罚伤害（Instant DamageBuff）
 */
UCLASS()
class DEVKIT_API UGA_Fear : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Fear(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** 惩罚伤害 GE（Instant DamageBuff，需在 BP 子类设置） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	TSubclassOf<UGameplayEffect> PenaltyDamageEffect;

	/** 需要逃离的最小距离（单位：cm，默认 800） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float RequiredDistance = 800.f;

	/** 检测时间（秒，默认 2s） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float FearDuration = 2.f;

	/** 移动更新间隔（秒，默认 0.2s） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float MoveUpdateInterval = 0.2f;

	/** 惩罚伤害量（从 TriggerEventData.EventMagnitude 读取，fallback 此默认值） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float DefaultPenaltyDamage = 30.f;

private:
	FVector OriginLocation;
	FVector InstigatorLocation;
	float PenaltyDamage = 0.f;
	FDelegateHandle TagChangeDelegateHandle;
	FTimerHandle CheckTimerHandle;
	FTimerHandle MoveTimerHandle;

	void UpdateFearMovement();
	void OnFearTimeExpired();
	void OnFearTagChanged(const FGameplayTag Tag, int32 NewCount);
};
