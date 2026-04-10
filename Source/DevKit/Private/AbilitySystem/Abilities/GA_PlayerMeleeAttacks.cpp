#include "AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h"
#include "UObject/ConstructorHelpers.h"

// ── 玩家近战公共基类 ──────────────────────────────────────────────────────

UGA_PlayerMeleeAttack::UGA_PlayerMeleeAttack()
{
	static ConstructorHelpers::FClassFinder<UGameplayEffect> GEBefore(
		TEXT("/Game/Code/GAS/Abilities/Shared/GE_StatBeforeATK"));
	if (GEBefore.Succeeded())
		StatBeforeATKEffect = GEBefore.Class;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> GEAfter(
		TEXT("/Game/Code/GAS/Abilities/Shared/GE_StatAfterATK"));
	if (GEAfter.Succeeded())
		StatAfterATKEffect = GEAfter.Class;
}

// ── 工具 ──────────────────────────────────────────────────────────────────

static FORCEINLINE FGameplayTag GT(const FName& Name)
{
	return FGameplayTag::RequestGameplayTag(Name);
}

// ── 常用 Tag ──────────────────────────────────────────────────────────────

static const FName TAG_CanCombo (TEXT("PlayerState.AbilityCast.CanCombo"));

// Light combo
static const FName TAG_LC1(TEXT("PlayerState.AbilityCast.LightAtk.Combo1"));
static const FName TAG_LC2(TEXT("PlayerState.AbilityCast.LightAtk.Combo2"));
static const FName TAG_LC3(TEXT("PlayerState.AbilityCast.LightAtk.Combo3"));
static const FName TAG_LC4(TEXT("PlayerState.AbilityCast.LightAtk.Combo4"));

