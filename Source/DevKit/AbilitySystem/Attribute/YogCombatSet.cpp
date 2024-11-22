#include "YogCombatSet.h"
#include "BaseAttributeSet.h"

#include "Net/UnrealNetwork.h"


UYogCombatSet::UYogCombatSet()
	: BaseDamage(0.0f)
	, BaseHeal(0.0f)
{
}

void UYogCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, BaseDamage, OldValue);
}

void UYogCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, BaseHeal, OldValue);
}


void UYogCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}