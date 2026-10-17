#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RenderStyle/YogShadowTransitionLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "StylizedCharacterLighting.h"
#include "StylizedLightingSettings.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevService.h"

namespace
{
    const TCHAR* ShadowTransitionSpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    const TCHAR* ShadowTransitionSpherePackage = TEXT("/Engine/BasicShapes/Sphere");
    constexpr double ShadowTransitionTotalTimeout = 120.0;
    constexpr double ShadowTransitionSettleSeconds = 2.0;

    TSet<FString> ShadowTransitionReportNames(const FString& Directory)
    {
        TArray<FString> Files;
        IFileManager::Get().FindFiles(Files, *(Directory / TEXT("LookDev_*.json")), true, false);
        TSet<FString> Result;
        for (const FString& File : Files) Result.Add(File);
        return Result;
    }

    /** Nonblocking real-RHI capture fixture; only for a dedicated disposable editor process. */
    class FDevKitShadowTransitionRenderCommand final : public IAutomationLatentCommand
    {
    public:
        explicit FDevKitShadowTransitionRenderCommand(FAutomationTestBase* InTest)
            : Test(InTest), Began(FPlatformTime::Seconds()), Settings(GetMutableDefault<UStylizedLightingSettings>()),
              SceneBefore(Settings->SceneShadowTransition), ProfilesBefore(Settings->CharacterLightingProfiles)
        {
            GetStylizedCharacterLightingProfiles(PackedBefore, ProfileCountBefore);
            for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
                TEXT("r.StylizedLumenLighting.BandSoftness")})
            {
                if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
                    CVarsBefore.Add(Variable, {Variable->GetString(), Variable->GetFlags()});
            }
        }

        virtual ~FDevKitShadowTransitionRenderCommand() override { RestoreSettings(); }

        virtual bool Update() override
        {
            const double Now = FPlatformTime::Seconds();
            if (Now - Began >= ShadowTransitionTotalTimeout)
                return Fail(FString::Printf(TEXT("Shadow transition RenderSmoke exceeded total %.0f seconds, variant %d, state %d. Pending PNG: %s"),
                    ShadowTransitionTotalTimeout, CaptureIndex, static_cast<int32>(State), *ExpectedPng));
            FText Message;
            switch (State)
            {
            case EState::Setup:
            {
                if (CVarsBefore.Num() != 3)
                    return Fail(TEXT("RenderSmoke requires all three registered scene-style CVars: Enable, BandCount, BandSoftness."));
                if (!FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message)) return Fail(Message.ToString());
                UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, ShadowTransitionSpherePath);
                if (!Sphere) return Fail(TEXT("Cannot load actual Engine BasicShapes Sphere for shadow transition capture."));
                UMaterialInterface* Material = Sphere->GetMaterial(0);
                if (!Material || !Material->GetShadingModels().HasShadingModel(MSM_DefaultLit)
                    || Material->GetShadingModels().HasShadingModel(MSM_Unlit))
                    return Fail(TEXT("Engine Sphere no longer uses the expected original Default Lit material; no material was replaced."));
                if (!FDevKitCharacterLookDevService::SetSubject(Sphere, Message)) return Fail(Message.ToString());
                if (!FDevKitCharacterLookDevService::SetCamera(false, Message))
                    return Fail(TEXT("Shadow transition capture requires an actual level perspective viewport: ") + Message.ToString());
                Test->AddInfo(FString::Printf(TEXT("Shadow transition actual subject=%s material=%s. Temporary Neutral stage; no asset or map save."),
                    ShadowTransitionSpherePath, *Material->GetPathName()));
                // Stage setup may read caller state; only now lower the priorities
                // of the three explicit fixture CVars. Never run the full settings upload.
                for (const auto& Pair : CVarsBefore) Pair.Key->ClearFlags(ECVF_SetByMask);
                State = EState::Apply;
                return false;
            }
            case EState::Apply:
            {
                IConsoleVariable* StyleEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedLumenLighting.Enable"));
                IConsoleVariable* BandCount = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedLumenLighting.BandCount"));
                IConsoleVariable* BandSoftness = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedLumenLighting.BandSoftness"));
                if (!StyleEnable || !BandCount || !BandSoftness)
                    return Fail(TEXT("A scene-style fixture CVar was unregistered during capture."));
                StyleEnable->Set(CaptureIndex == 0 ? 0 : 1, ECVF_SetByCode);
                BandCount->Set(2.f, ECVF_SetByCode);
                BandSoftness->Set(.18f, ECVF_SetByCode);
                if (StyleEnable->GetInt() != (CaptureIndex == 0 ? 0 : 1)
                    || !FMath::IsNearlyEqual(BandCount->GetFloat(), 2.f)
                    || !FMath::IsNearlyEqual(BandSoftness->GetFloat(), .18f))
                    return Fail(TEXT("Fixture scene-style CVars did not accept the requested values; capture would not represent the requested variant."));
                FYogShadowTransitionSettings Parameters;
                Parameters.bEnabled = CaptureIndex == 2;
                Parameters.Wrap = 1.f;
                Parameters.Falloff = 1.f;
                Parameters.Strength = 1.f;
                Parameters.Tint = FLinearColor(.2f, .4f, .7f, 1.f);
                TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> PackedBeforeLegacyCall;
                uint32 CountBeforeLegacyCall = 0;
                Settings->ApplyShadowTransitionParameters();
                GetStylizedCharacterLightingProfiles(PackedBeforeLegacyCall, CountBeforeLegacyCall);
                UYogShadowTransitionLibrary::SetGlobalShadowTransition(Parameters);
                TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> PackedAfterLegacyCall;
                uint32 CountAfterLegacyCall = 0;
                GetStylizedCharacterLightingProfiles(PackedAfterLegacyCall, CountAfterLegacyCall);
                if (CountAfterLegacyCall != CountBeforeLegacyCall)
                    return Fail(TEXT("Retired tint setter changed the renderer profile count."));
                for (int32 Index = 0; Index < PackedAfterLegacyCall.Num(); ++Index)
                    if (PackedAfterLegacyCall[Index] != PackedBeforeLegacyCall[Index])
                        return Fail(TEXT("Retired tint setter uploaded renderer data."));
                if (CaptureIndex == 2)
                {
                    // Simulate loading an old enabled/nonwhite ini and then a
                    // normal profile upload. Compatibility data must stay inert.
                    Settings->SceneShadowTransition = Parameters;
                    for (FStylizedCharacterLightingProfile& Profile : Settings->CharacterLightingProfiles)
                    {
                        Profile.bOverrideShadowTransition = true;
                        Profile.ShadowTransition = Parameters;
                    }
                    Settings->ApplyShadowTransitionParameters();
                    GetStylizedCharacterLightingProfiles(PackedAfterLegacyCall, CountAfterLegacyCall);
                    for (uint32 Index = 0; Index < CountAfterLegacyCall; ++Index)
                    {
                        const uint32 Base = Index * StylizedCharacterLighting::Float4sPerProfile;
                        for (uint32 Slot = 0; Slot < StylizedCharacterLighting::Float4sPerProfile; ++Slot)
                            if (PackedAfterLegacyCall[Base + Slot] != PackedBeforeLegacyCall[Base + Slot])
                                return Fail(TEXT("Loaded legacy tint settings modified active GPU profile data."));
                    }
                }
                Test->AddInfo(FString::Printf(TEXT("SHADOW_TRANSITION_CAPTURE_SETTINGS variant=%s scene_style_enable=%d band_count=2 band_softness=.18 legacy_tint_requested=%s effective_tint=disabled. Retired API and old enabled/nonwhite profile fields leave current metal/profile data unchanged; no configuration or asset save."),
                    VariantName(), StyleEnable->GetInt(), Parameters.bEnabled ? TEXT("true") : TEXT("false")));
                StableSince = 0;
                StableUpdates = 0;
                State = EState::Settle;
                return false;
            }
            case EState::Settle:
                // Ordinary editor frames handle shader completion and scene updates.
                // Do not block with Sleep, FinishAllCompilation or a render-thread wait loop.
                if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
                {
                    StableSince = 0;
                    StableUpdates = 0;
                    return false;
                }
                if (StableSince == 0) StableSince = Now;
                ++StableUpdates;
                if (Now - StableSince < ShadowTransitionSettleSeconds || StableUpdates < 20) return false;
                return RequestCapture();
            case EState::WaitForPng:
            {
                const int64 Size = IFileManager::Get().FileSize(*ExpectedPng);
                if (Size <= 32) return false;
                if (Size != LastPngSize)
                {
                    LastPngSize = Size;
                    PngStableSince = Now;
                    return false;
                }
                if (Now - PngStableSince < .5) return false;
                TArray<uint8> Bytes;
                if (!FFileHelper::LoadFileToArray(Bytes, *ExpectedPng)) return false;
                const uint8 Signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
                if (Bytes.Num() < 32 || FMemory::Memcmp(Bytes.GetData(), Signature, sizeof(Signature)) != 0)
                    return Fail(TEXT("Shadow transition capture is not a nonempty PNG: ") + ExpectedPng);
                const uint8* Data = Bytes.GetData();
                const uint32 Width = (uint32(Data[16]) << 24) | (uint32(Data[17]) << 16) | (uint32(Data[18]) << 8) | uint32(Data[19]);
                const uint32 Height = (uint32(Data[20]) << 24) | (uint32(Data[21]) << 16) | (uint32(Data[22]) << 8) | uint32(Data[23]);
                if (Width == 0 || Height == 0) return Fail(TEXT("Shadow transition capture has empty PNG dimensions: ") + ExpectedPng);
                if (CaptureIndex == 0) NativeCaptureBytes = Bytes;
                else if (CaptureIndex == 1)
                {
                    if (Bytes == NativeCaptureBytes)
                        return Fail(TEXT("TwoBand PNG is byte-identical to Native PNG; the actual scene-style response was not observed: ") + ExpectedPng);
                    Test->AddInfo(TEXT("SHADOW_TRANSITION_CAPTURE_DIFFERENCE Native versus TwoBand: PNG payloads differ. This is not a pixel-quality or art-approval assertion."));
                }
                else
                    Test->AddInfo(TEXT("SHADOW_TRANSITION_LEGACY_INERT: legacy enabled/nonwhite settings uploaded neutral slots. A temporal rendered PNG is retained for inspection, not used as a deterministic image-equality assertion."));
                Test->AddInfo(FString::Printf(TEXT("SHADOW_TRANSITION_CAPTURE_COMPLETE variant=%s file=%s report=%s bytes=%lld width=%u height=%u"),
                    VariantName(), *ExpectedPng, *ExpectedReport, Size, Width, Height));
                CompletedPngs.Add(ExpectedPng);
                ++CaptureIndex;
                if (CaptureIndex == 3)
                {
                    RestoreSettings();
                    Test->AddInfo(TEXT("Three actual PNG paths, Native/TwoBand response and retired-tint neutral-upload contract verified. Original CDO/profile table and all three fixture CVar values/priorities restored. This smoke test does not approve art, visual matching, or performance."));
                    return true;
                }
                State = EState::Apply;
                return false;
            }
            }
            return Fail(TEXT("Unknown shadow transition render smoke state."));
        }

    private:
        const TCHAR* VariantName() const
        {
            switch (CaptureIndex)
            {
            case 0: return TEXT("Native");
            case 1: return TEXT("TwoBand");
            case 2: return TEXT("LegacyTintInert");
            default: return TEXT("Complete");
            }
        }

        void RestoreSettings()
        {
            if (bRestored) return;
            bRestored = true;
            Settings->SceneShadowTransition = SceneBefore;
            Settings->CharacterLightingProfiles = ProfilesBefore;
            SetStylizedCharacterLightingProfiles(MakeArrayView(PackedBefore.GetData(), PackedBefore.Num()), ProfileCountBefore);
            for (const auto& Pair : CVarsBefore)
            {
                Pair.Key->ClearFlags(ECVF_SetByMask);
                Pair.Key->Set(*Pair.Value.Value, static_cast<EConsoleVariableFlags>(Pair.Value.Flags & ECVF_SetByMask));
                Pair.Key->SetFlags(Pair.Value.Flags);
            }
        }

        bool Fail(const FString& Reason)
        {
            RestoreSettings();
            Test->AddError(Reason);
            return true;
        }

        bool RequestCapture()
        {
            const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("CharacterLookDev"));
            const TSet<FString> BeforeReports = ShadowTransitionReportNames(Directory);
            TArray<FString> BeforePng;
            IFileManager::Get().FindFiles(BeforePng, *(Directory / TEXT("*.png")), true, false);
            FText Message;
            if (!FDevKitCharacterLookDevService::Capture(Message)) return Fail(Message.ToString());
            ExpectedPng.Reset();
            ExpectedReport.Reset();
            for (const FString& Name : ShadowTransitionReportNames(Directory))
            {
                if (BeforeReports.Contains(Name)) continue;
                FString Text;
                TSharedPtr<FJsonObject> Json;
                if (!FFileHelper::LoadFileToString(Text, *(Directory / Name))
                    || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid()) continue;
                FString Id, Status, Path;
                if (Json->TryGetStringField(TEXT("preset_id"), Id) && Id == TEXT("Neutral")
                    && Json->TryGetStringField(TEXT("capture_status"), Status) && Status == TEXT("requested_not_verified_complete")
                    && Json->TryGetStringField(TEXT("capture_path"), Path) && !Path.IsEmpty())
                {
                    ExpectedPng = Path;
                    ExpectedReport = Directory / Name;
                    break;
                }
            }
            if (ExpectedPng.IsEmpty()) return Fail(TEXT("Capture request did not produce a fresh Neutral report containing the exact PNG path."));
            if (BeforePng.Contains(FPaths::GetCleanFilename(ExpectedPng)) || CompletedPngs.Contains(ExpectedPng))
                return Fail(TEXT("Capture reused an existing PNG path; cannot claim a new rendered result: ") + ExpectedPng);
            Test->AddInfo(FString::Printf(TEXT("SHADOW_TRANSITION_CAPTURE_REQUESTED variant=%s file=%s report=%s"),
                VariantName(), *ExpectedPng, *ExpectedReport));
            LastPngSize = -1;
            PngStableSince = 0;
            State = EState::WaitForPng;
            return false;
        }

        enum class EState { Setup, Apply, Settle, WaitForPng };
        FAutomationTestBase* Test;
        double Began;
        UStylizedLightingSettings* Settings;
        FYogShadowTransitionSettings SceneBefore;
        TArray<FStylizedCharacterLightingProfile> ProfilesBefore;
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> PackedBefore;
        uint32 ProfileCountBefore = 0;
        struct FCVarSnapshot { FString Value; EConsoleVariableFlags Flags; };
        TMap<IConsoleVariable*, FCVarSnapshot> CVarsBefore;
        EState State = EState::Setup;
        int32 CaptureIndex = 0;
        double StableSince = 0;
        int32 StableUpdates = 0;
        double PngStableSince = 0;
        int64 LastPngSize = -1;
        FString ExpectedPng;
        FString ExpectedReport;
        TArray<FString> CompletedPngs;
        TArray<uint8> NativeCaptureBytes;
        bool bRestored = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitShadowTransitionRenderSmokeTest,
    "DevKitEditor.ShadowTransition.RenderSmoke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitShadowTransitionRenderSmokeTest::RunTest(const FString&)
{
    if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("Dev01ShadowTransitionRender")))
    {
        AddInfo(TEXT("Skipped: shadow transition RenderSmoke requires -Unattended -Dev01ShadowTransitionRender in a dedicated real-RHI editor process. It creates an unsaved temporary stage; never run in an interactive editing session."));
        return true;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("NullRHI")))
    {
        AddError(TEXT("Shadow transition RenderSmoke requires a real rendering device; -NullRHI cannot validate actual PNG captures."));
        return false;
    }
    if (!FPackageName::DoesPackageExist(ShadowTransitionSpherePackage))
    {
        AddError(TEXT("Actual Engine BasicShapes Sphere package is missing."));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDevKitShadowTransitionRenderCommand(this));
    return true;
}

#endif
