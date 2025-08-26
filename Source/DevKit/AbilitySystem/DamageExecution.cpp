#include "DamageExecution.h"

#include "DevKit/AbilitySystem/Attribute/BaseAttributeSet.h"
#include "DevKit/AbilitySystem/Attribute/AdditionAttributeSet.h"

#include "DevKit/AbilitySystem/Attribute/DamageAttributeSet.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "GameplayEffectExecutionCalculation.h"
#include <Data/AbilityData.h>
#include "AbilitySystem/Abilities/YogGameplayAbility.h"

struct FYogDamageStatics
{

	
	DECLARE_ATTRIBUTE_CAPTUREDEF(Attack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Sanity);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Resilience);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Resist);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Shield);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Dodge);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DmgTaken);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Crit_Rate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Crit_Damage);

	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Attack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Sanity);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Resilience);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Resist);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Shield);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Dodge);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_DmgTaken);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Crit_Rate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Add_Crit_Damage);


	DECLARE_ATTRIBUTE_CAPTUREDEF(DamagePhysical);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageMagic);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamagePure);

	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FYogDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Attack, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Sanity, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Resilience, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Resist, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Shield, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Dodge, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DmgTaken, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Crit_Rate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Crit_Damage, Source, false);


		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Attack, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_AttackPower, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Sanity, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Resilience, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Resist, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Shield, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Dodge, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_DmgTaken, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Crit_Rate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAdditionAttributeSet, Add_Crit_Damage, Source, false);


		//Current health and damage
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Target, false);


		DEFINE_ATTRIBUTE_CAPTUREDEF(UDamageAttributeSet, DamagePhysical, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UDamageAttributeSet, DamageMagic, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UDamageAttributeSet, DamagePure, Source, false);

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


	UYogAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UYogAbilitySystemComponent* SourceAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	
	UYogGameplayAbility* target_CurrentAbilityClass = TargetAbilitySystemComponent->GetCurrentAbilityClass();
	UYogGameplayAbility* source_CurrentAbilityClass = SourceAbilitySystemComponent->GetCurrentAbilityClass();



	FActionData target_ActionData = target_CurrentAbilityClass->GetRowData();
	FActionData source_ActionData = source_CurrentAbilityClass->GetRowData();

	//Get Avatar Actor
	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	//Get GameplayEffect Instance
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();


	//get both tags container
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	

	////////////////////////////////////////////////// Source Attack //////////////////////////////////////////////////
	//float AttackPower = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);
	//float Attack = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef, EvaluationParameters, Attack);
	//float WeaponAtk = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().WeaponAtkDef, EvaluationParameters, WeaponAtk);
	//float ActDamage = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ActDamageDef, EvaluationParameters, ActDamage);
	////////////////////////////////////////////////// Target Reduce //////////////////////////////////////////////////


	//float ActDmgReduce = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ActDmgReduceDef, EvaluationParameters, ActDmgReduce);
	//float Shield = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ShieldDef, EvaluationParameters, Shield);


	//////////////////////////////////////////////////// Critical //////////////////////////////////////////////////
	//float Crit_Rate = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalRateDef, EvaluationParameters, Crit_Rate);
	//float Crit_Damage = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, Crit_Damage); * buff_crit_damage
	//float DamageDone = 0.f;
	//float Crit_Value = FMath::FRand();
	//if (Shield < 0.f)
	//{	//Crit_Value <= Crit_Rate IS Critical Hit
	//	if (Crit_Value <= Crit_Rate)
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * Crit_Damage;
	//	}
	//	else
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * 1;
	//	}
	//}
	//else
	//{
	//	if (Crit_Value <= Crit_Rate)
	//	{
	//		DamageDone = AttackPower * (Attack + WeaponAtk + ActDamage) * Crit_Damage;
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
//attack * attackPower + weaponAtk * weaponAtkPower + ActionAtk
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
//
//if (!immuteResilience_value)
//{
//	if (self.act_resilience < enemy.resilience)
//	{
//		cause damage_montage
//	}
//}
////////////////////////////////////////////////// Damage Persudo Code //////////////////////////////////////////////////