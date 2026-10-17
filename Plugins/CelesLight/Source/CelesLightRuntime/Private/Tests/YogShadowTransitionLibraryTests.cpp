#if WITH_DEV_AUTOMATION_TESTS

#include "RenderStyle/YogShadowTransitionLibrary.h"

#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StylizedCharacterLighting.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

#include <limits>

namespace
{
    struct FScopedShadowTransitionLibraryState
    {
        UStylizedLightingSettings* Settings = GetMutableDefault<UStylizedLightingSettings>();
        FYogShadowTransitionSettings Scene = Settings->SceneShadowTransition;
        TArray<FStylizedCharacterLightingProfile> Profiles = Settings->CharacterLightingProfiles;
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
        uint32 Count = 0;

        FScopedShadowTransitionLibraryState() { GetStylizedCharacterLightingProfiles(Packed, Count); }
        ~FScopedShadowTransitionLibraryState()
        {
            Settings->SceneShadowTransition = Scene;
            Settings->CharacterLightingProfiles = Profiles;
            SetStylizedCharacterLightingProfiles(MakeArrayView(Packed.GetData(), Packed.Num()), Count);
        }
    };

    FString ExportAllShadowTransitionSettings(const UStylizedLightingSettings* Settings)
    {
        FString Text;
        for (TFieldIterator<FProperty> It(Settings->GetClass()); It; ++It)
        {
            FString Value;
            It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(Settings), nullptr, nullptr, PPF_None);
            Text += It->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        return Text;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogShadowTransitionRuntimeLibraryTest,
    "CelesLight.ShadowTransition.RuntimeLibrarySharedOverrides",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogShadowTransitionRuntimeLibraryTest::RunTest(const FString&)
{
    FScopedShadowTransitionLibraryState Restore;
    UStylizedLightingSettings* Settings = Restore.Settings;
    Settings->CharacterLightingProfiles.SetNum(2);
    Settings->SceneShadowTransition.bEnabled = true;
    Settings->SceneShadowTransition.Wrap = 1.f;
    Settings->SceneShadowTransition.Strength = 1.f;
    Settings->SceneShadowTransition.Tint = FLinearColor(.8f, .2f, .1f, 1.f);
    Settings->CharacterLightingProfiles[0].ProfileName = TEXT("LegacyTransitionA");
    Settings->CharacterLightingProfiles[0].bOverrideShadowTransition = true;
    Settings->CharacterLightingProfiles[0].ShadowTransition = Settings->SceneShadowTransition;
    Settings->CharacterLightingProfiles[0].ShadowTransition.Tint = FLinearColor::Blue;
    Settings->CharacterLightingProfiles[1].ProfileName = TEXT("LegacyTransitionB");
    Settings->CharacterLightingProfiles[1].bOverrideShadowTransition = false;
    const FString AllSettingsBefore = ExportAllShadowTransitionSettings(Settings);

    struct FCVarSnapshot { FString Value; EConsoleVariableFlags Flags; };
    TMap<IConsoleVariable*, FCVarSnapshot> CVars;
    for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
        TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
        TEXT("r.StylizedCharacterLighting.DirectColorInfluence"), TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"),
        TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence"), TEXT("r.DynamicGlobalIlluminationMethod"),
        TEXT("r.ReflectionMethod"), TEXT("r.Shadow.Virtual.Enable"), TEXT("r.DefaultFeature.AutoExposure"), TEXT("r.EyeAdaptationQuality")})
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
            CVars.Add(CVar, {CVar->GetString(), CVar->GetFlags()});
    TMap<FString, TArray<uint8>> ConfigBytes;
    for (const FString& Path : {FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"), GGameIni})
    {
        TArray<uint8> Bytes;
        if (FFileHelper::LoadFileToArray(Bytes, *Path)) ConfigBytes.Add(Path, MoveTemp(Bytes));
    }
    TMap<TWeakObjectPtr<UPackage>, bool> DirtyPackages;
    for (TObjectIterator<UPackage> It; It; ++It)
        if (!It->HasAnyFlags(RF_Transient)) DirtyPackages.Add(TWeakObjectPtr<UPackage>(*It), It->IsDirty());
    TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> PackedBefore;
    uint32 CountBefore = 0;
    GetStylizedCharacterLightingProfiles(PackedBefore, CountBefore);

    FYogShadowTransitionSettings LegacyRequest;
    LegacyRequest.bEnabled = true;
    LegacyRequest.Wrap = 1.f;
    LegacyRequest.Strength = 1.f;
    LegacyRequest.Tint = FLinearColor::Green;
    UYogShadowTransitionLibrary::SetGlobalShadowTransition(LegacyRequest);
    const FYogShadowTransitionSettings Global = UYogShadowTransitionLibrary::GetGlobalShadowTransition();
    TestFalse(TEXT("Retired global getter reports disabled effective state"), Global.bEnabled);
    TestEqual(TEXT("Retired global getter reports zero strength"), Global.Strength, 0.f);
    TestEqual(TEXT("Retired global getter reports neutral color"), Global.Tint, FLinearColor::White);
    FYogShadowTransitionSettings Resolved;
    bool bOverridden = true;
    for (const int32 Index : {0, 1})
    {
        TestTrue(TEXT("Existing shared profile is still a valid compatibility reference"),
            UYogShadowTransitionLibrary::GetCharacterShadowTransition(Index, Resolved, bOverridden));
        TestFalse(TEXT("Legacy authored override is never reported as active"), bOverridden);
        TestFalse(TEXT("Legacy enabled character tint is effectively disabled"), Resolved.bEnabled);
        TestEqual(TEXT("Effective character color is neutral"), Resolved.Tint, FLinearColor::White);
        TestEqual(TEXT("Effective character strength is zero"), Resolved.Strength, 0.f);
        TestFalse(TEXT("Retired setter does not report a successful mutation"),
            UYogShadowTransitionLibrary::SetCharacterShadowTransitionOverride(Index, LegacyRequest));
        TestFalse(TEXT("Retired clear does not report a successful mutation"),
            UYogShadowTransitionLibrary::ClearCharacterShadowTransitionOverride(Index));
    }
    for (const int32 Index : {-1, 2, 7, 8, 100})
    {
        bOverridden = true;
        TestFalse(TEXT("Invalid or missing profile getter returns false"),
            UYogShadowTransitionLibrary::GetCharacterShadowTransition(Index, Resolved, bOverridden));
        TestFalse(TEXT("Invalid getter clears override output"), bOverridden);
        TestFalse(TEXT("Invalid getter returns disabled output"), Resolved.bEnabled);
        TestEqual(TEXT("Invalid getter returns neutral tint"), Resolved.Tint, FLinearColor::White);
        TestFalse(TEXT("Invalid setter stays inert"),
            UYogShadowTransitionLibrary::SetCharacterShadowTransitionOverride(Index, LegacyRequest));
        TestFalse(TEXT("Invalid clear stays inert"), UYogShadowTransitionLibrary::ClearCharacterShadowTransitionOverride(Index));
    }
    LegacyRequest.Wrap = std::numeric_limits<float>::quiet_NaN();
    LegacyRequest.Falloff = std::numeric_limits<float>::infinity();
    LegacyRequest.Tint = FLinearColor(-1.f, 5.f, std::numeric_limits<float>::infinity(), .2f);
    UYogShadowTransitionLibrary::SetGlobalShadowTransition(LegacyRequest);
    TestFalse(TEXT("Invalid legacy payload cannot reactivate tint"),
        UYogShadowTransitionLibrary::GetGlobalShadowTransition().bEnabled);
    TestEqual(TEXT("Legacy calls retain every stored setting, including compatibility fields"),
        ExportAllShadowTransitionSettings(Settings), AllSettingsBefore);
    TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> PackedAfter;
    uint32 CountAfter = 0;
    GetStylizedCharacterLightingProfiles(PackedAfter, CountAfter);
    TestEqual(TEXT("Retired APIs never upload a new renderer profile count"), CountAfter, CountBefore);
    for (int32 Index = 0; Index < PackedAfter.Num(); ++Index)
        TestEqual(TEXT("Retired APIs never rewrite renderer profile data"), PackedAfter[Index], PackedBefore[Index]);
    for (const auto& Pair : CVars)
    {
        TestEqual(TEXT("Retired APIs do not change CVar values"), Pair.Key->GetString(), Pair.Value.Value);
        TestEqual(TEXT("Retired APIs do not change CVar priorities"), Pair.Key->GetFlags(), Pair.Value.Flags);
    }
    for (const auto& Pair : ConfigBytes)
    {
        TArray<uint8> After;
        TestTrue(TEXT("Retired APIs never save real configuration"), FFileHelper::LoadFileToArray(After, *Pair.Key) && After == Pair.Value);
    }
    for (const auto& Pair : DirtyPackages)
        if (const UPackage* Package = Pair.Key.Get()) TestEqual(TEXT("Retired APIs never dirty loaded assets"), Package->IsDirty(), Pair.Value);
    return true;
}

#endif
