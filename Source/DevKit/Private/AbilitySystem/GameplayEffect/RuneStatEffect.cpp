#include "AbilitySystem/GameplayEffect/RuneStatEffect.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

// ─── 辅助宏：快速添加 Additive Modifier ─────────────────────────
#define ADD_MODIFIER(AttributeGetter, Op, Value)                    \
{                                                                    \
    FGameplayModifierInfo ModInfo;                                   \
    ModInfo.Attribute = AttributeGetter();                           \
    ModInfo.ModifierOp = Op;                                         \
    ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(    \
        FScalableFloat(Value));                                      \
    Modifiers.Add(ModInfo);                                          \
}

// ─── Base ─────────────────────────────────────────────────────────
URuneStatEffect_Base::URuneStatEffect_Base()
{
    // DurationPolicy 已由 PowerUpEffect 设为 Infinite
}

// ─── Attack +20 ───────────────────────────────────────────────────
URuneStatEffect_Attack::URuneStatEffect_Attack()
{
    ADD_MODIFIER(UBaseAttributeSet::GetAttackAttribute, EGameplayModOp::Additive, 20.f);
}

// ─── AttackPower +0.1（即 10% 乘数增量）─────────────────────────
URuneStatEffect_AttackPower::URuneStatEffect_AttackPower()
{
    ADD_MODIFIER(UBaseAttributeSet::GetAttackPowerAttribute, EGameplayModOp::Additive, 0.1f);
}

// ─── MaxHealth +50 ────────────────────────────────────────────────
URuneStatEffect_MaxHealth::URuneStatEffect_MaxHealth()
{
    ADD_MODIFIER(UBaseAttributeSet::GetMaxHealthAttribute, EGameplayModOp::Additive, 50.f);
}

// ─── AttackSpeed +0.1 ─────────────────────────────────────────────
URuneStatEffect_AttackSpeed::URuneStatEffect_AttackSpeed()
{
    ADD_MODIFIER(UBaseAttributeSet::GetAttackSpeedAttribute, EGameplayModOp::Additive, 0.1f);
}

// ─── MoveSpeed +50 ────────────────────────────────────────────────
URuneStatEffect_MoveSpeed::URuneStatEffect_MoveSpeed()
{
    ADD_MODIFIER(UBaseAttributeSet::GetMoveSpeedAttribute, EGameplayModOp::Additive, 50.f);
}

// ─── Crit_Rate +0.05（5%）────────────────────────────────────────
URuneStatEffect_CritRate::URuneStatEffect_CritRate()
{
    ADD_MODIFIER(UBaseAttributeSet::GetCrit_RateAttribute, EGameplayModOp::Additive, 0.05f);
}

// ─── Crit_Damage +0.2（20%）──────────────────────────────────────
URuneStatEffect_CritDamage::URuneStatEffect_CritDamage()
{
    ADD_MODIFIER(UBaseAttributeSet::GetCrit_DamageAttribute, EGameplayModOp::Additive, 0.2f);
}

// ─── DmgTaken +0.1（受伤增加 10%，慎用）─────────────────────────
URuneStatEffect_DmgTaken::URuneStatEffect_DmgTaken()
{
    ADD_MODIFIER(UBaseAttributeSet::GetDmgTakenAttribute, EGameplayModOp::Additive, 0.1f);
}

// ─── Dodge +0.05（闪避 +5%）──────────────────────────────────────
URuneStatEffect_Dodge::URuneStatEffect_Dodge()
{
    ADD_MODIFIER(UBaseAttributeSet::GetDodgeAttribute, EGameplayModOp::Additive, 0.05f);
}

// ─── AttackRange +50 ──────────────────────────────────────────────
URuneStatEffect_AttackRange::URuneStatEffect_AttackRange()
{
    ADD_MODIFIER(UBaseAttributeSet::GetAttackRangeAttribute, EGameplayModOp::Additive, 50.f);
}
