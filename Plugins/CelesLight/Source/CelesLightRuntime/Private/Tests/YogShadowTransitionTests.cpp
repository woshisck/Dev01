#include "StylizedLightingSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "StylizedCharacterLighting.h"
#include "UObject/UnrealType.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogShadowTransitionMathTest,
    "CelesLight.ShadowTransition.MathAndVisibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogShadowTransitionMathTest::RunTest(const FString&)
{
    FYogShadowTransitionSettings Settings;
    TestFalse(TEXT("Retired tint is disabled by default"), Settings.bEnabled);
    Settings.bEnabled = true;
    Settings.Wrap = 1.f;
    Settings.Falloff = 1.f;
    Settings.Strength = 1.f;
    Settings.Tint = FLinearColor(.2f, .5f, .9f, 1.f);
    for (const float NoL : {-1.f, 0.f, .1f, .5f, 1.f, std::numeric_limits<float>::quiet_NaN()})
    {
        TestEqual(TEXT("Legacy enabled values cannot produce a tint mask"), Settings.Evaluate(NoL), 0.f);
        TestEqual(TEXT("Retired tint cannot bypass projected shadow visibility"), Settings.Evaluate(NoL, 0.f), 0.f);
    }
    TestEqual(TEXT("Old enabled parameters upload only an inactive marker"), Settings.BuildParameters(), FVector4f(0.f, 0.f, 0.f, 0.f));
    TestEqual(TEXT("Old nonwhite tint uploads only neutral RGB and no override"), Settings.BuildTint(true), FVector4f(1.f, 1.f, 1.f, 0.f));
    TestTrue(TEXT("Sanitization preserves legacy stored enable for load compatibility"), Settings.Sanitized().bEnabled);
    TestEqual(TEXT("Sanitization preserves bounded legacy stored tint"), Settings.Sanitized().Tint, Settings.Tint);
    Settings.Wrap = std::numeric_limits<float>::quiet_NaN();
    Settings.Falloff = std::numeric_limits<float>::infinity();
    Settings.Strength = -std::numeric_limits<float>::infinity();
    Settings.Tint = FLinearColor(-1.f, 4.f, std::numeric_limits<float>::quiet_NaN(), 0.f);
    const FYogShadowTransitionSettings Safe = Settings.Sanitized();
    TestEqual(TEXT("Compatibility scalar fallback stays stable"), Safe.Wrap, .35f);
    TestEqual(TEXT("Compatibility falloff fallback stays stable"), Safe.Falloff, 2.f);
    TestEqual(TEXT("Compatibility strength fallback stays stable"), Safe.Strength, .35f);
    TestEqual(TEXT("Compatibility tint remains finite and bounded"), Safe.Tint, FLinearColor(0.f, 1.f, 1.f, 1.f));
    TestEqual(TEXT("Invalid legacy data also uploads inactive parameters"), Safe.BuildParameters(), FVector4f(0.f, 0.f, 0.f, 0.f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogShadowTransitionRoutingTest,
    "CelesLight.ShadowTransition.InheritanceAndIsolatedUpload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogShadowTransitionRoutingTest::RunTest(const FString&)
{
    struct FRestore
    {
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
        uint32 Count = 0;
        ~FRestore() { SetStylizedCharacterLightingProfiles(MakeArrayView(Packed.GetData(), Packed.Num()), Count); }
    } Restore;
    GetStylizedCharacterLightingProfiles(Restore.Packed, Restore.Count);
    UStylizedLightingSettings* Settings = NewObject<UStylizedLightingSettings>();
    if (!TestTrue(TEXT("Character settings contain a default profile"), Settings->CharacterLightingProfiles.Num() > 0)) return false;
    Settings->CharacterLightingProfiles[0].bEnableNPRMetal = true; // Explicit fixture; saved user opt-outs remain valid.
    FStylizedCharacterLightingProfile LegacyProfile;
    TestFalse(TEXT("New material profiles do not implicitly opt into NPR metal"), LegacyProfile.bEnableNPRMetal);
    const TCHAR* LegacyEnd = FStylizedCharacterLightingProfile::StaticStruct()->ImportText(
        TEXT("(ProfileName=\"Legacy\",SpecularIntensity=1.7)"), &LegacyProfile, nullptr, PPF_SerializedAsImportText, GWarn, TEXT("LegacyNPRMetalProfile"));
    TestNotNull(TEXT("Legacy profile data remains importable"), LegacyEnd);
    TestFalse(TEXT("Missing new field keeps legacy nondefault profiles neutral"), LegacyProfile.bEnableNPRMetal);
    TestEqual(TEXT("Legacy material strength remains unchanged"), LegacyProfile.SpecularIntensity, 1.7f);
    LegacyProfile.bEnableNPRMetal = true;
    const TCHAR* DisabledEnd = FStylizedCharacterLightingProfile::StaticStruct()->ImportText(
        TEXT("(bEnableNPRMetal=False)"), &LegacyProfile, nullptr, PPF_SerializedAsImportText, GWarn, TEXT("DisabledNPRMetalProfile"));
    TestNotNull(TEXT("Explicit saved NPR disable remains importable"), DisabledEnd);
    TestFalse(TEXT("Explicit false is never forcibly re-enabled"), LegacyProfile.bEnableNPRMetal);
    Settings->CharacterLightingProfiles.SetNum(2);
    TestFalse(TEXT("Additional profiles start with NPR metal disabled"), Settings->CharacterLightingProfiles[1].bEnableNPRMetal);
    Settings->SceneShadowTransition.bEnabled = true;
    Settings->SceneShadowTransition.Wrap = 1.f;
    Settings->SceneShadowTransition.Strength = 1.f;
    Settings->SceneShadowTransition.Tint = FLinearColor::Red;
    FStylizedCharacterLightingProfile& Profile = Settings->CharacterLightingProfiles[1];
    Profile.bOverrideShadowTransition = true;
    Profile.ShadowTransition = Settings->SceneShadowTransition;
    Profile.ShadowTransition.Tint = FLinearColor::Blue;
    TestFalse(TEXT("Legacy inherited effective transition is disabled"),
        Settings->CharacterLightingProfiles[0].ResolveShadowTransition(Settings->SceneShadowTransition).bEnabled);
    TestFalse(TEXT("Legacy enabled character override is also disabled"),
        Profile.ResolveShadowTransition(Settings->SceneShadowTransition).bEnabled);
    TestEqual(TEXT("Legacy override effective color is neutral"),
        Profile.ResolveShadowTransition(Settings->SceneShadowTransition).Tint, FLinearColor::White);
    TMap<FString, FString> Before;
    for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
        TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.Shadow.Virtual.Enable"), TEXT("r.DefaultFeature.AutoExposure"), TEXT("r.EyeAdaptationQuality")})
        if (const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) Before.Add(Name, Variable->GetString());
    Settings->ApplyShadowTransitionParameters();
    TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
    uint32 Count = 0;
    GetStylizedCharacterLightingProfiles(Packed, Count);
    TestEqual(TEXT("Twelve-float4 profile ABI is unchanged"), StylizedCharacterLighting::Float4sPerProfile, 12u);
    TestEqual(TEXT("Normal character profiles still upload"), Count, 2u);
    for (uint32 Index = 0; Index < Count; ++Index)
    {
        const uint32 Base = Index * StylizedCharacterLighting::Float4sPerProfile;
        TestEqual(TEXT("Larger default width and base block size reach the GPU"), Packed[Base], FVector4f(.18f,.012f,1.5f,.30f));
        TestEqual(TEXT("Stronger default highlight and midpoint blend reach the GPU"), Packed[Base + 1], FVector4f(3.2f,4.5f,1.f,.5f));
        TestEqual(TEXT("Dark strengthening reuses one retired environment lane"), FVector3f(Packed[Base + 2]), FVector3f(.35f,0.f,0.f));
        TestEqual(TEXT("Nonmetal reflection alpha default"), Packed[Base + 2].W, .3f);
        TestEqual(TEXT("Metal reflection alpha default"), Packed[Base + 3], FVector4f(1.f,1.f,1.f,.7f));
        TestEqual(TEXT("Retirement does not replace authored GGX intensity"),
            Packed[Base + 7].W, Settings->CharacterLightingProfiles[Index].SpecularIntensity);
        TestEqual(TEXT("NPR metal uses only the previously reserved final lane"), Packed[Base + 11].W,
            Settings->CharacterLightingProfiles[Index].bEnableNPRMetal ? 1.f : 0.f);
    }
    Settings->MetalArt.HighlightWidth = 1.6f;
    Profile.bOverrideMetalArt = true;
    Profile.MetalArt.CoreStrength = 7.f;
    Settings->MetalArt.BaseBlockSize = .42f;
    Profile.MetalArt.BaseBlockSize = .65f;
    Settings->MetalArt.DarkSideStrength = .23f;
    Profile.MetalArt.DarkSideStrength = .81f;
    Settings->NonMetalReflectionAlpha = .25f;
    Settings->MetalReflectionAlpha = .85f;
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed, Count);
    TestEqual(TEXT("Global artist width reaches inherited profile"), Packed[0].Z, 1.6f);
    TestEqual(TEXT("Override width stays independent"), Packed[12].Z, 1.5f);
    TestEqual(TEXT("Global base size reaches inherited profile"), Packed[0].W, .42f);
    TestEqual(TEXT("Profile base size stays independent"), Packed[12].W, .65f);
    TestEqual(TEXT("Global dark strengthening reaches inherited profile"), Packed[2].X, .23f);
    TestEqual(TEXT("Profile dark strengthening stays independent"), Packed[14].X, .81f);
    TestEqual(TEXT("Override core reaches GPU"), Packed[13].Y, 7.f);
    TestEqual(TEXT("Nonmetal reflection lerp reaches GPU"), Packed[2].W, .25f);
    TestEqual(TEXT("Metal reflection lerp reaches GPU"), Packed[15].W, .85f);
    Settings->MetalArt.HighlightMode = EStylizedMetalHighlightMode::PBR;
    Settings->MetalArt.Stylization = .73f;
    Profile.MetalArt.HighlightMode = EStylizedMetalHighlightMode::Stylized;
    Profile.MetalArt.Stylization = .24f;
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed, Count);
    TestEqual(TEXT("Global PBR mode reaches inherited profile"), Packed[1].Z, 0.f);
    TestEqual(TEXT("PBR mode remembers but does not activate the artist blend"), Packed[1].W, .73f);
    TestEqual(TEXT("Profile can retain independent stylized mode"), Packed[13].Z, 1.f);
    TestEqual(TEXT("Profile blend is independent from global"), Packed[13].W, .24f);
    Settings->MetalArt.Stylization = 2.f;
    Profile.MetalArt.Stylization = -1.f;
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed, Count);
    TestEqual(TEXT("Oversized global blend is safely clamped"), Packed[1].W, 1.f);
    TestEqual(TEXT("Negative profile blend is safely clamped"), Packed[13].W, 0.f);
    FString SerializedMetal;
    FStylizedMetalArtSettings::StaticStruct()->ExportText(SerializedMetal, &Profile.MetalArt, nullptr, nullptr, PPF_SerializedAsImportText, nullptr);
    FStylizedMetalArtSettings ReloadedMetal;
    const TCHAR* MetalEnd = FStylizedMetalArtSettings::StaticStruct()->ImportText(*SerializedMetal, &ReloadedMetal, nullptr, PPF_SerializedAsImportText, GWarn, TEXT("MetalModeRoundTrip"));
    TestNotNull(TEXT("Mode and blend serialization round-trips without an asset save"), MetalEnd);
    TestEqual(TEXT("Round-trip preserves the selected mode"), ReloadedMetal.HighlightMode, Profile.MetalArt.HighlightMode);
    TestEqual(TEXT("Round-trip preserves the raw editable blend"), ReloadedMetal.Stylization, Profile.MetalArt.Stylization);
    TestEqual(TEXT("Round-trip preserves base block size"), ReloadedMetal.BaseBlockSize, Profile.MetalArt.BaseBlockSize);
    TestEqual(TEXT("Round-trip preserves dark strengthening"), ReloadedMetal.DarkSideStrength, Profile.MetalArt.DarkSideStrength);
    Settings->MetalArt.BaseBlockSize = std::numeric_limits<float>::quiet_NaN();
    Settings->MetalArt.DarkSideStrength = -1.f;
    Profile.MetalArt.BaseBlockSize = 2.f;
    Profile.MetalArt.DarkSideStrength = std::numeric_limits<float>::infinity();
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed, Count);
    TestEqual(TEXT("Nonfinite base size has a finite default fallback"), Packed[0].W, .30f);
    TestEqual(TEXT("Negative darkening clamps to neutral"), Packed[2].X, 0.f);
    TestEqual(TEXT("Oversized profile base size is safely bounded"), Packed[12].W, 1.f);
    TestEqual(TEXT("Nonfinite profile darkening has a finite default fallback"), Packed[14].X, .35f);
    const auto BeforeRetiredBand = Packed;
    Settings->MetalArt.EnvironmentStrength = 4.f;
    Settings->MetalArt.EnvironmentWidth = 3.f;
    Settings->MetalArt.BandDirection = FVector(1, 0, 0);
    Profile.MetalArt.EnvironmentStrength = 0.f;
    Profile.MetalArt.EnvironmentWidth = .1f;
    Profile.MetalArt.BandDirection = FVector(0, -1, 0);
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed, Count);
    for (int32 Index = 0; Index < Packed.Num(); ++Index)
        TestEqual(TEXT("Legacy global/profile environment-band values have no GPU effect"), Packed[Index], BeforeRetiredBand[Index]);
    for (const TCHAR* Name : {TEXT("EnvironmentStrength"), TEXT("EnvironmentWidth"), TEXT("BandDirection")})
    {
        const FProperty* Property = FindFProperty<FProperty>(FStylizedMetalArtSettings::StaticStruct(), Name);
        if (TestNotNull(TEXT("Legacy environment-band field remains loadable"), Property))
            TestFalse(TEXT("Legacy environment-band field is not an artist control"), Property->HasAnyPropertyFlags(CPF_Edit));
    }
    const auto BeforeNPRToggle = Packed;
    Settings->CharacterLightingProfiles[0].bEnableNPRMetal = false;
    Settings->CharacterLightingProfiles[1].bEnableNPRMetal = true;
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed, Count);
    for (uint32 Index = 0; Index < Count; ++Index)
    {
        const uint32 Base = Index * StylizedCharacterLighting::Float4sPerProfile;
        for (uint32 Slot = 0; Slot < 11; ++Slot)
            TestEqual(TEXT("NPR switch leaves all prior profile vectors unchanged"), Packed[Base + Slot], BeforeNPRToggle[Base + Slot]);
        TestEqual(TEXT("NPR keeps direct light influence"), Packed[Base + 11].X, BeforeNPRToggle[Base + 11].X);
        TestEqual(TEXT("NPR keeps the unused lane neutral"), Packed[Base + 11].Y, BeforeNPRToggle[Base + 11].Y);
        TestEqual(TEXT("NPR keeps existing character feature bits"), Packed[Base + 11].Z, BeforeNPRToggle[Base + 11].Z);
        TestEqual(TEXT("Each material profile can independently enable or disable NPR"), Packed[Base + 11].W, Index == 0 ? 0.f : 1.f);
    }
    TestTrue(TEXT("Legacy scene fields are retained for loading"), Settings->SceneShadowTransition.bEnabled);
    TestTrue(TEXT("Legacy character fields are retained for loading"), Profile.bOverrideShadowTransition);
    for (const auto& Pair : Before)
        TestEqual(*FString::Printf(TEXT("No native, exposure or cloud CVar reset: %s"), *Pair.Key), IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->GetString(), Pair.Value);
    return true;
}
#endif
