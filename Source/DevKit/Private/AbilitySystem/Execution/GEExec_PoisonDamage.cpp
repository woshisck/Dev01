#include "AbilitySystem/Execution/GEExec_PoisonDamage.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "GameplayTagContainer.h"

struct FPoisonDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorHP);

	FPoisonDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health,    Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, ArmorHP,   Target, false);
	}
};

static const FPoisonDamageStatics& PoisonStatics()
{
	static FPoisonDamageStatics S;
	return S;
}

UGEExec_PoisonDamage::UGEExec_PoisonDamage()
{
	RelevantAttributesToCapture.Add(PoisonStatics().MaxHealthDef);
	RelevantAttributesToCapture.Add(PoisonStatics().HealthDef);
	RelevantAttributesToCapture.Add(PoisonStatics().ArmorHPDef);
}

void UGEExec_PoisonDamage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxHealth = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(PoisonStatics().MaxHealthDef, EvalParams, MaxHealth);
	MaxHealth = FMath::Max(MaxHealth, 0.f);

	float CurrentHealth = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(PoisonStatics().HealthDef, EvalParams, CurrentHealth);
	CurrentHealth = FMath::Max(CurrentHealth, 0.f);

	float ArmorHP = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(PoisonStatics().ArmorHPDef, EvalParams, ArmorHP);
	ArmorHP = FMath::Max(ArmorHP, 0.f);

	// 主伤害：7% MaxHealth，不至死（最低保留 1HP）
	const float RawDamage = MaxHealth * 0.07f;
	const float HealthDamage = FMath::Min(RawDamage, FMath::Max(0.f, CurrentHealth - 1.f));

	if (HealthDamage > 0.f)
	{
		// DamageBuff 绕过护甲吸收，DamageAttributeSet 以 max(Health-damage, 1) 处理
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UDamageAttributeSet::GetDamageBuffAttribute(),
				EGameplayModOp::Additive,
				HealthDamage));
	}

	// 护甲额外伤害：25% MaxHealth，仅扣护甲
	static const FGameplayTag ArmoredTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Armored"), false);
	if (ArmoredTag.IsValid() && EvalParams.TargetTags && EvalParams.TargetTags->HasTag(ArmoredTag) && ArmorHP > 0.f)
	{
		const float ArmorDamage = FMath::Min(MaxHealth * 0.25f, ArmorHP);
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UBaseAttributeSet::GetArmorHPAttribute(),
				EGameplayModOp::Additive,
				-ArmorDamage));
	}
}
