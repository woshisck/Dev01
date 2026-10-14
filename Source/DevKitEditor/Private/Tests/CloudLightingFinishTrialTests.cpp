#include "Tools/Workflow/DevKitCloudLightingFinishTrial.h"
#include "Tools/Workflow/SDevKitRenderStyleWorkbenchWidget.h"

#include "Components/DirectionalLightComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StylizedCharacterLighting.h"
#include "StylizedLightingSettings.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    FString ExportProtectedSettings(const UStylizedLightingSettings* Settings)
    {
        FString Result;
        for (TFieldIterator<FProperty> It(Settings->GetClass()); It; ++It)
        {
            const FName Name = It->GetFName();
            if (Name == GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularIntensity)
                || Name == GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularOffset)
                || Name == GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterLightingProfiles))
            {
                continue;
            }
            FString Value;
            It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(Settings), nullptr, const_cast<UStylizedLightingSettings*>(Settings), PPF_None);
            Result += It->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        return Result;
    }

    FString ExportProtectedProfiles(const TArray<FStylizedCharacterLightingProfile>& Profiles)
    {
        FString Result;
        for (const FStylizedCharacterLightingProfile& Profile : Profiles)
        {
            for (TFieldIterator<FProperty> It(FStylizedCharacterLightingProfile::StaticStruct()); It; ++It)
            {
                const FName Name = It->GetFName();
                if (Name == GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularIntensity)
                    || Name == GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionIntensity)
                    || Name == GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectOcclusionStrength))
                {
                    continue;
                }
                FString Value;
                It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(&Profile), nullptr, nullptr, PPF_None);
                Result += It->GetName() + TEXT("=") + Value + TEXT("\n");
            }
            Result += TEXT("---\n");
        }
        return Result;
    }

    struct FScopedFinishState
    {
        struct FCVarState { FString Value; EConsoleVariableFlags Flags; };
        UStylizedLightingSettings* Settings = nullptr;
        float SceneSpecular = 0.f;
        float SceneOffset = 0.f;
        TArray<FStylizedCharacterLightingProfile> Profiles;
        TMap<IConsoleVariable*, FCVarState> CVars;
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
        uint32 PackedCount = 0;
        TMap<FString, TArray<uint8>> ConfigBytes;
        TMap<TWeakObjectPtr<UPackage>, bool> PackageDirty;
        TMap<FString, FCVarState> NativeContract;
        TMap<FBoolProperty*, bool> BoolSettings;

        FScopedFinishState()
        {
            FDevKitCloudLightingFinishTrial::Restore();
            Settings = GetMutableDefault<UStylizedLightingSettings>();
            SceneSpecular = Settings->SpecularIntensity;
            SceneOffset = Settings->SpecularOffset;
            Profiles = Settings->CharacterLightingProfiles;
            for (TFieldIterator<FBoolProperty> It(Settings->GetClass()); It; ++It)
                BoolSettings.Add(*It, It->GetPropertyValue_InContainer(Settings));
            GetStylizedCharacterLightingProfiles(Packed, PackedCount);
            for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
                TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.GlossInfluence"),
                TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
                TEXT("r.StylizedLumenLighting.IndirectMaxLuminance"), TEXT("r.StylizedLumenLighting.SpecularIntensity"),
                TEXT("r.StylizedLumenLighting.SpecularOffset"), TEXT("r.StylizedReflection.Kuwahara.Enable"),
                TEXT("r.StylizedReflection.Kuwahara.Strength"), TEXT("r.StylizedCharacter.SelfShadow.Enable"),
                TEXT("r.StylizedCharacter.Shadow.Split.Enable"), TEXT("r.StylizedCharacter.Shadow.Split.Self"),
                TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"),
                TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), TEXT("r.StylizedCharacter.SelfShadow.Strength"),
                TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), TEXT("r.StylizedCharacterLighting.DirectColorInfluence"),
                TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence")})
            {
                if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
                {
                    CVars.Add(Variable, {Variable->GetString(), Variable->GetFlags()});
                    // Permit deterministic settings uploads, then restore the caller's original priority.
                    Variable->ClearFlags(ECVF_SetByMask);
                }
            }
            for (const TCHAR* Name : {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Lumen.DiffuseIndirect.Allow"),
                TEXT("r.ReflectionMethod"), TEXT("r.Lumen.Reflections.Allow"), TEXT("r.Shadow.Virtual.Enable"),
                TEXT("r.ShadowQuality"), TEXT("r.Shadow.MaxResolution"), TEXT("r.Shadow.MaxCSMResolution"),
                TEXT("r.DefaultFeature.AutoExposure"), TEXT("r.EyeAdaptationQuality"),
                TEXT("r.FirstPerson.SelfShadow.RawFullResolution"), TEXT("r.FirstPerson.SelfShadow.MaxTraceDistance")})
            {
                if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
                    NativeContract.Add(Name, {Variable->GetString(), Variable->GetFlags()});
            }
            for (const FString& Path : {FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"),
                FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"), GGameIni, GEngineIni})
            {
                TArray<uint8> Bytes;
                if (FPaths::FileExists(Path) && FFileHelper::LoadFileToArray(Bytes, *Path)) ConfigBytes.Add(Path, MoveTemp(Bytes));
            }
            for (TObjectIterator<UPackage> It; It; ++It)
                if (!It->HasAnyFlags(RF_Transient)) PackageDirty.Add(TWeakObjectPtr<UPackage>(*It), It->IsDirty());
        }

        ~FScopedFinishState()
        {
            FDevKitCloudLightingFinishTrial::Restore();
            Settings->SpecularIntensity = SceneSpecular;
            Settings->SpecularOffset = SceneOffset;
            Settings->CharacterLightingProfiles = Profiles;
            for (const auto& Pair : BoolSettings) Pair.Key->SetPropertyValue_InContainer(Settings, Pair.Value);
            Settings->ApplyToConsoleVariables();
            SetStylizedCharacterLightingProfiles(MakeArrayView(Packed.GetData(), Packed.Num()), PackedCount);
            for (const auto& Pair : CVars)
            {
                Pair.Key->ClearFlags(ECVF_SetByMask);
                Pair.Key->Set(*Pair.Value.Value, static_cast<EConsoleVariableFlags>(Pair.Value.Flags & ECVF_SetByMask));
                Pair.Key->SetFlags(Pair.Value.Flags);
            }
        }

        void CheckNoPersistenceOrNativeChanges(FAutomationTestBase& Test) const
        {
            for (const auto& Pair : ConfigBytes)
            {
                TArray<uint8> Current;
                Test.TestTrue(*FString::Printf(TEXT("Preview does not write config %s"), *Pair.Key),
                    FFileHelper::LoadFileToArray(Current, *Pair.Key) && Current == Pair.Value);
            }
            for (const auto& Pair : PackageDirty)
                if (const UPackage* Package = Pair.Key.Get())
                    Test.TestEqual(*FString::Printf(TEXT("Preview preserves package dirty state %s"), *Package->GetName()), Package->IsDirty(), Pair.Value);
            for (const auto& Pair : NativeContract)
            {
                if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Pair.Key))
                {
                    Test.TestEqual(*FString::Printf(TEXT("Native rendering value remains unchanged %s"), *Pair.Key), Variable->GetString(), Pair.Value.Value);
                    Test.TestEqual(*FString::Printf(TEXT("Native rendering priority remains unchanged %s"), *Pair.Key), Variable->GetFlags(), Pair.Value.Flags);
                }
            }
        }
    };

    struct FScopedFinishWorld
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
        ~FScopedFinishWorld()
        {
            FDevKitCloudLightingFinishTrial::Restore();
            if (World) World->DestroyWorld(false);
        }
    };

    void SeedProfiles(UStylizedLightingSettings* Settings)
    {
        Settings->SpecularIntensity = 2.5f;
        Settings->SpecularOffset = -.02f;
        Settings->CharacterLightingProfiles.SetNum(2);
        FStylizedCharacterLightingProfile& First = Settings->CharacterLightingProfiles[0];
        First.ProfileName = TEXT("FinishTestDefault");
        First.SpecularIntensity = 2.2f;
        First.ReflectionIntensity = 1.3f;
        First.IndirectOcclusionStrength = .35f;
        First.CharacterExposure = .42f;
        First.CharacterContrast = 1.2f;
        First.CharacterBaseFill = .16f;
        FStylizedCharacterLightingProfile& Second = Settings->CharacterLightingProfiles[1];
        Second.ProfileName = TEXT("FinishTestMetal");
        Second.SpecularIntensity = 7.5f;
        Second.ReflectionIntensity = 4.2f;
        Second.IndirectOcclusionStrength = .8f;
        Second.DirectDiffuseIntensity = 1.4f;
        Second.IndirectLightingIntensity = .7f;
        Second.SpecularTint = FLinearColor(.9f, .8f, .7f, 1.f);
    }

    void TestFinishParameters(FAutomationTestBase& Test, const FDevKitCloudLightingFinishParameters& Actual,
        const FDevKitCloudLightingFinishParameters& Expected)
    {
        Test.TestEqual(TEXT("Session scene specular multiplier"), Actual.SceneSpecularScale, Expected.SceneSpecularScale);
        Test.TestEqual(TEXT("Session scene specular offset delta"), Actual.SceneSpecularOffsetDelta, Expected.SceneSpecularOffsetDelta);
        Test.TestEqual(TEXT("Session character specular multiplier"), Actual.CharacterSpecularScale, Expected.CharacterSpecularScale);
        Test.TestEqual(TEXT("Session character reflection multiplier"), Actual.CharacterReflectionScale, Expected.CharacterReflectionScale);
        Test.TestEqual(TEXT("Session character occlusion multiplier"), Actual.CharacterOcclusionScale, Expected.CharacterOcclusionScale);
        Test.TestEqual(TEXT("Session projected shadow multiplier"), Actual.CharacterProjectedShadowScale, Expected.CharacterProjectedShadowScale);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitCloudFinishReversibleProfilesTest,
    "DevKit.Workflow.CloudLightingFinish.ReversibleProfiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitCloudFinishReversibleProfilesTest::RunTest(const FString&)
{
    FScopedFinishState Cleanup;
    UStylizedLightingSettings* Settings = Cleanup.Settings;
    SeedProfiles(Settings);
    const FString ProtectedSettings = ExportProtectedSettings(Settings);
    const FString ProtectedProfiles = ExportProtectedProfiles(Settings->CharacterLightingProfiles);
    const TArray<FStylizedCharacterLightingProfile> OriginalProfiles = Settings->CharacterLightingProfiles;
    const FDevKitCloudLightingFinishParameters Parameters;
    FDevKitCloudLightingFinishTrial::Apply(nullptr, Parameters);
    TestTrue(TEXT("Applying starts a reversible session"), FDevKitCloudLightingFinishTrial::IsActive());
    TestTrue(TEXT("Scene highlight scales the user's current value, not a hard-coded one"), FMath::IsNearlyEqual(Settings->SpecularIntensity, 2.5f * Parameters.SceneSpecularScale));
    TestTrue(TEXT("Scene offset is relative to the user's current offset"), FMath::IsNearlyEqual(Settings->SpecularOffset, -.02f + Parameters.SceneSpecularOffsetDelta));
    for (int32 Index = 0; Index < OriginalProfiles.Num(); ++Index)
    {
        TestTrue(TEXT("Each authored specular ratio is preserved"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[Index].SpecularIntensity, OriginalProfiles[Index].SpecularIntensity * Parameters.CharacterSpecularScale));
        TestTrue(TEXT("Each authored reflection ratio is preserved"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[Index].ReflectionIntensity, OriginalProfiles[Index].ReflectionIntensity * Parameters.CharacterReflectionScale));
        TestTrue(TEXT("Each authored cavity ratio is preserved"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[Index].IndirectOcclusionStrength, OriginalProfiles[Index].IndirectOcclusionStrength * Parameters.CharacterOcclusionScale));
    }
    TestTrue(TEXT("Unusually strong authored character specular is not flattened to four"), Settings->CharacterLightingProfiles[1].SpecularIntensity > 4.f);
    const float TrialScene = Settings->SpecularIntensity;
    const float TrialOffset = Settings->SpecularOffset;
    const TArray<FStylizedCharacterLightingProfile> FirstTrial = Settings->CharacterLightingProfiles;
    FDevKitCloudLightingFinishTrial::Apply(nullptr, Parameters);
    TestEqual(TEXT("Repeat apply is non-cumulative for scene highlights"), Settings->SpecularIntensity, TrialScene);
    TestEqual(TEXT("Repeat apply is non-cumulative for scene offsets"), Settings->SpecularOffset, TrialOffset);
    for (int32 Index = 0; Index < FirstTrial.Num(); ++Index)
    {
        TestEqual(TEXT("Repeat apply is non-cumulative for character specular"), Settings->CharacterLightingProfiles[Index].SpecularIntensity, FirstTrial[Index].SpecularIntensity);
        TestEqual(TEXT("Repeat apply is non-cumulative for character reflections"), Settings->CharacterLightingProfiles[Index].ReflectionIntensity, FirstTrial[Index].ReflectionIntensity);
        TestEqual(TEXT("Repeat apply is non-cumulative for character occlusion"), Settings->CharacterLightingProfiles[Index].IndirectOcclusionStrength, FirstTrial[Index].IndirectOcclusionStrength);
    }
    FDevKitCloudLightingFinishParameters Revised = Parameters;
    Revised.SceneSpecularScale = .8f;
    Revised.CharacterSpecularScale = .6f;
    Revised.CharacterReflectionScale = .4f;
    Revised.CharacterOcclusionScale = .5f;
    FDevKitCloudLightingFinishTrial::Apply(nullptr, Revised);
    TestEqual(TEXT("Realtime re-edit derives scene intensity from first snapshot"), Settings->SpecularIntensity, 2.f);
    TestTrue(TEXT("Realtime re-edit derives profile strength from first snapshot"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[1].SpecularIntensity, 4.5f));
    TestEqual(TEXT("Bands, softness, gloss, direct/indirect mix, feature switches and self-shadow settings are untouched"), ExportProtectedSettings(Settings), ProtectedSettings);
    TestEqual(TEXT("Profile exposure, contrast, tint, GI and base fill remain untouched"), ExportProtectedProfiles(Settings->CharacterLightingProfiles), ProtectedProfiles);
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    FDevKitCloudLightingFinishTrial::Restore();
    TestFalse(TEXT("Restore releases the shared trial"), FDevKitCloudLightingFinishTrial::IsActive());
    TestEqual(TEXT("Restore recovers exact custom scene intensity"), Settings->SpecularIntensity, 2.5f);
    TestEqual(TEXT("Restore recovers exact custom scene offset"), Settings->SpecularOffset, -.02f);
    for (int32 Index = 0; Index < OriginalProfiles.Num(); ++Index)
    {
        TestEqual(TEXT("Restore recovers exact authored specular"), Settings->CharacterLightingProfiles[Index].SpecularIntensity, OriginalProfiles[Index].SpecularIntensity);
        TestEqual(TEXT("Restore recovers exact authored reflection"), Settings->CharacterLightingProfiles[Index].ReflectionIntensity, OriginalProfiles[Index].ReflectionIntensity);
        TestEqual(TEXT("Restore recovers exact authored occlusion"), Settings->CharacterLightingProfiles[Index].IndirectOcclusionStrength, OriginalProfiles[Index].IndirectOcclusionStrength);
    }
    FDevKitCloudLightingFinishTrial::Restore();
    TestFalse(TEXT("Restore without a snapshot is safe and idempotent"), FDevKitCloudLightingFinishTrial::IsActive());
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitCloudFinishShadowScopeTest,
    "DevKit.Workflow.CloudLightingFinish.ShadowScope", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitCloudFinishShadowScopeTest::RunTest(const FString&)
{
    FScopedFinishState Cleanup;
    FScopedFinishWorld TestWorld;
    if (!TestNotNull(TEXT("Transient editor world"), TestWorld.World)) return false;
    ADirectionalLight* Key = TestWorld.World->SpawnActor<ADirectionalLight>();
    ADirectionalLight* NonCasting = TestWorld.World->SpawnActor<ADirectionalLight>();
    APointLight* Point = TestWorld.World->SpawnActor<APointLight>();
    AActor* NestedOwner = TestWorld.World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Key light"), Key) || !TestNotNull(TEXT("Non-casting light"), NonCasting)
        || !TestNotNull(TEXT("Point light"), Point) || !TestNotNull(TEXT("Nested light owner"), NestedOwner)) return false;
    UDirectionalLightComponent* KeyComponent = CastChecked<UDirectionalLightComponent>(Key->GetLightComponent());
    UDirectionalLightComponent* SkipComponent = CastChecked<UDirectionalLightComponent>(NonCasting->GetLightComponent());
    ULightComponent* PointComponent = Point->GetLightComponent();
    UDirectionalLightComponent* NestedComponent = NewObject<UDirectionalLightComponent>(NestedOwner);
    NestedOwner->AddInstanceComponent(NestedComponent);
    NestedComponent->RegisterComponent();
    KeyComponent->SetCastShadows(true);
    KeyComponent->StylizedCharacterShadowStrength = .8f;
    KeyComponent->ShadowSharpen = .2f;
    NestedComponent->SetCastShadows(true);
    NestedComponent->StylizedCharacterShadowStrength = .6f;
    NestedComponent->ShadowSharpen = .05f;
    SkipComponent->SetCastShadows(false);
    SkipComponent->StylizedCharacterShadowStrength = .7f;
    SkipComponent->ShadowSharpen = .3f;
    PointComponent->SetCastShadows(true);
    PointComponent->StylizedCharacterShadowStrength = .55f;
    PointComponent->ShadowSharpen = .4f;
    const float KeyBias = KeyComponent->ShadowBias;
    const float KeySlope = KeyComponent->ShadowSlopeBias;
    const float KeyResolution = KeyComponent->ShadowResolutionScale;
    const float KeyIntensity = KeyComponent->Intensity;
    const EStylizedCharacterLightMode KeyMode = KeyComponent->StylizedCharacterLightMode;
    const bool bWorldDirtyBefore = TestWorld.World->GetOutermost()->IsDirty();
    FDevKitCloudLightingFinishParameters Parameters;
    Parameters.CharacterProjectedShadowScale = .9f;
    FDevKitCloudLightingFinishTrial::Apply(TestWorld.World, Parameters);
    TestEqual(TEXT("Key projected shadow receives a relative reduction"), KeyComponent->StylizedCharacterShadowStrength, .8f * Parameters.CharacterProjectedShadowScale);
    const float KeyTrialShadow = KeyComponent->StylizedCharacterShadowStrength;
    TestEqual(TEXT("VSM preview leaves legacy SM sharpening untouched"), KeyComponent->ShadowSharpen, .2f);
    TestTrue(TEXT("Directional components nested in actor/BP owners are included"), FMath::IsNearlyEqual(NestedComponent->StylizedCharacterShadowStrength, .54f));
    TestEqual(TEXT("Nested directional SM filtering is untouched"), NestedComponent->ShadowSharpen, .05f);
    TestEqual(TEXT("Non-casting directional shadow strength is skipped"), SkipComponent->StylizedCharacterShadowStrength, .7f);
    TestEqual(TEXT("Non-casting directional filtering is skipped"), SkipComponent->ShadowSharpen, .3f);
    TestEqual(TEXT("Point-light projected shadow strength is untouched"), PointComponent->StylizedCharacterShadowStrength, .55f);
    TestEqual(TEXT("Point-light filtering is untouched"), PointComponent->ShadowSharpen, .4f);
    FDevKitCloudLightingFinishTrial::Apply(TestWorld.World, Parameters);
    TestEqual(TEXT("Repeated light edit does not compound attenuation"), KeyComponent->StylizedCharacterShadowStrength, KeyTrialShadow);
    TestEqual(TEXT("Repeated preview never changes SM sharpening"), KeyComponent->ShadowSharpen, .2f);
    TestTrue(TEXT("Native shadow casting stays enabled"), !!KeyComponent->CastShadows);
    TestEqual(TEXT("Shadow bias stays authored"), KeyComponent->ShadowBias, KeyBias);
    TestEqual(TEXT("Shadow slope bias stays authored"), KeyComponent->ShadowSlopeBias, KeySlope);
    TestEqual(TEXT("Shadow resolution stays authored"), KeyComponent->ShadowResolutionScale, KeyResolution);
    TestEqual(TEXT("Light intensity stays authored"), KeyComponent->Intensity, KeyIntensity);
    TestEqual(TEXT("Toon/Wash light mode stays authored"), KeyComponent->StylizedCharacterLightMode, KeyMode);
    TestEqual(TEXT("Preview does not dirty the world package"), TestWorld.World->GetOutermost()->IsDirty(), bWorldDirtyBefore);
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    NestedComponent->DestroyComponent();
    FDevKitCloudLightingFinishTrial::Restore();
    TestFalse(TEXT("Restoration tolerates a destroyed nested component"), FDevKitCloudLightingFinishTrial::IsActive());
    TestEqual(TEXT("Key projected shadow restores exactly"), KeyComponent->StylizedCharacterShadowStrength, .8f);
    TestEqual(TEXT("Key shadow filtering restores exactly"), KeyComponent->ShadowSharpen, .2f);
    TestTrue(TEXT("Restore never disables native scene/character casting"), !!KeyComponent->CastShadows);
    TestEqual(TEXT("Restoration does not dirty the world package"), TestWorld.World->GetOutermost()->IsDirty(), bWorldDirtyBefore);
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitCloudFinishFiniteInputsTest,
    "DevKit.Workflow.CloudLightingFinish.FiniteInputs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitCloudFinishFiniteInputsTest::RunTest(const FString&)
{
    FScopedFinishState Cleanup;
    FScopedFinishWorld TestWorld;
    if (!TestNotNull(TEXT("Transient world for finite shadow controls"), TestWorld.World)) return false;
    ADirectionalLight* Key = TestWorld.World->SpawnActor<ADirectionalLight>();
    if (!TestNotNull(TEXT("Directional light for finite shadow controls"), Key)) return false;
    UDirectionalLightComponent* KeyComponent = CastChecked<UDirectionalLightComponent>(Key->GetLightComponent());
    KeyComponent->SetCastShadows(true);
    KeyComponent->StylizedCharacterShadowStrength = .7f;
    KeyComponent->ShadowSharpen = .2f;
    UStylizedLightingSettings* Settings = Cleanup.Settings;
    SeedProfiles(Settings);
    const FDevKitCloudLightingFinishParameters Defaults;
    FDevKitCloudLightingFinishParameters Invalid;
    Invalid.SceneSpecularScale = std::numeric_limits<float>::quiet_NaN();
    Invalid.SceneSpecularOffsetDelta = std::numeric_limits<float>::infinity();
    Invalid.CharacterSpecularScale = -std::numeric_limits<float>::infinity();
    Invalid.CharacterReflectionScale = std::numeric_limits<float>::quiet_NaN();
    Invalid.CharacterOcclusionScale = std::numeric_limits<float>::infinity();
    Invalid.CharacterProjectedShadowScale = std::numeric_limits<float>::quiet_NaN();
    FDevKitCloudLightingFinishTrial::Apply(TestWorld.World, Invalid);
    TestFinishParameters(*this, FDevKitCloudLightingFinishTrial::GetParameters(), Defaults);
    TestTrue(TEXT("NaN scene scale falls back to the conservative preset"), FMath::IsFinite(Settings->SpecularIntensity) && FMath::IsNearlyEqual(Settings->SpecularIntensity, 2.5f * Defaults.SceneSpecularScale));
    TestTrue(TEXT("Infinite scene offset falls back to the conservative preset"), FMath::IsFinite(Settings->SpecularOffset) && FMath::IsNearlyEqual(Settings->SpecularOffset, -.02f + Defaults.SceneSpecularOffsetDelta));
    TestTrue(TEXT("Infinite character specular falls back safely"), FMath::IsFinite(Settings->CharacterLightingProfiles[0].SpecularIntensity) && FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[0].SpecularIntensity, 2.2f * Defaults.CharacterSpecularScale));
    TestTrue(TEXT("NaN character reflection falls back safely"), FMath::IsFinite(Settings->CharacterLightingProfiles[0].ReflectionIntensity) && FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[0].ReflectionIntensity, 1.3f * Defaults.CharacterReflectionScale));
    TestTrue(TEXT("Infinite character occlusion falls back safely"), FMath::IsFinite(Settings->CharacterLightingProfiles[0].IndirectOcclusionStrength) && FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[0].IndirectOcclusionStrength, .35f * Defaults.CharacterOcclusionScale));
    TestTrue(TEXT("NaN projected shadow scale falls back safely"), FMath::IsFinite(KeyComponent->StylizedCharacterShadowStrength) && FMath::IsNearlyEqual(KeyComponent->StylizedCharacterShadowStrength, .7f * Defaults.CharacterProjectedShadowScale));
    FDevKitCloudLightingFinishParameters OutOfRange;
    OutOfRange.SceneSpecularScale = 99.f;
    OutOfRange.SceneSpecularOffsetDelta = 99.f;
    OutOfRange.CharacterSpecularScale = -99.f;
    OutOfRange.CharacterReflectionScale = 99.f;
    OutOfRange.CharacterOcclusionScale = -99.f;
    OutOfRange.CharacterProjectedShadowScale = -99.f;
    FDevKitCloudLightingFinishTrial::Apply(TestWorld.World, OutOfRange);
    FDevKitCloudLightingFinishParameters Bounded;
    Bounded.SceneSpecularScale = 2.f;
    Bounded.SceneSpecularOffsetDelta = 1.f;
    Bounded.CharacterSpecularScale = 0.f;
    Bounded.CharacterReflectionScale = 2.f;
    Bounded.CharacterOcclusionScale = 0.f;
    Bounded.CharacterProjectedShadowScale = 0.f;
    TestFinishParameters(*this, FDevKitCloudLightingFinishTrial::GetParameters(), Bounded);
    TestEqual(TEXT("Scene highlight result is bounded"), Settings->SpecularIntensity, 4.f);
    TestTrue(TEXT("Scene offset delta is bounded"), FMath::IsNearlyEqual(Settings->SpecularOffset, .98f));
    TestEqual(TEXT("Negative character specular is bounded at zero"), Settings->CharacterLightingProfiles[0].SpecularIntensity, 0.f);
    TestTrue(TEXT("Character reflection multiplier is bounded at two"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[0].ReflectionIntensity, 2.6f));
    TestEqual(TEXT("Negative cavity multiplier is bounded at zero"), Settings->CharacterLightingProfiles[0].IndirectOcclusionStrength, 0.f);
    TestEqual(TEXT("Negative projected multiplier is bounded at zero without changing cast flags"), KeyComponent->StylizedCharacterShadowStrength, 0.f);
    TestEqual(TEXT("Out-of-range preview never changes SM sharpening"), KeyComponent->ShadowSharpen, .2f);
    TestTrue(TEXT("Invalid and out-of-range inputs never disable native casting"), !!KeyComponent->CastShadows);
    FDevKitCloudLightingFinishTrial::Restore();
    TestEqual(TEXT("Sanitized previews still restore exact user values"), Settings->SpecularIntensity, 2.5f);
    TestEqual(TEXT("Sanitized previews still restore exact profile values"), Settings->CharacterLightingProfiles[0].SpecularIntensity, 2.2f);
    TestEqual(TEXT("Sanitized previews restore exact shadow strength"), KeyComponent->StylizedCharacterShadowStrength, .7f);
    TestEqual(TEXT("Sanitized previews restore exact filtering"), KeyComponent->ShadowSharpen, .2f);
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitCloudFinishWidgetTest,
    "DevKit.Workflow.CloudLightingFinish.LiveWidget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitCloudFinishWidgetTest::RunTest(const FString&)
{
    FScopedFinishState Cleanup;
    if (!TestNotNull(TEXT("Editor for live finishing controls"), GEditor)
        || !TestNotNull(TEXT("Editor world for live finishing controls"), GEditor->GetEditorWorldContext().World())) return false;
    UStylizedLightingSettings* Settings = Cleanup.Settings;
    SeedProfiles(Settings);
    const FString ProtectedSettings = ExportProtectedSettings(Settings);
    const FString ProtectedProfiles = ExportProtectedProfiles(Settings->CharacterLightingProfiles);
    TSharedPtr<SDevKitRenderStyleWorkbenchWidget> Widget = SNew(SDevKitRenderStyleWorkbenchWidget);
    TestFalse(TEXT("Opening the panel does not implicitly activate a trial"), FDevKitCloudLightingFinishTrial::IsActive());
    TestEqual(TEXT("Opening the panel does not change authored scene highlights"), Settings->SpecularIntensity, 2.5f);
    Widget->ApplyFinishTrial();
    TestTrue(TEXT("The real trial button activates the service"), FDevKitCloudLightingFinishTrial::IsActive());
    TestTrue(TEXT("The trial button retains the custom opening baseline"), FMath::IsNearlyEqual(Settings->SpecularIntensity, 2.25f));

    TArray<TSharedRef<SSpinBox<float>>> Controls;
    TFunction<void(TSharedRef<SWidget>)> CollectControls;
    CollectControls = [&Controls, &CollectControls](TSharedRef<SWidget> Node)
    {
        if (Node->GetType() == FName(TEXT("SSpinBox<float>"))) Controls.Add(StaticCastSharedRef<SSpinBox<float>>(Node));
        FChildren* Children = Node->GetChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index) CollectControls(Children->GetChildAt(Index));
    };
    CollectControls(Widget->MakeFinishControls());
    TestEqual(TEXT("The finish panel exposes six controls and no SM sharpening"), Controls.Num(), 6);
    if (Controls.Num() == 6)
    {
        const float Values[] = {.75f, -.01f, .7f, .8f, .65f, .75f};
        for (int32 Index = 0; Index < Controls.Num(); ++Index) Controls[Index]->SetValue(Values[Index], false);
        TestTrue(TEXT("Scene highlight callback updates rendering settings immediately"), FMath::IsNearlyEqual(Settings->SpecularIntensity, 1.875f));
        TestTrue(TEXT("Scene offset callback remains relative to the first snapshot"), FMath::IsNearlyEqual(Settings->SpecularOffset, -.03f));
        TestTrue(TEXT("Character specular callback updates every authored profile"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[0].SpecularIntensity, 1.54f)
            && FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[1].SpecularIntensity, 5.25f));
        TestTrue(TEXT("Character reflection callback updates settings immediately"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[0].ReflectionIntensity, 1.04f));
        TestEqual(TEXT("Character cavity callback updates settings immediately"), Settings->CharacterLightingProfiles[0].IndirectOcclusionStrength, .35f * Values[4]);
        TestEqual(TEXT("Projected shadow callback stores the requested ratio"), Widget->FinishParameters.CharacterProjectedShadowScale, Values[5]);
        Controls[0]->SetValue(.6f, false);
        TestEqual(TEXT("A second live scene edit uses the original 2.5 baseline"), Settings->SpecularIntensity, 1.5f);
        TestTrue(TEXT("Unrelated callback fields are not multiplied again"), FMath::IsNearlyEqual(Settings->CharacterLightingProfiles[1].SpecularIntensity, 5.25f));
        const FDevKitCloudLightingFinishParameters SessionParameters = Widget->FinishParameters;
        TestFinishParameters(*this, FDevKitCloudLightingFinishTrial::GetParameters(), SessionParameters);
        Controls.Reset();
        Widget.Reset();
        TestTrue(TEXT("Closing the panel does not silently cancel the shared session"), FDevKitCloudLightingFinishTrial::IsActive());
        Widget = SNew(SDevKitRenderStyleWorkbenchWidget);
        TestFinishParameters(*this, Widget->FinishParameters, SessionParameters);
        CollectControls(Widget->MakeFinishControls());
        TestEqual(TEXT("Reopened panel exposes all six session controls"), Controls.Num(), 6);
        const float ReopenedValues[] = {.6f, -.01f, .7f, .8f, .65f, .75f};
        for (int32 Index = 0; Index < Controls.Num() && Index < UE_ARRAY_COUNT(ReopenedValues); ++Index)
            TestEqual(*FString::Printf(TEXT("Reopened control %d retains the applied session value"), Index), Controls[Index]->GetValue(), ReopenedValues[Index]);
        TestEqual(TEXT("Reopening the panel does not apply the default trial again"), Settings->SpecularIntensity, 1.5f);
        FDevKitCloudLightingFinishTrial::Restore();
        Widget->Tick(FGeometry(), 0.0, 0.f);
        TestEqual(TEXT("External session restoration refreshes the scene highlight draft on Tick"), Widget->DraftSpecularIntensity, 2.5f);
        TestEqual(TEXT("External session restoration refreshes the scene offset draft on Tick"), Widget->DraftSpecularOffset, -.02f);
        Widget->ApplyFinishTrial();
        TestEqual(TEXT("Reopened controls can reapply the same trial from the recovered baseline"), Settings->SpecularIntensity, 1.5f);
    }
    TestEqual(TEXT("Live callbacks leave bands, GI, self shadows and tone switches unchanged"), ExportProtectedSettings(Settings), ProtectedSettings);
    TestEqual(TEXT("Live callbacks leave profile exposure, color and indirect lighting unchanged"), ExportProtectedProfiles(Settings->CharacterLightingProfiles), ProtectedProfiles);
    UYogRenderStyleSettings* RenderStyle = GetMutableDefault<UYogRenderStyleSettings>();
    const FArrayProperty* PresetsProperty = FindFProperty<FArrayProperty>(UYogRenderStyleSettings::StaticClass(), GET_MEMBER_NAME_CHECKED(UYogRenderStyleSettings, Presets));
    FString PresetsBefore;
    if (TestNotNull(TEXT("Existing preset table reflection"), PresetsProperty))
        PresetsProperty->ExportTextItem_Direct(PresetsBefore, &RenderStyle->Presets, nullptr, RenderStyle, PPF_None);
    Widget->SaveProjectDefaults();
    TestTrue(TEXT("Project-default save explicitly rejects active finishing trials"), Widget->Status.Contains(TEXT("不会保存")));
    // The active-trial guard must execute before accessing the preset-name control.
    Widget->PresetName.Reset();
    Widget->SavePreset();
    TestTrue(TEXT("Preset save explicitly rejects active finishing trials"), Widget->Status.Contains(TEXT("不能伪装")));
    if (PresetsProperty)
    {
        FString PresetsAfter;
        PresetsProperty->ExportTextItem_Direct(PresetsAfter, &RenderStyle->Presets, nullptr, RenderStyle, PPF_None);
        TestEqual(TEXT("Rejected save does not mutate the existing preset table"), PresetsAfter, PresetsBefore);
    }
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    Widget->RestoreFinishTrial();
    TestFalse(TEXT("The real restore button ends the trial"), FDevKitCloudLightingFinishTrial::IsActive());
    TestEqual(TEXT("The restore button recovers exact custom scene intensity"), Settings->SpecularIntensity, 2.5f);
    TestEqual(TEXT("The restore button recovers exact custom scene offset"), Settings->SpecularOffset, -.02f);
    TestEqual(TEXT("The restore button recovers exact profile specular"), Settings->CharacterLightingProfiles[1].SpecularIntensity, 7.5f);
    TestEqual(TEXT("The restore button recovers exact profile reflection"), Settings->CharacterLightingProfiles[1].ReflectionIntensity, 4.2f);
    TestEqual(TEXT("The restore button recovers exact profile occlusion"), Settings->CharacterLightingProfiles[1].IndirectOcclusionStrength, .8f);
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitCloudFinishLifecycleTest,
    "DevKit.Workflow.CloudLightingFinish.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitCloudFinishLifecycleTest::RunTest(const FString&)
{
    FScopedFinishState Cleanup;
    FScopedFinishWorld OwnedWorld;
    FScopedFinishWorld OtherWorld;
    if (!TestNotNull(TEXT("Owned transient world"), OwnedWorld.World)
        || !TestNotNull(TEXT("Other transient world"), OtherWorld.World)) return false;
    ADirectionalLight* Key = OwnedWorld.World->SpawnActor<ADirectionalLight>();
    if (!TestNotNull(TEXT("Lifecycle directional light"), Key)) return false;
    UDirectionalLightComponent* Light = CastChecked<UDirectionalLightComponent>(Key->GetLightComponent());
    Light->SetCastShadows(true);
    Light->StylizedCharacterShadowStrength = .8f;
    Light->ShadowSharpen = .2f;
    UStylizedLightingSettings* Settings = Cleanup.Settings;
    SeedProfiles(Settings);
    const FString ProtectedSettings = ExportProtectedSettings(Settings);
    const FString ProtectedProfiles = ExportProtectedProfiles(Settings->CharacterLightingProfiles);
    const FDevKitCloudLightingFinishParameters Defaults;
    FDevKitCloudLightingFinishParameters Parameters;
    Parameters.SceneSpecularScale = .8f;
    Parameters.SceneSpecularOffsetDelta = -.01f;
    Parameters.CharacterSpecularScale = .7f;
    Parameters.CharacterReflectionScale = .8f;
    Parameters.CharacterOcclusionScale = .65f;
    Parameters.CharacterProjectedShadowScale = .75f;
    TestFinishParameters(*this, FDevKitCloudLightingFinishTrial::GetParameters(), Defaults);
    FDevKitCloudLightingFinishTrial::Apply(OwnedWorld.World, Defaults);
    FDevKitCloudLightingFinishTrial::Apply(OwnedWorld.World, Parameters);
    TestFinishParameters(*this, FDevKitCloudLightingFinishTrial::GetParameters(), Parameters);
    const float TrialShadow = .8f * Parameters.CharacterProjectedShadowScale;
    TestEqual(TEXT("Updated session parameters remain relative to the initial light snapshot"), Light->StylizedCharacterShadowStrength, TrialShadow);

    FObjectSaveContextData SaveData;
    SaveData.ObjectSaveContextPhase = EObjectSaveContextPhase::PreSave;
    SaveData.Object = OtherWorld.World;
    // Exercise only the notification contract. No SavePackage, filename, or asset write occurs.
    FEditorDelegates::PreSaveWorldWithContext.Broadcast(OtherWorld.World, FObjectPreSaveContext(SaveData));
    FEditorDelegates::PreSaveExternalActors.Broadcast(OtherWorld.World);
    FWorldDelegates::OnWorldCleanup.Broadcast(OtherWorld.World, false, false);
    TestTrue(TEXT("Unrelated world save/cleanup does not cancel the owned session"), FDevKitCloudLightingFinishTrial::IsActive());
    TestEqual(TEXT("Unrelated world events do not change the owned trial"), Light->StylizedCharacterShadowStrength, TrialShadow);

    const auto CheckRestored = [this, Settings, Light, &Defaults]()
    {
        TestFalse(TEXT("Owned lifecycle notification ends the session"), FDevKitCloudLightingFinishTrial::IsActive());
        TestEqual(TEXT("Lifecycle restoration recovers scene highlights exactly"), Settings->SpecularIntensity, 2.5f);
        TestEqual(TEXT("Lifecycle restoration recovers scene offset exactly"), Settings->SpecularOffset, -.02f);
        TestEqual(TEXT("Lifecycle restoration recovers authored character specular exactly"), Settings->CharacterLightingProfiles[1].SpecularIntensity, 7.5f);
        TestEqual(TEXT("Lifecycle restoration recovers authored character reflection exactly"), Settings->CharacterLightingProfiles[1].ReflectionIntensity, 4.2f);
        TestEqual(TEXT("Lifecycle restoration recovers authored character occlusion exactly"), Settings->CharacterLightingProfiles[1].IndirectOcclusionStrength, .8f);
        TestEqual(TEXT("Lifecycle restoration recovers projected shadows exactly"), Light->StylizedCharacterShadowStrength, .8f);
        TestEqual(TEXT("Lifecycle restoration recovers shadow filtering exactly"), Light->ShadowSharpen, .2f);
        TestTrue(TEXT("Lifecycle restoration retains native shadow casting"), !!Light->CastShadows);
        TestFinishParameters(*this, FDevKitCloudLightingFinishTrial::GetParameters(), Defaults);
    };
    SaveData.Object = OwnedWorld.World;
    FEditorDelegates::PreSaveWorldWithContext.Broadcast(OwnedWorld.World, FObjectPreSaveContext(SaveData));
    CheckRestored();
    FDevKitCloudLightingFinishTrial::Apply(OwnedWorld.World, Parameters);
    FEditorDelegates::PreSaveExternalActors.Broadcast(OwnedWorld.World);
    CheckRestored();
    FDevKitCloudLightingFinishTrial::Apply(OwnedWorld.World, Parameters);
    FWorldDelegates::OnWorldCleanup.Broadcast(OwnedWorld.World, false, false);
    CheckRestored();

    FDevKitCloudLightingFinishTrial::Apply(OtherWorld.World, Parameters);
    FEditorDelegates::PreSaveWorldWithContext.Broadcast(OwnedWorld.World, FObjectPreSaveContext(SaveData));
    FEditorDelegates::PreSaveExternalActors.Broadcast(OwnedWorld.World);
    FWorldDelegates::OnWorldCleanup.Broadcast(OwnedWorld.World, false, false);
    TestTrue(TEXT("Earlier owned-world callbacks do not cancel a later world session"), FDevKitCloudLightingFinishTrial::IsActive());
    TestEqual(TEXT("A later world session keeps its own relative highlight adjustment"), Settings->SpecularIntensity, 2.f);
    FDevKitCloudLightingFinishTrial::Restore();
    CheckRestored();
    TestEqual(TEXT("Lifecycle handling leaves all unowned settings unchanged"), ExportProtectedSettings(Settings), ProtectedSettings);
    TestEqual(TEXT("Lifecycle handling leaves all unowned profile settings unchanged"), ExportProtectedProfiles(Settings->CharacterLightingProfiles), ProtectedProfiles);
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitCharacterShadowWorkbenchTest,
    "DevKit.Workflow.CloudLightingFinish.CharacterShadowEnvironmentControls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitCharacterShadowWorkbenchTest::RunTest(const FString&)
{
    FScopedFinishState Cleanup;
    auto* Settings = Cleanup.Settings;
    SeedProfiles(Settings);
    Settings->bEnableSceneShadowPartition = true;
    Settings->bEnableCharacterShadowSeparation = false;
    Settings->bEnableCharacterIndependentSelfShadow = true;
    Settings->bEnableCharacterHalfVectorShadow = true;
    Settings->bEnableLumenDiffuseIndirect = true;
    Settings->bEnableEnvironmentReflections = true;
    Settings->bEnableIndirectOcclusion = false;
    Settings->ApplyToConsoleVariables();
    const auto* ReflectionCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"));
    const FString NativeReflection = ReflectionCVar ? ReflectionCVar->GetString() : FString();
    const TSharedRef<SDevKitRenderStyleWorkbenchWidget> Widget = SNew(SDevKitRenderStyleWorkbenchWidget);
    TArray<TSharedRef<SCheckBox>> Switches;
    TArray<TSharedRef<SSpinBox<float>>> MetalSpins;
    int32 NumericControls = 0;
    FString Labels;
    TFunction<void(TSharedRef<SWidget>)> Collect;
    Collect = [&Switches, &MetalSpins, &NumericControls, &Labels, &Collect](TSharedRef<SWidget> Node)
    {
        if (Node->GetType() == FName(TEXT("SSpinBox<float>")))
        {
            ++NumericControls;
            MetalSpins.Add(StaticCastSharedRef<SSpinBox<float>>(Node));
        }
        if (Node->GetType() == FName(TEXT("SCheckBox"))) Switches.Add(StaticCastSharedRef<SCheckBox>(Node));
        if (Node->GetType() == FName(TEXT("STextBlock"))) Labels += StaticCastSharedRef<STextBlock>(Node)->GetText().ToString() + TEXT("\n");
        FChildren* Children = Node->GetChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index) Collect(Children->GetChildAt(Index));
    };
    Collect(Widget->MakeCharacterShadowControls());
    if (!TestEqual(TEXT("Character group exposes four live switches and one retained inactive self-shadow state"), Switches.Num(), 5)) return false;
    TestEqual(TEXT("Reflection bool is replaced by two artist alpha sliders"), NumericControls, 2);
    TestFalse(TEXT("Experimental split toggle is removed"), Labels.Contains(TEXT("角色阴影拆分（")));
    TestFalse(TEXT("Half-vector toggle is removed"), Labels.Contains(TEXT("半程向量自阴影（")));
    TestTrue(TEXT("Environment reflection purpose is explained"), Labels.Contains(TEXT("金属所需的环境形状与反射颜色")));
    TestTrue(TEXT("Default-profile metal control describes the mode blend"), Labels.Contains(TEXT("金属风格混合（默认 Profile 0）")));
    const bool NPRBefore = Settings->CharacterLightingProfiles[0].bEnableNPRMetal;
    const bool OtherNPRBefore = Settings->CharacterLightingProfiles[1].bEnableNPRMetal;
    Switches[4]->ToggleCheckedState();
    TestEqual(TEXT("NPR checkbox changes the default profile immediately"), Settings->CharacterLightingProfiles[0].bEnableNPRMetal, !NPRBefore);
    TestEqual(TEXT("NPR checkbox does not edit another material profile"), Settings->CharacterLightingProfiles[1].bEnableNPRMetal, OtherNPRBefore);
    TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> NPRPacked;
    uint32 NPRCount = 0;
    GetStylizedCharacterLightingProfiles(NPRPacked, NPRCount);
    TestEqual(TEXT("NPR checkbox reaches the GPU profile enable lane"), NPRPacked[11].W, NPRBefore ? 0.f : 1.f);
    Switches[4]->ToggleCheckedState();
    const FString FirstSavedProfile = TEXT("(ProfileName=\"Default\",SpecularIntensity=1.700000,FutureData=(Value=17),bEnableNPRMetal=True)");
    const FString OtherSavedProfile = TEXT("(ProfileName=\"Other\",bEnableNPRMetal=True,ReflectionIntensity=0.370000)");
    TArray<FString> SavedProfiles = {FirstSavedProfile, OtherSavedProfile};
    TestTrue(TEXT("NPR saved-field update accepts nested compatibility data"), Widget->SetSavedDefaultNPRMetal(SavedProfiles, false));
    TestEqual(TEXT("NPR save preserves all unrelated first-profile bytes"), SavedProfiles[0], FirstSavedProfile.Replace(TEXT("bEnableNPRMetal=True"), TEXT("bEnableNPRMetal=False")));
    TestEqual(TEXT("NPR save preserves other profiles verbatim"), SavedProfiles[1], OtherSavedProfile);
    SavedProfiles[0] = TEXT("(ProfileName=\"Default\",SpecularTint=(R=0.25,G=0.5,B=1,A=1))");
    const FString LegacySavedProfile = SavedProfiles[0];
    TestTrue(TEXT("NPR save adds one field to a legacy profile"), Widget->SetSavedDefaultNPRMetal(SavedProfiles, true));
    TestEqual(TEXT("NPR migration preserves existing legacy values"), SavedProfiles[0], LegacySavedProfile.LeftChop(1) + TEXT(",bEnableNPRMetal=True)"));
    const FString QuotedProfile = TEXT("(ProfileName=\"Slash\\\\\",FutureData=(Name=\"quoted \\\" ),bEnableNPRMetal=False(\"),bEnableNPRMetal=True)");
    SavedProfiles = {QuotedProfile, OtherSavedProfile, OtherSavedProfile};
    TestTrue(TEXT("NPR parser handles both odd and even quote-escape parity"), Widget->SetSavedDefaultNPRMetal(SavedProfiles, false));
    TestEqual(TEXT("NPR parser does not mistake quoted delimiters for fields"), SavedProfiles[0], QuotedProfile.LeftChop(5) + TEXT("False)"));
    TestEqual(TEXT("Duplicate profile rows preserve their material-facing indices"), SavedProfiles.Num(), 3);
    TestEqual(TEXT("First duplicate profile remains untouched"), SavedProfiles[1], OtherSavedProfile);
    TestEqual(TEXT("Second duplicate profile remains untouched"), SavedProfiles[2], OtherSavedProfile);
    for (const FString InvalidProfile : {
        FString(TEXT("(bEnableNPRMetal=True,FutureData=(Value=1)")),
        FString(TEXT("(bEnableNPRMetal=True))")),
        FString(TEXT("(bEnableNPRMetal=True,ProfileName=\"Unclosed)")),
        FString(TEXT("(bEnableNPRMetal=True)(FutureData=1)")),
        FString(TEXT("(bEnableNPRMetal=True,bEnableNPRMetal=False)"))})
    {
        SavedProfiles = {InvalidProfile};
        TestFalse(TEXT("Malformed or ambiguous profile is rejected before editing"), Widget->SetSavedDefaultNPRMetal(SavedProfiles, false));
        TestEqual(TEXT("Rejected profile remains byte-identical"), SavedProfiles[0], InvalidProfile);
    }
    SavedProfiles = {TEXT("()")};
    TestTrue(TEXT("Empty serialized profile accepts the explicit flag"), Widget->SetSavedDefaultNPRMetal(SavedProfiles, true));
    TestEqual(TEXT("Empty profile does not gain a leading comma"), SavedProfiles[0], FString(TEXT("(bEnableNPRMetal=True)")));
    Switches[1]->SlatePrepass();
    TestFalse(TEXT("Independent self-shadow is inactive while split is fixed off"), Switches[1]->IsEnabled());
    TestEqual(TEXT("Independent self-shadow retains screenshot checked state"), Switches[1]->GetCheckedState(), ECheckBoxState::Checked);
    Switches[1]->ToggleCheckedState();
    TestTrue(TEXT("Inactive self-shadow control also rejects programmatic changes"), Settings->bEnableCharacterIndependentSelfShadow);
    const bool Original[] = {Settings->bEnableSceneShadowPartition, Settings->bEnableLumenDiffuseIndirect,
        Settings->bEnableEnvironmentReflections, Settings->bEnableIndirectOcclusion};
    const bool SelfBefore = Settings->bEnableCharacterIndependentSelfShadow;
    const auto CheckSplitCVars = [this, Settings]()
    {
        for (const auto& NameAndEnabled : {
            TPair<const TCHAR*, bool>(TEXT("r.StylizedCharacter.Shadow.Split.Enable"), Settings->bEnableCharacterShadowSeparation),
            TPair<const TCHAR*, bool>(TEXT("r.StylizedCharacter.Shadow.Split.Self"), Settings->bEnableCharacterIndependentSelfShadow),
            TPair<const TCHAR*, bool>(TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"), Settings->bEnableCharacterHalfVectorShadow)})
        {
            const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(NameAndEnabled.Key);
            if (TestNotNull(FString::Printf(TEXT("Shadow split CVar is registered: %s"), NameAndEnabled.Key), Variable))
                TestEqual(FString::Printf(TEXT("Shadow split switch uploads its actual CVar: %s"), NameAndEnabled.Key),
                    Variable->GetInt(), NameAndEnabled.Value ? 1 : 0);
        }
    };
    for (int32 Index : {0, 2, 3}) Switches[Index]->ToggleCheckedState();
    TestEqual(TEXT("Native reception switch is realtime"), Settings->bEnableSceneShadowPartition, !Original[0]);
    TestEqual(TEXT("GI switch is realtime"), Settings->bEnableLumenDiffuseIndirect, !Original[1]);
    TestEqual(TEXT("Reflection remains enabled without a checkbox"), Settings->bEnableEnvironmentReflections, Original[2]);
    TestEqual(TEXT("Optional occlusion switch is realtime"), Settings->bEnableIndirectOcclusion, !Original[3]);
    TestFalse(TEXT("Independent receiver split remains off"), Settings->bEnableCharacterShadowSeparation);
    TestEqual(TEXT("Inactive independent self-shadow keeps authored state"), Settings->bEnableCharacterIndependentSelfShadow, SelfBefore);
    TestTrue(TEXT("Hidden half-vector setting remains on but inert"), Settings->bEnableCharacterHalfVectorShadow);
    CheckSplitCVars();
    for (int32 Index : {0, 2, 3}) Switches[Index]->ToggleCheckedState();
    Settings->bEnableCharacterShadowSeparation = true;
    Settings->bEnableCharacterHalfVectorShadow = false;
    Widget->ApplyCharacterShadowChanges();
    TestFalse(TEXT("Legacy split enable cannot reactivate the hidden experiment"), Settings->bEnableCharacterShadowSeparation);
    TestTrue(TEXT("Legacy half-vector value is restored to screenshot compatibility state"), Settings->bEnableCharacterHalfVectorShadow);
    CheckSplitCVars();
    TestEqual(TEXT("Switching does not change material-profile GI intensity"), Settings->CharacterLightingProfiles[1].IndirectLightingIntensity, .7f);
    TestEqual(TEXT("Native reflection method is not changed"), ReflectionCVar ? ReflectionCVar->GetString() : FString(), NativeReflection);
    TestFalse(TEXT("Character controls do not start the temporary finishing trial"), FDevKitCloudLightingFinishTrial::IsActive());
    Labels.Reset();
    Switches.Reset();
    NumericControls = 0;
    MetalSpins.Reset();
    Collect(Widget->MakeMetalArtControls());
    TestTrue(TEXT("Default metal width explicitly follows roughness"), Labels.Contains(TEXT("默认高光宽度倍率 · 随粗糙度")));
    TestFalse(TEXT("Artificial environment-band group is removed"), Labels.Contains(TEXT("艺术环境亮带")));
    TestFalse(TEXT("Artificial environment-band strength is removed"), Labels.Contains(TEXT("环境亮带强度")));
    TestFalse(TEXT("Artificial environment-band direction is removed"), Labels.Contains(TEXT("亮带世界方向")));
    TestTrue(TEXT("Native light-driven environment reflection is explained"), Labels.Contains(TEXT("环境反射仅使用真实光照")));
    TestTrue(TEXT("Both requested metal modes are visible"), Labels.Contains(TEXT("PBR 响应")) && Labels.Contains(TEXT("风格化块面")));
    TestTrue(TEXT("Realtime blend exposes explicit endpoints"), Labels.Contains(TEXT("风格化混合强度 · 0=PBR / 1=块面")));
    TestTrue(TEXT("Far-view base block size is an artist control"), Labels.Contains(TEXT("基础色块大小 · 远景块面尺度")));
    TestTrue(TEXT("Actual darkening replaces the misleading dark-face gain"), Labels.Contains(TEXT("暗部强化 · 0=不压暗")));
    if (TestEqual(TEXT("Metal panel has one profile override and two exclusive mode controls"), Switches.Num(), 3))
    {
        TGuardValue<FStylizedMetalArtSettings> RestoreMetalArt(Settings->MetalArt, Settings->MetalArt);
        Settings->MetalArt.HighlightMode = EStylizedMetalHighlightMode::Stylized;
        Settings->MetalArt.Stylization = .63f;
        Switches[1]->ToggleCheckedState();
        TestEqual(TEXT("PBR selection is realtime"), Settings->MetalArt.HighlightMode, EStylizedMetalHighlightMode::PBR);
        TestEqual(TEXT("Changing mode preserves the remembered blend"), Settings->MetalArt.Stylization, .63f);
        GetStylizedCharacterLightingProfiles(NPRPacked, NPRCount);
        TestEqual(TEXT("PBR selection uploads its mode immediately"), NPRPacked[1].Z, 0.f);
        Switches[2]->ToggleCheckedState();
        TestEqual(TEXT("Stylized selection is realtime"), Settings->MetalArt.HighlightMode, EStylizedMetalHighlightMode::Stylized);
        GetStylizedCharacterLightingProfiles(NPRPacked, NPRCount);
        TestEqual(TEXT("Stylized selection uploads its mode immediately"), NPRPacked[1].Z, 1.f);
        TestEqual(TEXT("Remembered blend reaches GPU unchanged"), NPRPacked[1].W, .63f);
        if (TestTrue(TEXT("Metal panel includes its live blend slider"), !MetalSpins.IsEmpty()))
        {
            MetalSpins[0]->SetValue(.35f, false);
            TestEqual(TEXT("Blend slider updates the settings without Apply"), Settings->MetalArt.Stylization, .35f);
            GetStylizedCharacterLightingProfiles(NPRPacked, NPRCount);
            TestEqual(TEXT("Blend slider uploads its value without Apply"), NPRPacked[1].W, .35f);
        }
        if (TestTrue(TEXT("Base size and darkening have live numeric controls"), MetalSpins.Num() >= 5))
        {
            MetalSpins[2]->SetValue(.72f, false);
            MetalSpins[3]->SetValue(.58f, false);
            GetStylizedCharacterLightingProfiles(NPRPacked, NPRCount);
            TestEqual(TEXT("Darkening slider uploads without Apply"), NPRPacked[2].X, .72f);
            TestEqual(TEXT("Base size slider uploads without Apply"), NPRPacked[0].W, .58f);
        }
    }
    Cleanup.CheckNoPersistenceOrNativeChanges(*this);
    return true;
}
#endif
