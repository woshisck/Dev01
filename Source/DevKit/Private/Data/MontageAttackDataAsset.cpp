#include "Data/MontageAttackDataAsset.h"

FActionData FComboAttackConfig::BuildActionData() const
{
	FActionData Out;
	Out.ActDamage = ActDamage;
	Out.ActRange = ActRange;
	Out.ActResilience = ActResilience;
	Out.ActDmgReduce = ActDmgReduce;
	Out.hitboxTypes = HitboxTypes;
	return Out;
}

void FComboAttackConfig::CopyFromAttackData(const UMontageAttackDataAsset* AttackData)
{
	if (!AttackData)
	{
		return;
	}

	bEnabled = true;
	EventTag = AttackData->EventTag;
	ActDamage = AttackData->ActDamage;
	ActRange = AttackData->ActRange;
	ActResilience = AttackData->ActResilience;
	ActDmgReduce = AttackData->ActDmgReduce;
	HitboxTypes = AttackData->HitboxTypes;
	HitStopMode = AttackData->HitStopMode;
	HitStopFrozenDuration = AttackData->HitStopFrozenDuration;
	HitStopSlowDuration = AttackData->HitStopSlowDuration;
	HitStopSlowRate = AttackData->HitStopSlowRate;
	HitStopCatchUpRate = AttackData->HitStopCatchUpRate;
	OnHitEventTags = AttackData->OnHitEventTags;
	AdditionalRuneEffects = AttackData->AdditionalRuneEffects;
}

void FComboAttackConfig::CopyFromNotify(const UAN_MeleeDamage* DamageNotify)
{
	if (!DamageNotify)
	{
		return;
	}

	bEnabled = true;
	EventTag = DamageNotify->EventTag;
	ActDamage = DamageNotify->ActDamage;
	ActRange = DamageNotify->ActRange;
	ActResilience = DamageNotify->ActResilience;
	ActDmgReduce = DamageNotify->ActDmgReduce;
	HitboxTypes = DamageNotify->HitboxTypes;
	HitStopMode = DamageNotify->HitStopMode;
	HitStopFrozenDuration = DamageNotify->HitStopFrozenDuration;
	HitStopSlowDuration = DamageNotify->HitStopSlowDuration;
	HitStopSlowRate = DamageNotify->HitStopSlowRate;
	HitStopCatchUpRate = DamageNotify->HitStopCatchUpRate;
	// Note: UAN_MeleeDamage does not carry OnHitEventTags — that list lives on
	// UMontageAttackDataAsset. Use AttackDataOverride on the notify (or the
	// per-combo-node AttackData) if you need per-hit event tags.
	AdditionalRuneEffects = DamageNotify->AdditionalRuneEffects;
}

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
