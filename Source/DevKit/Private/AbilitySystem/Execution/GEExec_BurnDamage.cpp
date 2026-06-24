#include "AbilitySystem/Execution/GEExec_BurnDamage.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "GameplayTagContainer.h"

UGEExec_BurnDamage::UGEExec_BurnDamage()
{
	// 不需要捕获属性，直接SetByCaller 读取 BaseDamage，从 TargetTags 读取状
}

void UGEExec_BurnDamage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 基础伤害GE SetByCaller 读取（GE_Burn 在此槽填写每秒基础燃烧伤害
	static const FGameplayTag BurnDamageTag =
		FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Burn"), false);
	float BaseDamage = 0.f;
	if (BurnDamageTag.IsValid())
	{
		BaseDamage = Spec.GetSetByCallerMagnitude(BurnDamageTag, false, 0.f);
	}

	if (BaseDamage <= 0.f) return;

	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// 无护+15%
	static const FGameplayTag ArmoredTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Armored"), false);
	const bool bHasArmor = ArmoredTag.IsValid() && TargetTags && TargetTags->HasTag(ArmoredTag);
	if (!bHasArmor)
	{
		BaseDamage *= 1.15f;
	}

	// 有流血 +15%
	static const FGameplayTag BleedingTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Bleed"), false);
	const bool bBleeding = BleedingTag.IsValid() && TargetTags && TargetTags->HasTag(BleedingTag);
	if (bBleeding)
	{
		BaseDamage *= 1.15f;
	}

	// 输出DamagePure 经护甲拦扣血
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UDamageAttributeSet::GetDamagePureAttribute(),
			EGameplayModOp::Additive,
			BaseDamage));
}
