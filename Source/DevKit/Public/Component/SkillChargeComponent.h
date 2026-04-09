#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "SkillChargeComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 单个技能的充能运行状态（不序列化，运行时生成）
 */
USTRUCT()
struct FSkillChargeState
{
	GENERATED_BODY()

	/** 当前可用格数 */
	int32 CurrentCharge = 0;

	/** 正在回复中的格数（用后触发队列计数） */
	int32 ChargesInRecovery = 0;

	/** 回复计时器（队列模式：单 Timer 依次处理每格） */
	FTimerHandle RecoveryTimer;

	/** GAS 属性引用，用于运行时动态读值（符文可修改） */
	FGameplayAttribute MaxChargeAttr;
	FGameplayAttribute CDDurationAttr;
};

/** 充能变化委托，供 UI 绑定 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChargeChanged, FGameplayTag, SkillTag, int32, NewCharge);

/**
 * USkillChargeComponent
 *
 * 统一管理所有技能的充能/CD，支持：
 *   - 单次 CD（MaxCharge=1）
 *   - 多格充能（MaxCharge>1）
 *   - 用后触发队列回复（每用 1 格启动独立 CD，顺序回复）
 *   - 运行时动态 CDR（符文改 CDDuration 属性，下次 tick 自动生效）
 *
 * 使用流程：
 *   1. BeginPlay 后 ASC 就绪：调用 InitWithASC()
 *   2. 每个技能调用 RegisterSkill() 注册
 *   3. GA.CanActivate → HasCharge()
 *   4. GA.Activate    → ConsumeCharge()
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API USkillChargeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillChargeComponent();

	/** BeginPlay 后、ASC 就绪时调用 */
	void InitWithASC(UAbilitySystemComponent* InASC);

	/**
	 * 注册一个技能的充能配置（InitWithASC 之后调用）
	 * @param SkillTag        技能的 Gameplay Tag（如 Ability.Dash）
	 * @param MaxChargeAttr   对应 GAS 属性（MaxDashCharge 等，符文可加层数）
	 * @param CDDurationAttr  对应 GAS 属性（DashCooldownDuration 等，符文可改速）
	 */
	void RegisterSkill(FGameplayTag SkillTag, FGameplayAttribute MaxChargeAttr, FGameplayAttribute CDDurationAttr);

	/** GA CanActivate 里调用 */
	UFUNCTION(BlueprintCallable, Category = "SkillCharge")
	bool HasCharge(FGameplayTag SkillTag) const;

	/**
	 * GA Activate 里调用：消耗 1 格充能，启动用后触发回复
	 * @return false 表示当前无充能（调用方应已通过 HasCharge 检查）
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillCharge")
	bool ConsumeCharge(FGameplayTag SkillTag);

	/** 当前可用充能格数（UI 用） */
	UFUNCTION(BlueprintPure, Category = "SkillCharge")
	int32 GetCurrentCharge(FGameplayTag SkillTag) const;

	/** 当前最大充能格数（UI 用） */
	UFUNCTION(BlueprintPure, Category = "SkillCharge")
	int32 GetMaxCharge(FGameplayTag SkillTag) const;

	/** 充能变化时广播（供 UI 刷新） */
	UPROPERTY(BlueprintAssignable, Category = "SkillCharge")
	FOnChargeChanged OnChargeChanged;

private:
	TMap<FGameplayTag, FSkillChargeState> ChargeStates;
	TWeakObjectPtr<UAbilitySystemComponent> ASC;

	int32 GetMaxChargeValue(const FSkillChargeState& State) const;
	float GetCDDurationValue(const FSkillChargeState& State) const;

	/** 队列式回复 Tick：回复 1 格，若还有待回复格则继续启动 Timer */
	void RecoveryTick(FGameplayTag SkillTag);
};