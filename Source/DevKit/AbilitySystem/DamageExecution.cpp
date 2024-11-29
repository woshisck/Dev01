#include "DamageExecution.h"

#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>

#include "GameplayEffectExecutionCalculation.h"

struct FYogDamageStatics
{

	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);

	DECLARE_ATTRIBUTE_CAPTUREDEF(BaseDMG);
	DECLARE_ATTRIBUTE_CAPTUREDEF(WeaponDMG);

	DECLARE_ATTRIBUTE_CAPTUREDEF(BuffAmplify);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(BuffingATK);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(OwnerSpeed);

	//DECLARE_ATTRIBUTE_CAPTUREDEF(DMGCorrect);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(DMGAbsorb);
	//
	//DECLARE_ATTRIBUTE_CAPTUREDEF(HitRate);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(Evade);

	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageResult);

	FYogDamageStatics()
	{


		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, BaseDMG, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, WeaponDMG, Source, true);


		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, BuffAmplify, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DamageResult, Source, true);

		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, BuffingATK, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DMGCorrect, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DMGAbsorb, Source, true);
		

	}

};


static FYogDamageStatics& DamageStatics()
{
	static FYogDamageStatics DmgStatics;
	return DmgStatics;
}

UDamageExecution::UDamageExecution()
{

	RelevantAttributesToCapture.Add(DamageStatics().BaseDMGDef);
	RelevantAttributesToCapture.Add(DamageStatics().WeaponDMGDef);
	RelevantAttributesToCapture.Add(DamageStatics().BuffAmplifyDef);

//	RelevantAttributesToCapture.Add(DamageStatics().BuffingATKDef);
//	RelevantAttributesToCapture.Add(DamageStatics().DMGCorrectDef);
//	RelevantAttributesToCapture.Add(DamageStatics().DMGAbsorbDef);
}


void UDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const 
{
	// --------------------------------------
	//	Damage Done =  ((BaseDMG + WeaponDMG) * BuffAmplify + BuffingATK) * DMGCorrect * DMGAbsorb
	// --------------------------------------

	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();



	////Get Avatar Actor
	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	////Get GameplayEffect Instance
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();



	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	//float WeaponDamage = FMath::Max<float>(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damge")), false, -1.0f), 0.0f);

	float BaseDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDMGDef, EvaluationParameters, BaseDMG);

	float WeaponDMG = 50.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().WeaponDMGDef, EvaluationParameters, WeaponDMG);
	//WeaponDMG = FMath::Max(0.f, WeaponDMG);

	float BuffATKAmplify = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BuffAmplifyDef, EvaluationParameters, BuffATKAmplify);

	float DamageResult = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageResultDef, EvaluationParameters, DamageResult);


	float DMGDone = (BaseDMG + WeaponDMG) * BuffATKAmplify;
	//UE_LOG(LogTemp, Log, TEXT("stat print: %f, %f, %f, %f, %f, %f, %f"), BaseDMG,WeaponDMG,BuffATKAmplify,BuffATK,DMGCorrect,DMGAbsorb, DMGDone);
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamageResultProperty, EGameplayModOp::Additive, DMGDone));
}
