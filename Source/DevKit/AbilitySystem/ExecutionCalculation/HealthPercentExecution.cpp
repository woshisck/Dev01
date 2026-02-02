#include "HealthPercentExecution.h"

#include "DevKit/AbilitySystem/Attribute/BaseAttributeSet.h"


#include "DevKit/AbilitySystem/Attribute/DamageAttributeSet.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "GameplayEffectExecutionCalculation.h"
#include <Data/AbilityData.h>
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "DevKit/AbilitySystem/ExecutionCalculation/DamageExecution.h"

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

static const FYogHealthStatics& HealthStatics()
{
	static FYogHealthStatics HealthStatics;
	return HealthStatics;
}



UHealthPercentExecution::UHealthPercentExecution()
{
	
	RelevantAttributesToCapture.Add(HealthStatics().HealthDef);
	RelevantAttributesToCapture.Add(HealthStatics().MaxHealthDef);
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
	//RelevantAttributesToCapture.Add(DamageStatics().DamagePhysicalDef);
	//RelevantAttributesToCapture.Add(DamageStatics().DamageMagicDef);
	//RelevantAttributesToCapture.Add(DamageStatics().DamagePureDef);

}

void UHealthPercentExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
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

