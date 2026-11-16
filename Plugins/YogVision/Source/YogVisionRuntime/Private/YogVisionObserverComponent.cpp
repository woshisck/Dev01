#include "YogVisionObserverComponent.h"

UYogVisionObserverComponent::UYogVisionObserverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UYogVisionObserverComponent::SetVisionModifier(FName Token, float SightBonus, float SpiritBonus, float SpiritStrengthBonus)
{
    if (Token.IsNone()) return;
    FYogVisionModifier Modifier;
    Modifier.SightBonus = FMath::IsFinite(SightBonus) ? FMath::Clamp(SightBonus, -20000.f, 20000.f) : 0.f;
    Modifier.SpiritBonus = FMath::IsFinite(SpiritBonus) ? FMath::Clamp(SpiritBonus, -20000.f, 20000.f) : 0.f;
    Modifier.SpiritStrengthBonus = FMath::IsFinite(SpiritStrengthBonus) ? FMath::Clamp(SpiritStrengthBonus, -2.f, 2.f) : 0.f;
    Modifiers.Add(Token, Modifier);
}

void UYogVisionObserverComponent::RemoveVisionModifier(FName Token)
{
    Modifiers.Remove(Token);
}

void UYogVisionObserverComponent::ClearVisionModifiers()
{
    Modifiers.Reset();
}

FYogVisionParameters UYogVisionObserverComponent::ResolveParameters(const FYogVisionParameters& Base) const
{
    FYogVisionParameters Result = Base;
    Result.Sanitize();
    // Add first, clamp once: negative modifiers never depend on map iteration order.
    for (const auto& Pair : Modifiers)
    {
        Result.BaseSightRadius += Pair.Value.SightBonus;
        Result.SpiritRadius += Pair.Value.SpiritBonus;
        Result.SpiritStrength += Pair.Value.SpiritStrengthBonus;
    }
    Result.Sanitize();
    return Result;
}
