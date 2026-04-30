#include "Data/MontageAttackDataAsset.h"

UMontageAttackDataAsset::UMontageAttackDataAsset()
{
	EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack"));
}

FActionData UMontageAttackDataAsset::BuildActionData() const
{
	FActionData Out;
	Out.ActDamage = ActDamage;
	Out.ActRange = ActRange;
	Out.ActResilience = ActResilience;
	Out.ActDmgReduce = ActDmgReduce;
	Out.hitboxTypes = HitboxTypes;
	return Out;
}
