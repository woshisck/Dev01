#include "YogCombatSet.h"


#include "Net/UnrealNetwork.h"


UYogCombatSet::UYogCombatSet()
	: baseDMG(0.0f)
	, BaseHeal(0.0f)
	, OwnerSpeed(600.0f)
	, DMGCorrect(1.0f)
	, DMGAbsorb(0.0f)
	, HitRate(1.0f)
	, Evade(0.0f)
{
}



void UYogCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);


	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, OwnerSpeed, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, DMGCorrect, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, DMGAbsorb, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, HitRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, Evade, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, BuffATKAmplify, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYogCombatSet, BuffATK, COND_OwnerOnly, REPNOTIFY_Always);


}

void UYogCombatSet::OnRep_OwnerSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, OwnerSpeed, OldValue);
}

void UYogCombatSet::OnRep_DMGCorrect(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, DMGCorrect, OldValue);
}

void UYogCombatSet::OnRep_DMGAbsorb(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, DMGAbsorb, OldValue);
}

void UYogCombatSet::OnRep_HitRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, HitRate, OldValue);
}

void UYogCombatSet::OnRep_Evade(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, Evade, OldValue);
}

void UYogCombatSet::OnRep_BuffATKAmplify(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, BuffATKAmplify, OldValue);
}

void UYogCombatSet::OnRep_BuffATK(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYogCombatSet, BuffATK, OldValue);
}
