#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "YogAirFogVolume.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "UObject/UObjectIterator.h"
#include <limits>

namespace AirFogStabilityTests
{
struct FWorldFixture
{
    UWorld* World = nullptr;

    FWorldFixture()
    {
        if (!GEngine) return;
        const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
            .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
            .ShouldSimulatePhysics(false).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("AirFogStabilityWorld")),
            nullptr, true, ERHIFeatureLevel::Num, &Init);
        if (World) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }

    ~FWorldFixture()
    {
        if (!World) return;
        for (TActorIterator<AYogAirFogVolume> It(World); It; ++It) It->SetAirFogEnabled(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }

    AYogAirFogVolume* SpawnVolume()
    {
        FActorSpawnParameters Args;
        Args.ObjectFlags = RF_Transient;
        AYogAirFogVolume* Air = World->SpawnActor<AYogAirFogVolume>(Args);
        if (Air)
        {
            Air->bAutoActivate = false;
            Air->bFollowPlayer = false;
            Air->bFinishEnabled = false;
        }
        return Air;
    }

    UPointLightComponent* SpawnLight(EComponentMobility::Type Mobility, bool bShadow, bool bDynamicShadow,
        bool bVolumeShadow, float Scattering)
    {
        FActorSpawnParameters Args;
        Args.ObjectFlags = RF_Transient;
        AActor* Owner = World->SpawnActor<AActor>(Args);
        if (!Owner) return nullptr;
        UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
        Owner->SetRootComponent(Light);
        Owner->AddInstanceComponent(Light);
        Light->SetMobility(Mobility);
        Light->CastShadows = bShadow;
        Light->CastDynamicShadows = bDynamicShadow;
        Light->bCastVolumetricShadow = bVolumeShadow;
        Light->VolumetricScatteringIntensity = Scattering;
        Light->RegisterComponent();
        return Light;
    }
};

bool ResourcesReady(FAutomationTestBase& Test)
{
    for (TObjectIterator<AYogAirFogVolume> It; It; ++It)
    {
        if (It->GetWorld() && It->IsAirFogEnabled())
        {
            Test.AddWarning(TEXT("AirFog stability test skipped: close another active preview before testing process-wide settings."));
            return false;
        }
    }
    UMaterial* Density = LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity"));
    return Test.TestTrue(TEXT("The saved native density material is ready; no shared resource is modified"),
        Density && Density->MaterialDomain == MD_Volume && Density->GetShadingModels().HasShadingModel(MSM_DefaultLit));
}

struct FSoftFadingGuard
{
    IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.LightSoftFading"));
    FString Original = Variable ? Variable->GetString() : FString();
    EConsoleVariableFlags Priority = Variable ? static_cast<EConsoleVariableFlags>(Variable->GetFlags() & ECVF_SetByMask) : ECVF_SetByConstructor;
    bool bAuthoredConsole = false;

    void CheckRestored(FAutomationTestBase& Test, const TCHAR* Phase) const
    {
        if (!Test.TestNotNull(TEXT("The local-light soft-fading CVar exists"), Variable)) return;
        Test.TestEqual(FString::Printf(TEXT("%s restores soft-fading value"), Phase), Variable->GetString(), Original);
        Test.TestEqual(FString::Printf(TEXT("%s restores soft-fading priority"), Phase),
            static_cast<uint32>(Variable->GetFlags() & ECVF_SetByMask), static_cast<uint32>(Priority));
    }

