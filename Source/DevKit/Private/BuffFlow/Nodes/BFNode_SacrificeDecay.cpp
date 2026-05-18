#include "BuffFlow/Nodes/BFNode_SacrificeDecay.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Component/BackpackGridComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameplayEffect.h"

UBFNode_SacrificeDecay::UBFNode_SacrificeDecay(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Sacrifice");
#endif
    InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
    OutputPins = {};
}

void UBFNode_SacrificeDecay::ExecuteBuffFlowInput(const FName& PinName)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (PinName == TEXT("In"))
    {
        if (!World->GetTimerManager().IsTimerActive(DecayTimerHandle))
        {
            CurrentDecayRate = BaseDecayRate;
            World->GetTimerManager().SetTimer(
                DecayTimerHandle, this, &UBFNode_SacrificeDecay::OnDecayTick, 1.f, true);
        }
    }
    else if (PinName == TEXT("Stop"))
    {
        World->GetTimerManager().ClearTimer(DecayTimerHandle);
        CurrentDecayRate = BaseDecayRate;
    }
}

void UBFNode_SacrificeDecay::OnDecayTick()
{
    UYogAbilitySystemComponent* ASC = GetOwnerASC();
    if (!ASC) return;

    // ── 热度衰退 ──────────────────────────────────────────────────────
    if (CurrentDecayRate > 0.f)
    {
        UGameplayEffect* HeatGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
        HeatGE->DurationPolicy = EGameplayEffectDurationType::Instant;

        FGameplayModifierInfo HeatMod;
        HeatMod.Attribute         = UBaseAttributeSet::GetHeatAttribute();
        HeatMod.ModifierOp        = EGameplayModOp::Additive;
        HeatMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-CurrentDecayRate));
        HeatGE->Modifiers.Add(HeatMod);

        FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
        FGameplayEffectSpec HeatSpec(HeatGE, Ctx, 1.f);
        ASC->ApplyGameplayEffectSpecToSelf(HeatSpec);
    }

    // 速率累加（上限 MaxDecayRate）
    CurrentDecayRate = FMath::Min(CurrentDecayRate + DecayAccelPerSecond, MaxDecayRate);

    // ── Phase 0 扣血 ───────────────────────────────────────────────────
    AYogCharacterBase* Owner = GetBuffOwner();
    if (!Owner) return;

    UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>();
    if (!BGC || BGC->GetCurrentPhase() != 0) return;

    // 当前 HP > HPDrainPerSecond 才扣血（非致命保底）
    const float CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
    if (CurrentHP <= HPDrainPerSecond) return;

    UGameplayEffect* HPGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
    HPGE->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo HPMod;
    HPMod.Attribute         = UBaseAttributeSet::GetHealthAttribute();
    HPMod.ModifierOp        = EGameplayModOp::Additive;
    HPMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-HPDrainPerSecond));
    HPGE->Modifiers.Add(HPMod);

    FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
    FGameplayEffectSpec HPSpec(HPGE, Ctx, 1.f);
    ASC->ApplyGameplayEffectSpecToSelf(HPSpec);
}

void UBFNode_SacrificeDecay::Cleanup()
{
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(DecayTimerHandle);

    CurrentDecayRate = BaseDecayRate;
    Super::Cleanup();
}
