#include "RenderStyle/YogShadowTransitionLibrary.h"

namespace
{
    FYogShadowTransitionSettings NeutralShadowTransition()
    {
        FYogShadowTransitionSettings Neutral;
        Neutral.bEnabled = false;
        Neutral.Wrap = 0.f;
        Neutral.Strength = 0.f;
        Neutral.Tint = FLinearColor::White;
        return Neutral;
    }
}

FYogShadowTransitionSettings UYogShadowTransitionLibrary::GetGlobalShadowTransition()
{
    return NeutralShadowTransition();
}

void UYogShadowTransitionLibrary::SetGlobalShadowTransition(const FYogShadowTransitionSettings&)
{
    // Preserve the callable symbol for existing C++ and Blueprint references.
    // No CDO mutation or profile-table upload: legacy tint is retired.
}

bool UYogShadowTransitionLibrary::GetCharacterShadowTransition(int32 ProfileIndex,
    FYogShadowTransitionSettings& OutSettings, bool& bOutOverridden)
{
    OutSettings = NeutralShadowTransition();
    bOutOverridden = false;
    const UStylizedLightingSettings* Settings = GetDefault<UStylizedLightingSettings>();
    return ProfileIndex >= 0 && ProfileIndex < 8 && Settings->CharacterLightingProfiles.IsValidIndex(ProfileIndex);
}

bool UYogShadowTransitionLibrary::SetCharacterShadowTransitionOverride(int32,
    const FYogShadowTransitionSettings&)
{
    return false;
}

bool UYogShadowTransitionLibrary::ClearCharacterShadowTransitionOverride(int32)
{
    return false;
}
