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


// ═══════════════════════════════════════════════════════════════
//  4.15 符文 GE 实现
// ═══════════════════════════════════════════════════════════════

// ─── 振奋（3s 有时限，叠加最多5层，+1 Attack/层）──────────────
UGE_Rune_ZhenFen::UGE_Rune_ZhenFen()
{
    DurationPolicy  = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

    ADD_MODIFIER(UBaseAttributeSet::GetAttackAttribute, EGameplayModOp::Additive, 1.f);

    StackingType                   = EGameplayEffectStackingType::AggregateByTarget;
    StackLimitCount                = 5;
    StackDurationRefreshPolicy     = EGameplayEffectStackingDurationPolicy::NeverRefresh;
    StackExpirationPolicy          = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}

// ─── 奋力一击 BehaviorGE ────────────────────────────────────────
UGE_Rune_FenLiYiJi::UGE_Rune_FenLiYiJi()
{
    InheritableOwnedTagsContainer.Added.AddTag(
        FGameplayTag::RequestGameplayTag(TEXT("Rune.FenLiYiJi.Active")));
}

// ─── 突袭 BehaviorGE ────────────────────────────────────────────
UGE_Rune_TuXi::UGE_Rune_TuXi()
{
    InheritableOwnedTagsContainer.Added.AddTag(
        FGameplayTag::RequestGameplayTag(TEXT("Rune.TuXi.Active")));
}

// ─── 双重打击 BehaviorGE ────────────────────────────────────────
UGE_Rune_ShuangChongDaJi::UGE_Rune_ShuangChongDaJi()
{
    InheritableOwnedTagsContainer.Added.AddTag(
        FGameplayTag::RequestGameplayTag(TEXT("Rune.ShuangChongDaJi.Active")));
}

// ─── 风行者 BehaviorGE ──────────────────────────────────────────
UGE_Rune_FengXingZhe::UGE_Rune_FengXingZhe()
{
    InheritableOwnedTagsContainer.Added.AddTag(
        FGameplayTag::RequestGameplayTag(TEXT("Rune.FengXingZhe.Active")));
}

// ─── 滑行速度增益（3s，MoveSpeed +120）──────────────────────────
UGE_Buff_SlideSpeed::UGE_Buff_SlideSpeed()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

    ADD_MODIFIER(UBaseAttributeSet::GetMoveSpeedAttribute, EGameplayModOp::Additive, 120.f);
}

// ─── 蛇咬中毒（5s，每秒 -MaxHP*2%）─────────────────────────────
UGE_Buff_Poison_Rune::UGE_Buff_Poison_Rune()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
    Period            = 1.0f;
    bExecutePeriodicEffectOnApplication = false;

    // Health -= Target.MaxHealth * 0.02 per tick
    FGameplayEffectAttributeCaptureDefinition CaptureMaxHealth(
        UBaseAttributeSet::GetMaxHealthAttribute(),
        EGameplayEffectAttributeCaptureSource::Target,
        false);

    FAttributeBasedFloat AttrBased;
    AttrBased.Coefficient                = FScalableFloat(-0.02f);
    AttrBased.PreMultiplyAdditiveValue   = FScalableFloat(0.f);
    AttrBased.PostMultiplyAdditiveValue  = FScalableFloat(0.f);
    AttrBased.BackingAttribute           = CaptureMaxHealth;
    AttrBased.AttributeCalculationType   = EAttributeBasedFloatCalculationType::AttributeMagnitude;

    FGameplayModifierInfo PoisonMod;
    PoisonMod.Attribute        = UBaseAttributeSet::GetHealthAttribute();
    PoisonMod.ModifierOp       = EGameplayModOp::Additive;
    PoisonMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttrBased);
    Modifiers.Add(PoisonMod);

    StackingType      = EGameplayEffectStackingType::AggregateBySource;
    StackLimitCount   = 1;
    StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
    StackExpirationPolicy      = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}

// ─── 战斗渴望 BehaviorGE ────────────────────────────────────────
UGE_Rune_ZhanDouKewang::UGE_Rune_ZhanDouKewang()
{
    InheritableOwnedTagsContainer.Added.AddTag(
        FGameplayTag::RequestGameplayTag(TEXT("Rune.ZhanDouKewang.Active")));
}

// ─── 战斗渴望动态攻速 GE（Infinite，SetByCaller AttackSpeed）─────
UGE_Buff_ZhanDouKewang::UGE_Buff_ZhanDouKewang()
{
    // DurationPolicy = Infinite (inherited from UPowerUpEffect)
    FSetByCallerFloat SBC;
    SBC.DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.ZhanDouKewang.AttackSpeedBonus"));

    FGameplayModifierInfo ModInfo;
    ModInfo.Attribute        = UBaseAttributeSet::GetAttackSpeedAttribute();
    ModInfo.ModifierOp       = EGameplayModOp::Additive;
    ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
    Modifiers.Add(ModInfo);
}

// ─── 全能 BehaviorGE ────────────────────────────────────────────
UGE_Rune_QuanNeng::UGE_Rune_QuanNeng()
{
    InheritableOwnedTagsContainer.Added.AddTag(
        FGameplayTag::RequestGameplayTag(TEXT("Rune.QuanNeng.Active")));
}

// ─── 全能动态暴击率 GE（5s，SetByCaller Crit_Rate）──────────────
UGE_Buff_QuanNeng::UGE_Buff_QuanNeng()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));

    FSetByCallerFloat SBC;
    SBC.DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.QuanNeng.CritRateBonus"));

    FGameplayModifierInfo ModInfo;
    ModInfo.Attribute        = UBaseAttributeSet::GetCrit_RateAttribute();
    ModInfo.ModifierOp       = EGameplayModOp::Additive;
    ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
    Modifiers.Add(ModInfo);
}
