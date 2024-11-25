#include "DamageExecution.h"
#include "../AbilitySystem/Attribute/YogCombatSet.h"
#include "GameplayEffectExecutionCalculation.h"

struct FYogDamageStatics
{
	//DECLARE_ATTRIBUTE_CAPTUREDEF(baseDMG);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(WeaponDMG);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(BuffATKAmplify);

	//DECLARE_ATTRIBUTE_CAPTUREDEF(BuffATK);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(DMGCorrect);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(DMGAbsorb);

	FGameplayEffectAttributeCaptureDefinition baseDMG;
	FGameplayEffectAttributeCaptureDefinition WeaponDMG;
	FGameplayEffectAttributeCaptureDefinition BuffATKAmplify;
	FGameplayEffectAttributeCaptureDefinition BuffATK;
	FGameplayEffectAttributeCaptureDefinition DMGCorrect;
	FGameplayEffectAttributeCaptureDefinition DMGAbsorb;


	FYogDamageStatics()
	{
		baseDMG = FGameplayEffectAttributeCaptureDefinition(UYogCombatSet::GetbaseDMGAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		WeaponDMG = FGameplayEffectAttributeCaptureDefinition(UYogCombatSet::GetWeaponDMGAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		BuffATKAmplify = FGameplayEffectAttributeCaptureDefinition(UYogCombatSet::GetBuffATKAmplifyAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		BuffATK = FGameplayEffectAttributeCaptureDefinition(UYogCombatSet::GetBuffATKAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		DMGCorrect = FGameplayEffectAttributeCaptureDefinition(UYogCombatSet::GetDMGCorrectAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		DMGAbsorb = FGameplayEffectAttributeCaptureDefinition(UYogCombatSet::GetDMGAbsorbAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		//// Capture the Target's DefensePower attribute. Do not snapshot it, because we want to use the health value at the moment we apply the execution.
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, DefensePower, Target, false);

		//// Capture the Source's AttackPower. We do want to snapshot this at the moment we create the GameplayEffectSpec that will execute the damage.
		//// (imagine we fire a projectile: we create the GE Spec when the projectile is fired. When it hits the target, we want to use the AttackPower at the moment
		//// the projectile was launched, not when it hits).
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, AttackPower, Source, true);

		//// Also capture the source's raw Damage, which is normally passed in directly via the execution
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, Damage, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, baseDMG, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, WeaponDMG, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, BuffATKAmplify, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, BuffATK, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, DMGCorrect, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UYogCombatSet, DMGAbsorb, Source, true);

	}

};


static FYogDamageStatics& DamageStatics()
{
	static FYogDamageStatics DmgStatics;
	return DmgStatics;
}

UDamageExecution::UDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().baseDMG);
	RelevantAttributesToCapture.Add(DamageStatics().WeaponDMG);
	RelevantAttributesToCapture.Add(DamageStatics().BuffATKAmplify);
	RelevantAttributesToCapture.Add(DamageStatics().BuffATK);
	RelevantAttributesToCapture.Add(DamageStatics().DMGCorrect);
	RelevantAttributesToCapture.Add(DamageStatics().DMGAbsorb);
}


void UDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const 
{
	// --------------------------------------
	//	Damage Done =  ((baseDMG + WeaponDMG) * BuffAmplify + BuffingATK) * DMGCorrect * DMGAbsorb
	// --------------------------------------

	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float baseDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().baseDMG, EvaluationParameters, baseDMG);

	float WeaponDMG = 0.f;
	float BuffAmplify = 0.f;
	float BuffingATK = 0.f;
	float DMGCorrect = 0.f;
	float DMGAbsorb = 0.f;

	float DamgeDone = ((baseDMG + WeaponDMG) * BuffAmplify + BuffingATK) * DMGCorrect * DMGAbsorb;
}
