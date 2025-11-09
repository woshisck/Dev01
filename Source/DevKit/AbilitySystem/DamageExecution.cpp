#include "DamageExecution.h"

#include "DevKit/AbilitySystem/Attribute/BaseAttributeSet.h"


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

		//Target DamageTaken
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, DmgTaken, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Crit_Rate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Crit_Damage, Source, false);





		//Current target health 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Target, false);

		//Current source damage
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
	RelevantAttributesToCapture.Add(DamageStatics().AttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().SanityDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResilienceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResistDef);
	RelevantAttributesToCapture.Add(DamageStatics().ShieldDef);
	RelevantAttributesToCapture.Add(DamageStatics().DodgeDef);
	RelevantAttributesToCapture.Add(DamageStatics().DmgTakenDef);
	RelevantAttributesToCapture.Add(DamageStatics().Crit_RateDef);
	RelevantAttributesToCapture.Add(DamageStatics().Crit_DamageDef);

	RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamagePhysicalDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageMagicDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamagePureDef);


}


void UDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const 
{
	SCOPED_NAMED_EVENT(UDamageExecution__Execute_Implementation, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(UDamageExecution_Execute_Implementation);


	FGameplayEffectSpec* spec = ExecutionParams.GetOwningSpecForPreExecuteMod();


	UYogAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UYogAbilitySystemComponent* SourceAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	
	UYogGameplayAbility* target_CurrentAbilityClass = TargetAbilitySystemComponent->GetCurrentAbilityClass();
	UYogGameplayAbility* source_CurrentAbilityClass = SourceAbilitySystemComponent->GetCurrentAbilityClass();


	//FActionData target_ActionData = target_CurrentAbilityClass->GetRowData(target_CurrentAbilityClass->ActionRow);
	//FActionData source_ActionData = source_CurrentAbilityClass->GetRowData(source_CurrentAbilityClass->ActionRow);


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
	

//Crit_Rate +  = Final_Crit_rate
//Crit_Damage +  = Crit_Damage
//(Attack + ) * (AttackPower + ) * (DmgTaken + Add_DmgTaken)

	////////////////////////////////////////////////// Source //////////////////////////////////////////////////

	float Attack = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef, EvaluationParameters, Attack);

	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);

	float Crit_Rate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_RateDef, EvaluationParameters, Crit_Rate);

	float Crit_Damage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_DamageDef, EvaluationParameters, Crit_Damage);




	////////////////////////////////////////////////// Target //////////////////////////////////////////////////
	float DmgTaken = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DmgTakenDef, EvaluationParameters, DmgTaken);


	float Dodge = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DodgeDef, EvaluationParameters, Dodge);

	float Shield = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ShieldDef, EvaluationParameters, Shield);



	float DamageDone = 0.f;
	float RandomFloatValue = FMath::FRand();

	if (Shield < 0.f)
	{	//Crit_Value <= Crit_Rate IS Critical Hit
		if (RandomFloatValue <= Crit_Rate)
		{
			DamageDone = (AttackPower ) * (Attack ) * (Crit_Damage ) * (DmgTaken );
			
		}
		else
		{
			DamageDone = (AttackPower ) * (Attack ) * (DmgTaken );
			
		}
	}
	else
	{
		if (RandomFloatValue <= Crit_Rate)
		{
			DamageDone = (AttackPower ) * (Attack ) * (Crit_Damage ) * (DmgTaken );
			
		}
		else
		{
			DamageDone = (AttackPower ) * (Attack ) * (DmgTaken );
			
		}
	}
	
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamagePhysicalProperty, EGameplayModOp::Additive, DamageDone));
	//Broadcast damages to Target ASC
	UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
	if (TargetASC)
	{	
		UE_LOG(LogTemp, Warning, TEXT("Damage deal total: %f"), DamageDone);
		UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceAbilitySystemComponent);
		TargetASC->ReceiveDamage(SourceASC, DamageDone);
	}
}