    ~FSoftFadingGuard()
    {
        if (!Variable || !bAuthoredConsole) return;
        if (Priority == ECVF_SetByConsole) Variable->Set(*Original, Priority);
        else Variable->Unset(ECVF_SetByConsole);
        IConsoleManager::Get().CallAllConsoleVariableSinks();
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogDensityBoundsTest, "YogVision.AirFog.Stability.DensityBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogDensityBoundsTest::RunTest(const FString&)
{
    FYogAirFogParameters P;
    TestEqual(TEXT("Tune D keeps the far density"), P.Density, .10f);
    TestEqual(TEXT("Default near air has an absolute per-metre density"), P.NearDensity, .006f);
    TestEqual(TEXT("Default camera clear region retains absolute air"), P.ClearMinDensity, .003f);
    TestEqual(TEXT("Camera-depth growth begins at an absolute view depth"), P.NearOffset, 1800.f);
    TestEqual(TEXT("Tune D uses the wider depth growth distance"), P.DepthRamp, 1800.f);
    TestEqual(TEXT("Painterly fog uses a restrained ambient intensity"), P.AmbientIntensity, .25f);
    TestTrue(TEXT("Tune D has the selected ambient colour"), P.AmbientColor == FLinearColor(.016f, .035f, .065f, 1.f));
    TestTrue(TEXT("Tune D has the selected distant colour"), P.FarColor == FLinearColor(.008f, .016f, .032f, 1.f));
    TestTrue(TEXT("Tune D has the selected edge colour"), P.EdgeColor == FLinearColor(.002f, .004f, .008f, 1.f));
    TestEqual(TEXT("Default stability grid uses eight-pixel cells"), P.GridPixelSize, 8);
    TestEqual(TEXT("Default stability grid has 128 depth slices"), P.GridSizeZ, 128);
    P.Density = .007f;
    P.NearDensity = .4f;
    P.ClearMinDensity = .8f;
    P.Sanitize();
    TestEqual(TEXT("Near density cannot exceed total density"), P.NearDensity, P.Density);
    TestEqual(TEXT("Clear density cannot exceed near density"), P.ClearMinDensity, P.NearDensity);
    P.Density = 0.f;
    P.Sanitize();
    TestEqual(TEXT("Zero-density baseline also clears near density"), P.NearDensity, 0.f);
    TestEqual(TEXT("Zero-density baseline also clears minimum clear density"), P.ClearMinDensity, 0.f);
    P.Density = .002f;
    P.NearDensity = std::numeric_limits<float>::quiet_NaN();
    P.ClearMinDensity = std::numeric_limits<float>::infinity();
    P.LightSoftFading = std::numeric_limits<float>::quiet_NaN();
    P.Sanitize();
    TestTrue(TEXT("Nonfinite defaults still obey the current smaller density bound"),
        FMath::IsFinite(P.ClearMinDensity) && P.ClearMinDensity >= 0.f
        && P.ClearMinDensity <= P.NearDensity && P.NearDensity <= P.Density);
    TestEqual(TEXT("Nonfinite near fallback is clamped below its new default"), P.NearDensity, P.Density);
    TestEqual(TEXT("Nonfinite clear fallback is clamped below its new default"), P.ClearMinDensity, P.NearDensity);
    TestEqual(TEXT("Nonfinite soft fading returns its finite default"), P.LightSoftFading, 1.f);
    P.Density = -1.f;
    P.NearDensity = -2.f;
    P.ClearMinDensity = -3.f;
    P.LightSoftFading = 999.f;
    P.Sanitize();
    TestTrue(TEXT("Negative density inputs retain the disabled baseline"), P.Density == 0.f && P.NearDensity == 0.f && P.ClearMinDensity == 0.f);
    TestEqual(TEXT("Soft fading is bounded in froxel units"), P.LightSoftFading, 4.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogLocalLightLifecycleTest, "YogVision.AirFog.Stability.LocalLightLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogLocalLightLifecycleTest::RunTest(const FString&)
{
    using namespace AirFogStabilityTests;
    if (!ResourcesReady(*this)) return !HasAnyErrors();
    FSoftFadingGuard Console;
    FWorldFixture Fixture;
    FWorldFixture OtherWorld;
    if (!TestNotNull(TEXT("A real test World exists"), Fixture.World) || !TestNotNull(TEXT("A second real World exists"), OtherWorld.World)) return false;
    UPointLightComponent* Shadow = Fixture.SpawnLight(EComponentMobility::Movable, true, true, false, .8f);
    UPointLightComponent* Stationary = Fixture.SpawnLight(EComponentMobility::Stationary, true, true, false, .5f);
    UPointLightComponent* Fill = Fixture.SpawnLight(EComponentMobility::Movable, false, true, false, 1.4f);
    UPointLightComponent* NoDynamic = Fixture.SpawnLight(EComponentMobility::Movable, true, false, true, .7f);
    UPointLightComponent* Static = Fixture.SpawnLight(EComponentMobility::Static, true, true, false, .9f);
    UPointLightComponent* Foreign = OtherWorld.SpawnLight(EComponentMobility::Movable, true, true, false, .6f);
    AYogAirFogVolume* Air = Fixture.SpawnVolume();
    if (!TestNotNull(TEXT("Movable shadow light"), Shadow) || !TestNotNull(TEXT("Stationary shadow light"), Stationary)
        || !TestNotNull(TEXT("Unshadowed fill"), Fill) || !TestNotNull(TEXT("No-dynamic-shadow light"), NoDynamic)
        || !TestNotNull(TEXT("Static light"), Static) || !TestNotNull(TEXT("Foreign-world light"), Foreign)
        || !TestNotNull(TEXT("Air-fog actor"), Air) || !TestNotNull(TEXT("Soft-fading CVar"), Console.Variable)) return false;
    UStaticMesh* Mesh = Air->VolumeBounds->GetStaticMesh();
    Air->VolumeBounds->SetStaticMesh(nullptr);
    TestFalse(TEXT("Missing coverage mesh fails open"), Air->SetAirFogEnabled(true));
    TestFalse(TEXT("A failed activation does not acquire local-light shadows"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("A failed activation does not suppress fill scattering"), Fill->VolumetricScatteringIntensity, 1.4f);
    Console.CheckRestored(*this, TEXT("Failed activation"));
    Air->SetAirFogEnabled(false);
    Air->VolumeBounds->SetStaticMesh(Mesh);
    if (!TestTrue(TEXT("Air fog activates with prepared native resources"), Air->SetAirFogEnabled(true))) return false;
    auto CheckActive = [this, Shadow, Stationary, Fill, NoDynamic, Static, Foreign]()
    {
        TestTrue(TEXT("Movable shadowed light gains volumetric shadow"), Shadow->bCastVolumetricShadow != 0);
        TestTrue(TEXT("Stationary shadowed light gains volumetric shadow"), Stationary->bCastVolumetricShadow != 0);
        TestEqual(TEXT("Shadowed light keeps authored scattering"), Shadow->VolumetricScatteringIntensity, .8f);
        TestEqual(TEXT("Unshadowed fill stops lighting the fog"), Fill->VolumetricScatteringIntensity, 0.f);
        TestFalse(TEXT("Fill ordinary shadows are never forced on"), Fill->CastShadows != 0);
        TestEqual(TEXT("A light without dynamic shadows cannot inject unshadowed fog light"), NoDynamic->VolumetricScatteringIntensity, 0.f);
        TestFalse(TEXT("Dynamic shadow policy is never forced on"), NoDynamic->CastDynamicShadows != 0);
        TestFalse(TEXT("Static light is untouched"), Static->bCastVolumetricShadow != 0);
        TestEqual(TEXT("Static light scattering stays authored"), Static->VolumetricScatteringIntensity, .9f);
        TestFalse(TEXT("Another World's light is untouched"), Foreign->bCastVolumetricShadow != 0);
        TestEqual(TEXT("Another World's scattering stays authored"), Foreign->VolumetricScatteringIntensity, .6f);
    };
    auto CheckOriginal = [this, Shadow, Stationary, Fill, NoDynamic]()
    {
        TestFalse(TEXT("Movable light volumetric shadow restores exactly"), Shadow->bCastVolumetricShadow != 0);
        TestFalse(TEXT("Stationary light volumetric shadow restores exactly"), Stationary->bCastVolumetricShadow != 0);
        TestEqual(TEXT("Fill restores its original scattering"), Fill->VolumetricScatteringIntensity, 1.4f);
        TestEqual(TEXT("No-dynamic light restores original scattering"), NoDynamic->VolumetricScatteringIntensity, .7f);
        TestTrue(TEXT("An authored volumetric-shadow flag is retained"), NoDynamic->bCastVolumetricShadow != 0);
    };
    CheckActive();
    Shadow->CastDynamicShadows = false;
    Air->Tick(1.01f);
    TestFalse(TEXT("Changing ordinary shadow policy releases the temporary volume-shadow flag"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("A newly unshadowed light becomes a non-volumetric fill"), Shadow->VolumetricScatteringIntensity, 0.f);
    Shadow->CastDynamicShadows = true;
    Air->Tick(1.01f);
    CheckActive();
    Shadow->SetMobility(EComponentMobility::Static);
    Air->Tick(1.01f);
    TestFalse(TEXT("An acquired light becoming static restores our temporary flag"), Shadow->bCastVolumetricShadow != 0);
    Shadow->SetMobility(EComponentMobility::Movable);
    Air->Tick(1.01f);
    CheckActive();
    TestEqual(TEXT("Activation scopes light soft fading"), Console.Variable->GetFloat(), 1.f);
    FYogAirFogParameters P = Air->GetParameters();
    P.LightSoftFading = 2.f;
    Air->ApplyParameters(P);
    TestEqual(TEXT("Parameter updates adjust owned light soft fading"), Console.Variable->GetFloat(), 2.f);
    UPointLightComponent* Added = Fixture.SpawnLight(EComponentMobility::Movable, true, true, false, .3f);
    if (!TestNotNull(TEXT("A dynamically attached torch-like light exists"), Added)) return false;
    Air->Tick(1.01f);
    TestTrue(TEXT("Periodic scan acquires newly registered local lights"), Added->bCastVolumetricShadow != 0);
    Air->SuspendForSave(true);
    Air->SuspendForSave(true);
    CheckOriginal();
    TestFalse(TEXT("Save suspension restores newly registered lights too"), Added->bCastVolumetricShadow != 0);
    Console.CheckRestored(*this, TEXT("Nested save"));
    Air->SuspendForSave(false);
    CheckOriginal();
    Air->SuspendForSave(false);
    CheckActive();
    TestEqual(TEXT("Save resumes the requested soft-fading value"), Console.Variable->GetFloat(), 2.f);
    Air->bManageLocalLightScattering = false;
    Air->Tick(.01f);
    CheckOriginal();
    Air->bManageLocalLightScattering = true;
    Air->Tick(1.01f);
    CheckActive();
    Air->Destroy();
    CheckOriginal();
    TestFalse(TEXT("Destroy restores dynamically registered light shadows"), Added->bCastVolumetricShadow != 0);
    Console.CheckRestored(*this, TEXT("Actor destruction"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogLocalLightExternalEditTest, "YogVision.AirFog.Stability.LocalLightExternalEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogLocalLightExternalEditTest::RunTest(const FString&)
{
    using namespace AirFogStabilityTests;
    if (!ResourcesReady(*this)) return !HasAnyErrors();
    FSoftFadingGuard Console;
    FWorldFixture Fixture;
    if (!TestNotNull(TEXT("A real World exists"), Fixture.World)) return false;
    UPointLightComponent* Shadow = Fixture.SpawnLight(EComponentMobility::Movable, true, true, false, .8f);
    UPointLightComponent* Fill = Fixture.SpawnLight(EComponentMobility::Movable, false, true, false, 1.4f);
    AYogAirFogVolume* Air = Fixture.SpawnVolume();
    if (!TestNotNull(TEXT("Shadow light"), Shadow) || !TestNotNull(TEXT("Fill light"), Fill)
        || !TestNotNull(TEXT("Air fog"), Air) || !TestNotNull(TEXT("Soft-fading CVar"), Console.Variable)
        || !TestTrue(TEXT("Air fog activates"), Air->SetAirFogEnabled(true))) return false;
    Shadow->SetCastVolumetricShadow(false);
    Fill->SetVolumetricScatteringIntensity(.37f);
    Air->Tick(1.01f);
    TestFalse(TEXT("Periodic scan respects the newer user's volume-shadow choice"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Periodic scan respects the newer user's fill scattering"), Fill->VolumetricScatteringIntensity, .37f);
    Air->SuspendForSave(true);
    Air->SuspendForSave(true);
    TestFalse(TEXT("Nested save leaves the user's shadow choice intact"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Nested save leaves the user's scattering choice intact"), Fill->VolumetricScatteringIntensity, .37f);
    Air->SuspendForSave(false);
    Air->SuspendForSave(false);
    Air->Tick(1.01f);
    TestFalse(TEXT("Save resume retains the shadow field's external opt-out"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Save resume retains the scattering field's external opt-out"), Fill->VolumetricScatteringIntensity, .37f);
    Console.Variable->Set(1.5f, ECVF_SetByConsole);
    Console.bAuthoredConsole = true;
    FYogAirFogParameters P = Air->GetParameters();
    P.LightSoftFading = 3.f;
    Air->ApplyParameters(P);
    TestEqual(TEXT("Parameter updates preserve newer console soft fading"), Console.Variable->GetFloat(), 1.5f);
    // Status polling exercises the CVar pointer cache instead of repeated engine lookups.
    for (int32 Index = 0; Index < 600; ++Index) Air->GetStatusText();
    Air->SetAirFogEnabled(false);
    TestFalse(TEXT("Disable preserves the user's volume-shadow choice"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Disable preserves the user's fill scattering"), Fill->VolumetricScatteringIntensity, .37f);
    TestEqual(TEXT("Disable preserves external soft fading"), Console.Variable->GetFloat(), 1.5f);
    TestEqual(TEXT("External soft fading keeps Console priority"), static_cast<uint32>(Console.Variable->GetFlags() & ECVF_SetByMask), static_cast<uint32>(ECVF_SetByConsole));
    // Cleanup must also preserve a newer edit made between scans.
    if (!TestTrue(TEXT("Another activation succeeds"), Air->SetAirFogEnabled(true))) return false;
    TestTrue(TEXT("Explicit off/on begins a new shadow-management scope"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Explicit off/on begins a new scattering-management scope"), Fill->VolumetricScatteringIntensity, 0.f);
    Shadow->SetCastVolumetricShadow(false);
    Fill->SetVolumetricScatteringIntensity(.46f);
    Air->SuspendForSave(true);
    Air->SuspendForSave(false);
    TestFalse(TEXT("Save detects a newer shadow edit even before the periodic scan"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Save detects a newer scattering edit even before the periodic scan"), Fill->VolumetricScatteringIntensity, .46f);
    Air->Destroy();
    TestFalse(TEXT("Destruction does not overwrite an unobserved newer shadow edit"), Shadow->bCastVolumetricShadow != 0);
    TestEqual(TEXT("Destruction does not overwrite an unobserved newer scattering edit"), Fill->VolumetricScatteringIntensity, .46f);
    return true;
}

#endif
