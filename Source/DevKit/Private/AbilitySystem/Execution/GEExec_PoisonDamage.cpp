#include "AbilitySystem/Execution/GEExec_PoisonDamage.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "GameplayTagContainer.h"

namespace
{
	struct FPoisonDamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
		DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorHP);

		FPoisonDamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, MaxHealth, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, ArmorHP, Target, false);
		}
	};

	const FPoisonDamageStatics& PoisonStatics()
	{
		static FPoisonDamageStatics Statics;
		return Statics;
	}

	float GetSetByCallerOrDefault(const FGameplayEffectSpec& Spec, const TCHAR* TagName, const float DefaultValue)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (!Tag.IsValid())
		{
			return DefaultValue;
		}

		const float Value = Spec.GetSetByCallerMagnitude(Tag, false, DefaultValue);
		return Value > 0.f ? Value : DefaultValue;
	}
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

	const int32 StackCount = FMath::Max(1, Spec.GetStackCount());
	const float PercentPerStack = GetSetByCallerOrDefault(Spec, TEXT("Data.Poison.PercentPerStack"), 0.02f);
	const float ArmorPercentPerStack = GetSetByCallerOrDefault(Spec, TEXT("Data.Poison.ArmorPercentPerStack"), 0.08f);

	const FGameplayTag FlatDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
	const float FlatDamage = FlatDamageTag.IsValid()
		? FMath::Max(0.f, Spec.GetSetByCallerMagnitude(FlatDamageTag, false, 0.f))
		: 0.f;

	const float RawHealthDamage = (MaxHealth * PercentPerStack * StackCount) + FlatDamage;
	const float HealthDamage = FMath::Min(RawHealthDamage, FMath::Max(0.f, CurrentHealth - 1.f));
	if (HealthDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UDamageAttributeSet::GetDamageBuffAttribute(),
				EGameplayModOp::Additive,
				HealthDamage));
	}

	static const FGameplayTag ArmoredTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Armored"), false);
	const bool bTargetIsArmored = ArmoredTag.IsValid()
		&& EvalParams.TargetTags
		&& EvalParams.TargetTags->HasTag(ArmoredTag)
		&& ArmorHP > 0.f;
	if (bTargetIsArmored)
	{
		const float RawArmorDamage = MaxHealth * ArmorPercentPerStack * StackCount;
		const float ArmorDamage = FMath::Min(RawArmorDamage, ArmorHP);
		if (ArmorDamage > 0.f)
		{
			OutExecutionOutput.AddOutputModifier(
				FGameplayModifierEvaluatedData(
					UBaseAttributeSet::GetArmorHPAttribute(),
					EGameplayModOp::Additive,
					-ArmorDamage));
		}
	}
}