// Heavy combo
static const FName TAG_HC1(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"));
static const FName TAG_HC2(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"));
static const FName TAG_HC3(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"));
static const FName TAG_HC4(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"));

// Dash attack
static const FName TAG_DA       (TEXT("PlayerState.AbilityCast.DashAtk"));
static const FName TAG_Dead     (TEXT("Buff.Status.Dead"));
static const FName TAG_HitReact (TEXT("Buff.Status.HitReact"));
static const FName TAG_Knockback(TEXT("Buff.Status.Knockback"));

// ── Light Attack Combo ────────────────────────────────────────────────────
//
// 规则（源自 Blueprint GA 截图）：
//   AbilityTags         = { 本步 ComboTag }
//   ActivationOwnedTags = { Combo1 … 本步 }（累积）
//   ActivationRequired  = { CanCombo, Combo1 … 前一步 }（Combo1 无 Required）
//   ActivationBlocked   = { 本步 ComboTag }（防止同步激活自身）
//   Dead / HitReact / Knockback 交给 StateConflict DA 处理

UGA_Player_LightAtk1::UGA_Player_LightAtk1()
{
	AbilityTags.AddTag(GT(TAG_LC1));

	ActivationOwnedTags.AddTag(GT(TAG_LC1));

	// 首段无 Required
	ActivationBlockedTags.AddTag(GT(TAG_LC1));
}

UGA_Player_LightAtk2::UGA_Player_LightAtk2()
{
	AbilityTags.AddTag(GT(TAG_LC2));

	ActivationOwnedTags.AddTag(GT(TAG_LC1));
	ActivationOwnedTags.AddTag(GT(TAG_LC2));

	ActivationRequiredTags.AddTag(GT(TAG_CanCombo));
	ActivationRequiredTags.AddTag(GT(TAG_LC1));

	ActivationBlockedTags.AddTag(GT(TAG_LC2));
}

UGA_Player_LightAtk3::UGA_Player_LightAtk3()
{
	AbilityTags.AddTag(GT(TAG_LC3));

	ActivationOwnedTags.AddTag(GT(TAG_LC1));
	ActivationOwnedTags.AddTag(GT(TAG_LC2));
	ActivationOwnedTags.AddTag(GT(TAG_LC3));

	ActivationRequiredTags.AddTag(GT(TAG_CanCombo));
	ActivationRequiredTags.AddTag(GT(TAG_LC1));
	ActivationRequiredTags.AddTag(GT(TAG_LC2));

	ActivationBlockedTags.AddTag(GT(TAG_LC3));
}

UGA_Player_LightAtk4::UGA_Player_LightAtk4()
{
	AbilityTags.AddTag(GT(TAG_LC4));

	ActivationOwnedTags.AddTag(GT(TAG_LC1));
	ActivationOwnedTags.AddTag(GT(TAG_LC2));
	ActivationOwnedTags.AddTag(GT(TAG_LC3));
	ActivationOwnedTags.AddTag(GT(TAG_LC4));

	ActivationRequiredTags.AddTag(GT(TAG_CanCombo));
	ActivationRequiredTags.AddTag(GT(TAG_LC1));
	ActivationRequiredTags.AddTag(GT(TAG_LC2));
	ActivationRequiredTags.AddTag(GT(TAG_LC3));

	ActivationBlockedTags.AddTag(GT(TAG_LC4));
}

// ── Heavy Attack Combo ────────────────────────────────────────────────────

UGA_Player_HeavyAtk1::UGA_Player_HeavyAtk1()
{
	AbilityTags.AddTag(GT(TAG_HC1));

	ActivationOwnedTags.AddTag(GT(TAG_HC1));

	ActivationBlockedTags.AddTag(GT(TAG_HC1));
}

UGA_Player_HeavyAtk2::UGA_Player_HeavyAtk2()
{
	AbilityTags.AddTag(GT(TAG_HC2));

	ActivationOwnedTags.AddTag(GT(TAG_HC1));
	ActivationOwnedTags.AddTag(GT(TAG_HC2));

	ActivationRequiredTags.AddTag(GT(TAG_CanCombo));
	ActivationRequiredTags.AddTag(GT(TAG_HC1));

	ActivationBlockedTags.AddTag(GT(TAG_HC2));
}

UGA_Player_HeavyAtk3::UGA_Player_HeavyAtk3()
{
	AbilityTags.AddTag(GT(TAG_HC3));

	ActivationOwnedTags.AddTag(GT(TAG_HC1));
	ActivationOwnedTags.AddTag(GT(TAG_HC2));
	ActivationOwnedTags.AddTag(GT(TAG_HC3));

	ActivationRequiredTags.AddTag(GT(TAG_CanCombo));
	ActivationRequiredTags.AddTag(GT(TAG_HC1));
	ActivationRequiredTags.AddTag(GT(TAG_HC2));

	ActivationBlockedTags.AddTag(GT(TAG_HC3));
}

UGA_Player_HeavyAtk4::UGA_Player_HeavyAtk4()
{
	AbilityTags.AddTag(GT(TAG_HC4));

	ActivationOwnedTags.AddTag(GT(TAG_HC1));
	ActivationOwnedTags.AddTag(GT(TAG_HC2));
	ActivationOwnedTags.AddTag(GT(TAG_HC3));
	ActivationOwnedTags.AddTag(GT(TAG_HC4));

	ActivationRequiredTags.AddTag(GT(TAG_CanCombo));
	ActivationRequiredTags.AddTag(GT(TAG_HC1));
	ActivationRequiredTags.AddTag(GT(TAG_HC2));
	ActivationRequiredTags.AddTag(GT(TAG_HC3));

	ActivationBlockedTags.AddTag(GT(TAG_HC4));
}

// ── Dash Attack ───────────────────────────────────────────────────────────
// 非连招，直接封锁三个阻断状态

UGA_Player_DashAtk::UGA_Player_DashAtk()
{
	AbilityTags.AddTag(GT(TAG_DA));
	ActivationOwnedTags.AddTag(GT(TAG_DA));

	ActivationBlockedTags.AddTag(GT(TAG_Dead));
	ActivationBlockedTags.AddTag(GT(TAG_HitReact));
	ActivationBlockedTags.AddTag(GT(TAG_Knockback));
}
