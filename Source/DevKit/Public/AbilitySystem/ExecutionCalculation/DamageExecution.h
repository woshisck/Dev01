#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/AbilityData.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "DamageExecution.generated.h"

class UObject;


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
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);

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
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, MaxHealth, Target, false);
		//Current source damage
		DEFINE_ATTRIBUTE_CAPTUREDEF(UDamageAttributeSet, DamagePhysical, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UDamageAttributeSet, DamageMagic, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UDamageAttributeSet, DamagePure, Source, false);

	}

};


UCLASS()
class DEVKIT_API UDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()


public:
	UDamageExecution();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

};
