#include "AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
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

static FORCEINLINE FGameplayTag GT(const TCHAR* Name)
{
	return FGameplayTag::RequestGameplayTag(FName(Name));
}

// ── 常用 Tag ──────────────────────────────────────────────────────────────


// Light combo

// Heavy combo

// Dash attack
// NOTE: TAG_Dead 已在 GA_EnemyMeleeAttacks.cpp 定义，此处不重复定义
//       避免 UE5 adaptive non-unity 批次编译时产生 C2374 重定义错误
static const FName TAG_HitReact (TEXT("Buff.Status.HitReact"));

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
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));

	// 首段无 Required
	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));
}

UGA_Player_LightAtk2::UGA_Player_LightAtk2()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));

	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.CanCombo")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));
}

UGA_Player_LightAtk3::UGA_Player_LightAtk3()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo3")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo3")));

	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.CanCombo")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo3")));
}

UGA_Player_LightAtk4::UGA_Player_LightAtk4()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo4")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo3")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo4")));

	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.CanCombo")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo1")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo2")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo3")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.LightAtk.Combo4")));
}

void UGA_Player_LightAtk4::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 消费冲刺连招保存的 LooseGameplayTags（若非 DashSave 触发则为空操作）
	if (UYogAbilitySystemComponent* YASC = Cast<UYogAbilitySystemComponent>(
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
	{
		YASC->ConsumeDashSave();
	}
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// ── Heavy Attack Combo ────────────────────────────────────────────────────

UGA_Player_HeavyAtk1::UGA_Player_HeavyAtk1()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));
}

UGA_Player_HeavyAtk2::UGA_Player_HeavyAtk2()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));

	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.CanCombo")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));
}

UGA_Player_HeavyAtk3::UGA_Player_HeavyAtk3()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3")));

	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.CanCombo")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3")));
}

UGA_Player_HeavyAtk4::UGA_Player_HeavyAtk4()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4")));

	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4")));

	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.CanCombo")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2")));
	ActivationRequiredTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3")));

	ActivationBlockedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4")));
}

void UGA_Player_HeavyAtk4::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 消费冲刺连招保存的 LooseGameplayTags（若非 DashSave 触发则为空操作）
	if (UYogAbilitySystemComponent* YASC = Cast<UYogAbilitySystemComponent>(
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
	{
		YASC->ConsumeDashSave();
	}
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// ── Dash Attack ───────────────────────────────────────────────────────────
// 非连招，直接封锁三个阻断状态

UGA_Player_DashAtk::UGA_Player_DashAtk()
{
	AbilityTags.AddTag(GT(TEXT("PlayerState.AbilityCast.DashAtk")));
	ActivationOwnedTags.AddTag(GT(TEXT("PlayerState.AbilityCast.DashAtk")));

	ActivationBlockedTags.AddTag(GT(TEXT("Buff.Status.Dead")));
	ActivationBlockedTags.AddTag(GT(TEXT("Buff.Status.HitReact")));
	ActivationBlockedTags.AddTag(GT(TEXT("Buff.Status.Knockback")));
}
