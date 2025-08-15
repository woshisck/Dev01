#include "DamageExecution.h"

#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "GameplayEffectExecutionCalculation.h"

struct FYogDamageStatics
{

	//DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);

	//DECLARE_ATTRIBUTE_CAPTUREDEF(BaseDMG);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(WeaponDMG);

	//DECLARE_ATTRIBUTE_CAPTUREDEF(BuffAmplify);


	//DECLARE_ATTRIBUTE_CAPTUREDEF(BuffingATK);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(OwnerSpeed);

	//DECLARE_ATTRIBUTE_CAPTUREDEF(DMGCorrect);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(DMGAbsorb);
	//
	//DECLARE_ATTRIBUTE_CAPTUREDEF(HitRate);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(Evade);

	DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);

	FYogDamageStatics()
	{
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Source, true);

		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, BaseDMG, Source, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, WeaponDMG, Source, false);


		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, BuffAmplify, Source, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Damage, Source, true);

		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, BuffingATK, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DMGCorrect, Source, true);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DMGAbsorb, Source, true);
		

	}

};


static const FYogDamageStatics& DamageStatics()
{
	static FYogDamageStatics DmgStatics;
	return DmgStatics;
}

UDamageExecution::UDamageExecution()
{
	//RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
	//RelevantAttributesToCapture.Add(DamageStatics().BaseDMGDef);
	//RelevantAttributesToCapture.Add(DamageStatics().WeaponDMGDef);
	//RelevantAttributesToCapture.Add(DamageStatics().BuffAmplifyDef);

}


void UDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const 
{

	//UE_LOG(LogTemp, Warning, TEXT("Start Damage execution "));


	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();



	////Get Avatar Actor
	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	////Get GameplayEffect Instance
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();



	//const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	//const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	//FAggregatorEvaluateParameters EvaluationParameters;
	//EvaluationParameters.SourceTags = SourceTags;
	//EvaluationParameters.TargetTags = TargetTags;


	//float BaseDMG = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDMGDef, EvaluationParameters, BaseDMG);



	//float WeaponDMG = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().WeaponDMGDef, EvaluationParameters, WeaponDMG);

	////float WeaponDMG = FMath::Max<float>(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), false, -1.0f), 0.0f);


	//float BuffATKAmplify = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BuffAmplifyDef, EvaluationParameters, BuffATKAmplify);

	//float Damage = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageDef, EvaluationParameters, Damage);

	//// --------------------------------------
	////	Damage Done =  ((BaseDMG + WeaponDMG) * BuffAmplify + BuffingATK) * DMGCorrect
	//// BuffingATK = Self DMG + Weapon DMG 
	//// 
	//// --------------------------------------

	////TODO:need to change in future
	//float DMGDone = (BaseDMG + WeaponDMG) * BuffATKAmplify;
	////UE_LOG(LogTemp, Warning, TEXT("DMGDone is: %f"), DMGDone);


	//OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamageProperty, EGameplayModOp::Additive, DMGDone));
	//
	////Broadcast damages to Target ASC
	//UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
	//if (TargetASC)
	//{	
	//	UE_LOG(LogTemp, Warning, TEXT("Damage deal total: %f"), DMGDone);

	//	UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceAbilitySystemComponent);
	//	TargetASC->ReceiveDamage(SourceASC, DMGDone);
	//}
}
