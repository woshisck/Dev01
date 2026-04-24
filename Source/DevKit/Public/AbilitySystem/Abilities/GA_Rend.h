#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GA_Rend.generated.h"

/**
 * 撕裂 GA（施加在目标身上）
 *
 * 移动距离掉血：每移动 100 单位触发一次额外伤害（DamageBuff，绕过护甲）。
 * 原地静止 2 秒后自动消失（Buff.Status.Rended Tag 也会随 GE_Rend_Marker 一起消失）。
 *
 * Blueprint 子类需配置：
 *   RendDamageEffect = GE_RendDamage（Instant, DamageBuff SetByCaller）
 */
UCLASS()
class DEVKIT_API UGA_Rend : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Rend(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** Damage GE（Instant，DamageBuff，用 SetByCaller 传入伤害量） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rend")
	TSubclassOf<UGameplayEffect> RendDamageEffect;

	/** 每移动此距离触发一次扣血（默认 100 单位） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rend")
	float DamagePerUnits = 100.f;

	/** Tick 间隔（秒），越小越精确，默认 0.2s */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rend")
	float TickInterval = 0.2f;

	/** 静止超过此时间（秒）后自动结束（默认 2.0s） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rend")
	float StationaryTimeout = 2.f;

	/** 每次触发的伤害量（从 TriggerEventData.EventMagnitude 读取，fallback 此默认值） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rend")
	float DefaultDamagePerTrigger = 15.f;

private:
	float DamagePerTrigger   = 0.f;
	float AccumulatedDistance = 0.f;
	float StationaryTimer    = 0.f;
	FVector LastPosition;
	FDelegateHandle TagChangeDelegateHandle;
	FTimerHandle TickTimerHandle;

	TWeakObjectPtr<UYogAbilitySystemComponent> InstigatorASC;

	void RendTick();
	void OnRendTagChanged(const FGameplayTag Tag, int32 NewCount);
};
