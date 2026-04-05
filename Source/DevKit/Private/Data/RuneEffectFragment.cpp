#include "Data/RuneEffectFragment.h"
#include "Data/RuneDataAsset.h"
#include "AbilitySystemComponent.h"

// ============================================================
//  URuneEffect_AttributeModifier
// ============================================================

void URuneEffect_AttributeModifier::ApplyToGE(UGameplayEffect* GE) const
{
    if (!GE || !Attribute.IsValid())
        return;

    FGameplayModifierInfo ModInfo;
    ModInfo.Attribute       = Attribute;
    ModInfo.ModifierOp      = ModOp;
    ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
    GE->Modifiers.Add(ModInfo);
}


// ============================================================
//  URuneEffect_AddTags
// ============================================================

void URuneEffect_AddTags::ApplyToGE(UGameplayEffect* GE) const
{
    if (!GE || Tags.IsEmpty())
        return;

    GE->InheritableOwnedTagsContainer.Added.AppendTags(Tags);
}


// ============================================================
//  URuneEffect_TriggerGA
// ============================================================

FGameplayAbilitySpecHandle URuneEffect_TriggerGA::OnActivate(UAbilitySystemComponent* ASC, URuneDataAsset* SourceDA) const
{
    if (!ASC || !AbilityClass)
        return FGameplayAbilitySpecHandle();

    FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
    AbilitySpec.SourceObject = SourceDA;
    return ASC->GiveAbility(AbilitySpec);
}

void URuneEffect_TriggerGA::OnDeactivate(UAbilitySystemComponent* ASC, const FGameplayAbilitySpecHandle& Handle) const
{
    if (ASC && Handle.IsValid())
        ASC->ClearAbility(Handle);
}


// ============================================================
//  URuneEffect_GameplayCue
// ============================================================

void URuneEffect_GameplayCue::ApplyToGE(UGameplayEffect* GE) const
{
    if (!GE)
        return;

    GE->GameplayCues.Append(Cues);
}


// ============================================================
//  URuneEffect_Modifier
// ============================================================

void URuneEffect_Modifier::ApplyToGE(UGameplayEffect* GE) const
{
    if (!GE || !ModifierInfo.Attribute.IsValid())
        return;

    GE->Modifiers.Add(ModifierInfo);
}


// ============================================================
//  URuneEffect_Execution
// ============================================================

void URuneEffect_Execution::ApplyToGE(UGameplayEffect* GE) const
{
    if (!GE)
        return;

    GE->Executions.Add(ExecutionDefinition);
}
