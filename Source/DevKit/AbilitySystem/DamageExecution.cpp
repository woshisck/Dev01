#include "DamageExecution.h"

#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "GameplayEffectExecutionCalculation.h"

struct FYogDamageStatics
{

	//DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(Attack);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(WeaponAtk);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(ActDamage);


	//DECLARE_ATTRIBUTE_CAPTUREDEF(ActDmgReduce);

	//DECLARE_ATTRIBUTE_CAPTUREDEF(Shield);


	//DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalRate);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);


	//DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);
	//DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FYogDamageStatics()
	{

		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Attack, Source, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, WeaponAtk, Source, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, ActDamage, Source, false);

		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, ActDmgReduce, Target, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Shield, Target, false);

		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CriticalRate, Source, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CriticalDamage, Source, false);


		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Target, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Damage, Source, false);

	}

};


static const FYogDamageStatics& DamageStatics()
{
	static FYogDamageStatics DmgStatics;
	return DmgStatics;
}

UDamageExecution::UDamageExecution()
{
	//RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	//RelevantAttributesToCapture.Add(DamageStatics().AttackDef);
	//RelevantAttributesToCapture.Add(DamageStatics().WeaponAtkDef);
	//RelevantAttributesToCapture.Add(DamageStatics().ActDamageDef);

	//RelevantAttributesToCapture.Add(DamageStatics().ActDmgReduceDef);
	//RelevantAttributesToCapture.Add(DamageStatics().ShieldDef);

	//RelevantAttributesToCapture.Add(DamageStatics().CriticalRateDef);
	//RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);


	//RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
	//RelevantAttributesToCapture.Add(DamageStatics().DamageDef);
}


void UDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const 
{

	//UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	//UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();



	//////Get Avatar Actor
	//AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
	//AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	//////Get GameplayEffect Instance
	//const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();


	////get both tags container
	//const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	//const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	//FAggregatorEvaluateParameters EvaluationParameters;
	//EvaluationParameters.SourceTags = SourceTags;
	//EvaluationParameters.TargetTags = TargetTags;
	

	//////////////////////////////////////////////////// Source Attack //////////////////////////////////////////////////
	//float AttackPower = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);
	//float Attack = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef, EvaluationParameters, Attack);
	//float WeaponAtk = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().WeaponAtkDef, EvaluationParameters, WeaponAtk);
	//float ActDamage = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ActDamageDef, EvaluationParameters, ActDamage);
	//////////////////////////////////////////////////// Target Reduce //////////////////////////////////////////////////


	//float ActDmgReduce = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ActDmgReduceDef, EvaluationParameters, ActDmgReduce);
	//float Shield = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ShieldDef, EvaluationParameters, Shield);


	//////////////////////////////////////////////////// Critical //////////////////////////////////////////////////
	//float CriticalRate = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalRateDef, EvaluationParameters, CriticalRate);
	//float CriticalDamage = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, CriticalDamage);
	//float DamageDone = 0.f;
	//float Crit_Value = FMath::FRand();
	//if (Shield < 0.f)
	//{	//Crit_Value <= CriticalRate IS Critical Hit
	//	if (Crit_Value <= CriticalRate)
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * CriticalDamage;
	//	}
	//	else
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * 1;
	//	}
	//}
	//else
	//{
	//	if (Crit_Value <= CriticalRate)
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * CriticalDamage;
	//	}
	//	else
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * 1;
	//	}
	//}
	////DamageDone = -1 * DamageDone;
	//
	//OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamageProperty, EGameplayModOp::Additive, DamageDone));
	////Broadcast damages to Target ASC
	//UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
	//if (TargetASC)
	//{	
	//	UE_LOG(LogTemp, Warning, TEXT("Damage deal total: %f"), DamageDone);
	//	UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceAbilitySystemComponent);
	//	TargetASC->ReceiveDamage(SourceASC, DamageDone);
	//}
}





////////////////////////////////////////////////// Damage Persudo Code //////////////////////////////////////////////////
//AttackPower* (Attack + WeaponAtk + ActDamage)* ActDmgReduce* (if Crit CritcalDamage)
//
//if (shield > 0)

//{
//	if (crit)
//	{
//		damage = AttackPower * (Attack + WeaponAtk + ActDamage) * CritcalDamage
//	}
//	else
//	{
//		damage = (AttackPower * (Attack + WeaponAtk + ActDamage) * 1)
//	}
//}
//else
//{
//	if (crit)
//	{
//		damage = AttackPower * (Attack + WeaponAtk + ActDamage) * CritcalDamage
//	}
//	else
//	{
//		damage = (AttackPower * (Attack + WeaponAtk + ActDamage) * 1 + Flat_DAMAGE) * dmg_reduce - afterDmg_reduce
//
//
//			damage = ((AttackPower * (Attack + WeaponAtk + ActDamage) * 1) * dmg_reduce) +
//	}
//}
//
//
//if (!immuteResilience_value)
//{
//	if (self.act_resilience < enemy.resilience)
//	{
//		cause damage_montage
//	}
//}
////////////////////////////////////////////////// Damage Persudo Code //////////////////////////////////////////////////