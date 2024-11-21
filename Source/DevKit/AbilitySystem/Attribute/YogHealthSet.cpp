#include "YogHealthSet.h"
#include "Net/UnrealNetwork.h"


UYogHealthSet::UYogHealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
{
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
}
void UYogHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogHealthSet, Health, OldValue);
}


void UYogHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogHealthSet, Health, OldValue);



}

bool UYogHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return false;
}

void UYogHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
}

void UYogHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
}

void UYogHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
}

void UYogHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
}

void UYogHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		// Do not allow health to go negative or above max health.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// Do not allow max health to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}


void UYogHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UYogHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

