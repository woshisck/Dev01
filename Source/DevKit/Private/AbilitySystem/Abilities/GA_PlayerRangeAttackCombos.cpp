#include "AbilitySystem/Abilities/GA_PlayerRangeAttackCombos.h"

UGA_PlayerRangeAttack_Combo1::UGA_PlayerRangeAttack_Combo1()
{
	static const FGameplayTag RangedTag    = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"),                    false);
	static const FGameplayTag ComboTag     = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo1"),   false);
	ActivationRequiredTags.AddTag(RangedTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_PlayerRangeAttack_Combo2::UGA_PlayerRangeAttack_Combo2()
{
	static const FGameplayTag RangedTag    = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"),                    false);
	static const FGameplayTag ComboTag     = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo2"),   false);
	ActivationRequiredTags.AddTag(RangedTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_PlayerRangeAttack_Combo3::UGA_PlayerRangeAttack_Combo3()
{
	static const FGameplayTag RangedTag    = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"),                    false);
	static const FGameplayTag ComboTag     = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo3"),   false);
	ActivationRequiredTags.AddTag(RangedTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_PlayerRangeAttack_Combo4::UGA_PlayerRangeAttack_Combo4()
{
	static const FGameplayTag RangedTag    = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"),                    false);
	static const FGameplayTag ComboTag     = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo4"),   false);
	ActivationRequiredTags.AddTag(RangedTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}
