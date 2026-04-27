#include "AbilitySystem/Abilities/GA_EnemyMeleeAttacks.h"

namespace
{
	FGameplayTag DeadStatusTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Status.Dead")));
	}
}

// ── Light Attacks ──────────────────────────────────────────────────────────

UGA_Enemy_LAtk1::UGA_Enemy_LAtk1()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.LAtk1")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

UGA_Enemy_LAtk2::UGA_Enemy_LAtk2()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.LAtk2")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

UGA_Enemy_LAtk3::UGA_Enemy_LAtk3()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.LAtk3")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

UGA_Enemy_LAtk4::UGA_Enemy_LAtk4()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.LAtk4")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

// ── Heavy Attacks ──────────────────────────────────────────────────────────

UGA_Enemy_HAtk1::UGA_Enemy_HAtk1()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.HAtk1")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

UGA_Enemy_HAtk2::UGA_Enemy_HAtk2()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.HAtk2")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

UGA_Enemy_HAtk3::UGA_Enemy_HAtk3()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.HAtk3")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}

UGA_Enemy_HAtk4::UGA_Enemy_HAtk4()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Melee.HAtk4")));
	ActivationBlockedTags.AddTag(DeadStatusTag());
}
