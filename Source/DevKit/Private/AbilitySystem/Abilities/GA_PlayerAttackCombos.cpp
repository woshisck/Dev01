#include "AbilitySystem/Abilities/GA_PlayerAttackCombos.h"

UGA_PlayerAttack_Combo1::UGA_PlayerAttack_Combo1()
{
	const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo1"));
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_PlayerAttack_Combo2::UGA_PlayerAttack_Combo2()
{
	const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo2"));
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_PlayerAttack_Combo3::UGA_PlayerAttack_Combo3()
{
	const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo3"));
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_PlayerAttack_Combo4::UGA_PlayerAttack_Combo4()
{
	const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo4"));
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(ComboTag);
}
