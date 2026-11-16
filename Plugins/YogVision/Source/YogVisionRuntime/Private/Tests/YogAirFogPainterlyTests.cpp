#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "YogAirFogVolume.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogPainterlyParametersTest, "YogVision.AirFog.Painterly.ParameterContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogPainterlyParametersTest::RunTest(const FString&)
{
    const FYogAirFogParameters Defaults;
    TestTrue(TEXT("Artist material palette is the default colour authority"), Defaults.bUseArtistPalette);
    TestTrue(TEXT("Scene directional lighting is read automatically by default"), Defaults.bUseSceneDirectionalLight);
    TestEqual(TEXT("Palette transitions along 35 metres of camera depth"), Defaults.PaletteDistance, 3500.f);
    TestEqual(TEXT("The height reference is 11 metres below the camera"), Defaults.CameraHeightOffset, 1100.f);
    TestEqual(TEXT("Near growth uses an absolute 18-metre camera depth"), Defaults.NearOffset, 1800.f);
    TestTrue(TEXT("Legacy distant structure distances remain serializable"), Defaults.FarStart == 2200.f && Defaults.FarEnd == 5000.f);
    TestTrue(TEXT("Directional form defaults retain restrained light and shadow"),
        Defaults.FormLightYaw == -45.f && Defaults.FormLightPitch == 35.f
        && Defaults.FormLightStrength == .4f && Defaults.FormShadowStrength == .16f);
    TestTrue(TEXT("Projected native fog strokes use the requested scale and slow flow"),
        Defaults.StrokeStrength == .12f && Defaults.StrokeScale == 3.f
        && Defaults.StrokeFlowSpeed == .2f && Defaults.StrokeWarp == .12f);
    TestTrue(TEXT("Volume detail has independent spatial controls"),
        Defaults.BoundaryDetail == .45f && Defaults.BoundaryDetailScale == .02f && Defaults.VolumeFormStrength == .5f);

    // Exercise invalid values arriving from a draft/Blueprint, then compare the
    // public tuning bounds to the sanitizer rather than duplicating its formula.
    FYogAirFogParameters P = Defaults;
    for (TFieldIterator<FFloatProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
        It->SetPropertyValue_InContainer(&P, std::numeric_limits<float>::quiet_NaN());
    P.FormLightColor = FLinearColor(std::numeric_limits<float>::infinity(), -1.f,
        std::numeric_limits<float>::quiet_NaN(), 0.f);
    P.Sanitize();
    for (TFieldIterator<FFloatProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
    {
        const float Value = It->GetPropertyValue_InContainer(&P);
        TestTrue(FString::Printf(TEXT("%s remains finite after invalid draft input"), *It->GetName()), FMath::IsFinite(Value));
        TestEqual(FString::Printf(TEXT("%s returns its declared default after NaN input"), *It->GetName()),
            Value, It->GetPropertyValue_InContainer(&Defaults));
    }
    TestTrue(TEXT("Directional colour is finite, nonnegative, opaque and bounded"),
        FMath::IsFinite(P.FormLightColor.R) && FMath::IsFinite(P.FormLightColor.G) && FMath::IsFinite(P.FormLightColor.B)
        && P.FormLightColor.R >= 0.f && P.FormLightColor.R <= 10.f && P.FormLightColor.G == 0.f && P.FormLightColor.A == 1.f);
#if WITH_EDITOR
    for (const float Extreme : { -1.e9f, 1.e9f, std::numeric_limits<float>::infinity() })
    {
        P = Defaults;
        for (TFieldIterator<FFloatProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
            It->SetPropertyValue_InContainer(&P, Extreme);
        P.Sanitize();
        for (TFieldIterator<FFloatProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
        {
            const float Value = It->GetPropertyValue_InContainer(&P);
            if (It->HasMetaData(TEXT("ClampMin")))
                TestTrue(FString::Printf(TEXT("%s honours the displayed minimum"), *It->GetName()), Value >= It->GetFloatMetaData(TEXT("ClampMin")));
            if (It->HasMetaData(TEXT("ClampMax")))
                TestTrue(FString::Printf(TEXT("%s honours the displayed maximum"), *It->GetName()), Value <= It->GetFloatMetaData(TEXT("ClampMax")));
        }
        TestTrue(TEXT("Sanitizing any draft preserves the three absolute density bounds"),
            P.ClearMinDensity >= 0.f && P.ClearMinDensity <= P.NearDensity && P.NearDensity <= P.Density);
        TestTrue(TEXT("Sanitizing any draft preserves a nonempty far interval"), P.FarEnd > P.FarStart);
    }
#endif
    return true;
}

#endif
