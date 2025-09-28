#include "AttributeStatComponent.h"
#include <AbilitySystemGlobals.h>
#include "AbilitySystemComponent.h"
#include "DevKit/AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "DevKit/AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "DevKit/AbilitySystem/Attribute/BaseAttributeSet.h"


UAttributeStatComponent::UAttributeStatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UAttributeStatComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* abilitySystemComponent = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();

	abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute()).AddUObject(this, &UAttributeStatComponent::HandleHealthChange);

}

void UAttributeStatComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);
}


float UAttributeStatComponent::GetAttribute(FGameplayAttribute attribute) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());

	check(ASC);
	return ASC->GetNumericAttributeBase(attribute);
}

void UAttributeStatComponent::HandleHealthChange(const FOnAttributeChangeData& data)
{

	if (data.NewValue == data.OldValue) //this callback gets called when we set the value even if there was no change
	{
		return;
	}
	OnHealthChange.Broadcast(data.NewValue);
	UE_LOG(LogTemp, Warning, TEXT("UAttributeStatComponent::HandleHealthChange to : %f"), data.NewValue);
}

void UAttributeStatComponent::AddAttribute(FGameplayAttribute attribute, float value_add) const
{
	if (value_add != 0)
	{
		UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		check(abilitySystemComponent);

		abilitySystemComponent->SetNumericAttributeBase(attribute, abilitySystemComponent->GetNumericAttributeBase(attribute) + value_add);
	}
}

void UAttributeStatComponent::MultiplyAttribute(FGameplayAttribute attribute, float value_multiply) const
{
	if (value_multiply != 0)
	{
		UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		check(abilitySystemComponent);

		abilitySystemComponent->SetNumericAttributeBase(attribute, abilitySystemComponent->GetNumericAttributeBase(attribute) * value_multiply);
	}
}

void UAttributeStatComponent::DivideAttribute(FGameplayAttribute attribute, float value_divide) const
{
	if (value_divide != 0)
	{
		UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		check(abilitySystemComponent);

		abilitySystemComponent->SetNumericAttributeBase(attribute, abilitySystemComponent->GetNumericAttributeBase(attribute) / value_divide);
	}
}

float UAttributeStatComponent::GetStat_Attack() const
{
	return GetAttribute(UBaseAttributeSet::GetAttackAttribute());
}

float UAttributeStatComponent::GetStat_AttackPower() const
{
	return GetAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
}

float UAttributeStatComponent::GetStat_Health() const
{
	return GetAttribute(UBaseAttributeSet::GetHealthAttribute());
}

float UAttributeStatComponent::GetStat_MaxHealth() const
{
	return GetAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
}

float UAttributeStatComponent::GetStat_AttackSpeed() const
{
	return GetAttribute(UBaseAttributeSet::GetAttackSpeedAttribute());
}

float UAttributeStatComponent::GetStat_AttackRange() const
{
	return GetAttribute(UBaseAttributeSet::GetAttackRangeAttribute());
}

float UAttributeStatComponent::GetStat_Sanity() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_MoveSpeed() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_Dodge() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_Resilience() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_Resist() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_DmgTaken() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_Crit_Rate() const
{
	return 0.0f;
}

float UAttributeStatComponent::GetStat_Crit_Damage() const
{
	return 0.0f;
}
