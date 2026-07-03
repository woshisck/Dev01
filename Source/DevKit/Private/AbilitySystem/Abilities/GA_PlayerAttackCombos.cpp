#include "AbilitySystem/Abilities/GA_PlayerAttackCombos.h"

UGA_PlayerAttack_Combo1::UGA_PlayerAttack_Combo1()
{
	const FGameplayTag MeleeTag          = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"));
	const FGameplayTag ComboTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo1"));
	const FGameplayTag CharacterComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo1"));
	ActivationRequiredTags.AddTag(MeleeTag);
	AbilityTags.AddTag(CharacterComboTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(CharacterComboTag);
}

UGA_PlayerAttack_Combo2::UGA_PlayerAttack_Combo2()
{
	const FGameplayTag MeleeTag          = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"));
	const FGameplayTag BroadTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag ComboTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo2"));
	const FGameplayTag CharacterBroadTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"));
	const FGameplayTag CharacterComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo2"));
	ActivationRequiredTags.AddTag(MeleeTag);
	AbilityTags.RemoveTag(BroadTag);
	AbilityTags.RemoveTag(CharacterBroadTag);
	ActivationOwnedTags.RemoveTag(BroadTag);
	ActivationOwnedTags.RemoveTag(CharacterBroadTag);
	AbilityTags.AddTag(CharacterComboTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(CharacterComboTag);
}

UGA_PlayerAttack_Combo3::UGA_PlayerAttack_Combo3()
{
	const FGameplayTag MeleeTag          = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"));
	const FGameplayTag BroadTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag ComboTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo3"));
	const FGameplayTag CharacterBroadTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"));
	const FGameplayTag CharacterComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo3"));
	ActivationRequiredTags.AddTag(MeleeTag);
	AbilityTags.RemoveTag(BroadTag);
	AbilityTags.RemoveTag(CharacterBroadTag);
	ActivationOwnedTags.RemoveTag(BroadTag);
	ActivationOwnedTags.RemoveTag(CharacterBroadTag);
	AbilityTags.AddTag(CharacterComboTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(CharacterComboTag);
}

UGA_PlayerAttack_Combo4::UGA_PlayerAttack_Combo4()
{
	const FGameplayTag MeleeTag          = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"));
	const FGameplayTag BroadTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag ComboTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo4"));
	const FGameplayTag CharacterBroadTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"));
	const FGameplayTag CharacterComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo4"));
	ActivationRequiredTags.AddTag(MeleeTag);
	AbilityTags.RemoveTag(BroadTag);
	AbilityTags.RemoveTag(CharacterBroadTag);
	ActivationOwnedTags.RemoveTag(BroadTag);
	ActivationOwnedTags.RemoveTag(CharacterBroadTag);
	AbilityTags.AddTag(CharacterComboTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(CharacterComboTag);
}
