#include "AbilitySystem/ExecutionCalculation/HealthPercentExecution.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"


#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Data/AbilityData.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/ExecutionCalculation/DamageExecution.h"

struct FYogHealthStatics
{

	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);

	FYogHealthStatics()
	{
		//Current target health 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, MaxHealth, Target, false);

	}
};



static const FYogDamageStatics& MaxHealthStatics()
{
	static FYogDamageStatics HealthStatics;
	return HealthStatics;
}



UHealthPercentExecution::UHealthPercentExecution()
{
	


	RelevantAttributesToCapture.Add(MaxHealthStatics().HealthDef);
	RelevantAttributesToCapture.Add(MaxHealthStatics().MaxHealthDef);
	
	//RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	//RelevantAttributesToCapture.Add(DamageStatics().SanityDef);
	//RelevantAttributesToCapture.Add(DamageStatics().ResilienceDef);
	//RelevantAttributesToCapture.Add(DamageStatics().ResistDef);
	//RelevantAttributesToCapture.Add(DamageStatics().ShieldDef);
	//RelevantAttributesToCapture.Add(DamageStatics().DodgeDef);
	//RelevantAttributesToCapture.Add(DamageStatics().DmgTakenDef);
	//RelevantAttributesToCapture.Add(DamageStatics().Crit_RateDef);
	//RelevantAttributesToCapture.Add(DamageStatics().Crit_DamageDef);
	//RelevantAttributesToCapture.Add(DamageStatics().HealthDef);

	RelevantAttributesToCapture.Add(MaxHealthStatics().DamagePhysicalDef);
	RelevantAttributesToCapture.Add(MaxHealthStatics().DamageMagicDef);
	RelevantAttributesToCapture.Add(MaxHealthStatics().DamagePureDef);

}

void UHealthPercentExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	SCOPED_NAMED_EVENT(UDamageExecution__Execute_Implementation, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(UDamageExecution_Execute_Implementation);


	FGameplayEffectSpec* spec = ExecutionParams.GetOwningSpecForPreExecuteMod();


	UYogAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UYogAbilitySystemComponent* SourceAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	
	//UYogGameplayAbility* target_CurrentAbilityClass = TargetAbilitySystemComponent->GetCurrentAbilityClass();
	//UYogGameplayAbility* source_CurrentAbilityClass = SourceAbilitySystemComponent->GetCurrentAbilityClass();


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
	

	float MaxHealth = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MaxHealthStatics().MaxHealthDef, EvaluationParameters, MaxHealth);


	float DamageDone = 0.f;
	DamageDone = MaxHealth * MaxHealthPercent;



	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(MaxHealthStatics().DamagePureProperty, EGameplayModOp::Additive, DamageDone));
	//Broadcast damages to Target ASC
	UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
	if (TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Health percent DamagePure deal total: %f"), DamageDone);
		UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceAbilitySystemComponent);
		TargetASC->ReceiveDamage(SourceASC, DamageDone);
	}


	
	//OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamagePhysicalProperty, EGameplayModOp::Additive, DamageDone));
	////Broadcast damages to Target ASC
	//UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
	//if (TargetASC)
	//{	
	//	UE_LOG(LogTemp, Warning, TEXT("Damage deal total: %f"), DamageDone);
	//	UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceAbilitySystemComponent);
	//	TargetASC->ReceiveDamage(SourceASC, DamageDone);
	//}
}

