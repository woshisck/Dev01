#include "AbilitySystem/GameplayEffect/GE_RuneBurn.h"

#include "AbilitySystem/Execution/GEExec_BurnDamage.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

UGE_RuneBurn::UGE_RuneBurn()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(4.f));
	Period = FScalableFloat(1.f);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UGEExec_BurnDamage::StaticClass();
	Executions.Add(ExecDef);

	if (const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Burning"), false);
		BurningTag.IsValid())
	{
		FInheritedTagContainer GrantedTagChanges;
		GrantedTagChanges.Added.AddTag(BurningTag);
		UTargetTagsGameplayEffectComponent* TargetTagsComponent =
			CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
		if (TargetTagsComponent)
		{
			GEComponents.Add(TargetTagsComponent);
			TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTagChanges);
		}
	}

	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
}
