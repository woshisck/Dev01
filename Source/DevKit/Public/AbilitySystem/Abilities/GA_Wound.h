#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GA_Wound.generated.h"

/**
 * 伤口 GA（GameplayEvent 触发，施加在目标身上）
 *
 * 工作原理：
 *   - FA 先 ApplyEffect GE_Wound_Marker（HasDuration, GrantedTags=Buff.Status.Wounded,
 *     GrantedAbilities=GA_Wound）到目标
 *   - FA 再 SendGameplayEvent Buff.Event.Wound（Magnitude=每次额外伤害）到目标，激活此GA
 *   - GA 激活后监听 Ability.Event.Damaged（目标受到攻击时广播）
 *   - 每次受攻击触发：对自身施加 WoundDamageEffect（DamageBuff 路径，绕过护甲）
 *   - Buff.Status.Wounded Tag 消失时自动结束
 *
 * Blueprint 子类需配置：
 *   WoundDamageEffect = GE_WoundDamage（HasDuration:Instant, Modifier: DamageBuff=SetByCaller）
 */
UCLASS()
class DEVKIT_API UGA_Wound : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Wound(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** Blueprint 子类里设置为 GE_WoundDamage（Instant GE，DamageBuff Modifier） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wound")
	TSubclassOf<UGameplayEffect> WoundDamageEffect;

	/** 每次受击额外伤害（从 TriggerEventData.EventMagnitude 读取，fallback 此默认值） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wound")
	float DefaultExtraDamage = 10.f;

private:
	float ExtraDamage = 0.f;
	FDelegateHandle TagChangeDelegateHandle;

	TWeakObjectPtr<UYogAbilitySystemComponent> InstigatorASC;

	UFUNCTION()
	void OnDamageTaken(FGameplayEventData Payload);

	void OnWoundedTagChanged(const FGameplayTag Tag, int32 NewCount);
};
