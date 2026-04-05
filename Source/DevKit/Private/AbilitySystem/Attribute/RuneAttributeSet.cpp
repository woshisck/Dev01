#include "AbilitySystem/Attribute/RuneAttributeSet.h"
#include "Net/UnrealNetwork.h"

URuneAttributeSet::URuneAttributeSet()
{
}

void URuneAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(URuneAttributeSet, KnockbackForce, COND_None, REPNOTIFY_Always);
}

void URuneAttributeSet::OnRep_KnockbackForce(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(URuneAttributeSet, KnockbackForce, OldValue);
}
