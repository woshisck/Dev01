#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LookDev/YogLookDevLightingRig.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "Misc/Guid.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
template<typename T>
T* AddLightingFixtureComponent(AYogLookDevLightingRig* Rig, const TCHAR* Name)
{
    T* Component = NewObject<T>(Rig, FName(Name), RF_Transient);
    Rig->AddInstanceComponent(Component);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetupAttachment(Rig->GetRootComponent());
    return Component;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLookDevLightingRigDefaultsTest,
    "YogLookDev.LightingRig.DefaultsAndRemovedControls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogLookDevLightingRigDefaultsTest::RunTest(const FString& Parameters)
{
    const AYogLookDevLightingRig* Rig = GetDefault<AYogLookDevLightingRig>();
    TestEqual(TEXT("Initial comparison is MidContrast"), Rig->LightingScenario, EYogLookDevLightingScenario::MidContrast);
    TestEqual(TEXT("Background has no compensation"), Rig->BackgroundCompensation, 0.f);
    TestEqual(TEXT("Exposure has no compensation"), Rig->ExposureCompensation, 0.f);
    TestTrue(TEXT("Grounded starts enabled"), Rig->bGrounded);
    TestTrue(TEXT("Calibrator starts enabled"), Rig->bCalibrator);
    TestEqual(TEXT("Mid key matches authored Blueprint"), Rig->DirectionalSettings.Intensity, 2.3f);
    TestEqual(TEXT("Mid sky matches authored Blueprint"), Rig->SkySettings.Intensity, 1.2f);
    TestEqual(TEXT("Sky occlusion distance matches screenshot"), Rig->SkySettings.OcclusionMaxDistance, 1000.f);
    TestEqual(TEXT("Sky occlusion exponent matches screenshot"), Rig->SkySettings.OcclusionExponent, 1.f);
    TestEqual(TEXT("Sky minimum occlusion matches screenshot"), Rig->SkySettings.MinOcclusion, 0.f);
    TestTrue(TEXT("Sky occlusion tint matches screenshot"), Rig->SkySettings.OcclusionTint == FColor::Black);
    TestTrue(TEXT("Preview behavior is editor-only"), Rig->IsEditorOnly());
    TestFalse(TEXT("No automatic light rotation or game tick"), Rig->PrimaryActorTick.bCanEverTick);
    const TCHAR* RemovedLabels[] = { TEXT("Raytracing Shadow"), TEXT("Enable Transmission"),
        TEXT("Use Hard Ware Raytracing"), TEXT("Enable Checker"), TEXT("Turn Lighting") };
    for (TFieldIterator<FProperty> It(Rig->GetClass(), EFieldIterationFlags::None); It; ++It)
    {
        if (!It->HasAnyPropertyFlags(CPF_Edit)) continue;
        for (const TCHAR* Removed : RemovedLabels)
            TestFalse(*FString::Printf(TEXT("Removed control is not editable: %s"), Removed), It->GetDisplayNameText().ToString() == Removed);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLookDevLightingRigApplicationTest,
    "YogLookDev.LightingRig.ApplyPreservesLayout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogLookDevLightingRigApplicationTest::RunTest(const FString& Parameters)
{
    const FString PackageName = TEXT("/Temp/YogLookDevRigTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    UPackage* Package = CreatePackage(*PackageName);
    Package->SetFlags(RF_Transient);
    UWorld::InitializationValues Init;
    Init.CreatePhysicsScene(false).ShouldSimulatePhysics(false).CreateNavigation(false).CreateAISystem(false).CreateWorldPartition(false);
    TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Inactive, false,
        FName(TEXT("LightingRigTest")), Package, false, ERHIFeatureLevel::Num, &Init));
    if (!TestNotNull(TEXT("Isolated transient world"), World.Get())) return false;
    ON_SCOPE_EXIT { Package->SetDirtyFlag(false); World->DestroyWorld(false); World->MarkObjectsPendingKill(); };
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    Spawn.OverrideLevel = World->PersistentLevel;
    AYogLookDevLightingRig* Rig = World->SpawnActor<AYogLookDevLightingRig>(AYogLookDevLightingRig::StaticClass(), FTransform::Identity, Spawn);
    if (!TestNotNull(TEXT("Native behavior can be tested without authoring assets"), Rig)) return false;
    USceneComponent* Root = NewObject<USceneComponent>(Rig, TEXT("SceneComponent"), RF_Transient);
    Rig->AddInstanceComponent(Root);
    Rig->SetRootComponent(Root);
    auto* Low = AddLightingFixtureComponent<UDirectionalLightComponent>(Rig, TEXT("DirectionalLight_Low"));
    auto* Mid = AddLightingFixtureComponent<UDirectionalLightComponent>(Rig, TEXT("DirectionalLight_Mid"));
    auto* High = AddLightingFixtureComponent<UDirectionalLightComponent>(Rig, TEXT("DirectionalLight_High"));
    auto* Sky = AddLightingFixtureComponent<USkyLightComponent>(Rig, TEXT("Skylight"));
    auto* Post = AddLightingFixtureComponent<UPostProcessComponent>(Rig, TEXT("PostProcess"));
    auto* Checker = AddLightingFixtureComponent<UPostProcessComponent>(Rig, TEXT("PostProcess_PBRChecker"));
    auto* Calibration = AddLightingFixtureComponent<USceneComponent>(Rig, TEXT("SpawnLocation"));
    auto* Character = AddLightingFixtureComponent<USceneComponent>(Rig, TEXT("Character_STM"));
    Calibration->SetRelativeLocation(FVector(15.f, 200.f, 50.f));
    const FVector AuthoredCalibration = Calibration->GetRelativeLocation();
    Rig->RefreshLighting();
    TestTrue(TEXT("Only Mid is visible initially"), !Low->IsVisible() && Mid->IsVisible() && !High->IsVisible());
    TestEqual(TEXT("Mid scalar reaches actual component"), Mid->Intensity, 2.3f);
    TestEqual(TEXT("Sky scalar reaches actual component"), Sky->Intensity, 1.2f);
    TestEqual(TEXT("Sky AO reaches actual component"), Sky->OcclusionMaxDistance, 1000.f);
    TestEqual(TEXT("Manual exposure reaches the authored postprocess"), Post->Settings.AutoExposureMethod.GetValue(), AEM_Manual);
    TestFalse(TEXT("Retired checker cannot remain active"), Checker->bEnabled || Checker->IsVisible());

    Rig->DirectionalSettings.Intensity = 3.6f;
    Rig->SkySettings.Intensity = 1.8f;
    Rig->ExposureCompensation = 0.5f;
    Rig->RefreshLighting();
    Rig->SetLightingScenario(EYogLookDevLightingScenario::MidContrast);
    TestEqual(TEXT("Refreshing same scenario preserves authored intensity"), Mid->Intensity, 3.6f);
    TestEqual(TEXT("Refreshing same scenario preserves sky intensity"), Sky->Intensity, 1.8f);
    TestEqual(TEXT("Exposure edit reaches postprocess"), Post->Settings.AutoExposureBias, 0.5f);
    const FRotator MidRotation = Mid->GetRelativeRotation();
    Rig->RefreshLighting();
    TestTrue(TEXT("Repeated apply cannot accumulate light rotation"), Mid->GetRelativeRotation().Equals(MidRotation));

    Rig->SetLightingScenario(EYogLookDevLightingScenario::LowContrast);
    TestTrue(TEXT("Low selection is exclusive"), Low->IsVisible() && !Mid->IsVisible() && !High->IsVisible());
    TestEqual(TEXT("Low uses original calibrated key"), Low->Intensity, 0.8f);
    TestEqual(TEXT("Low uses original calibrated sky"), Sky->Intensity, 2.5f);
    Rig->SetLightingScenario(EYogLookDevLightingScenario::HighContrast);
    TestTrue(TEXT("High selection is exclusive"), !Low->IsVisible() && !Mid->IsVisible() && High->IsVisible());
    TestEqual(TEXT("High uses original calibrated key"), High->Intensity, 7.f);
    TestEqual(TEXT("High uses original calibrated sky"), Sky->Intensity, 0.42f);
    TestEqual(TEXT("Lighting comparison keeps exposure compensation"), Post->Settings.AutoExposureBias, 0.5f);
    Rig->bCalibrator = false;
    Rig->RefreshLighting();
    TestFalse(TEXT("Calibration group responds to visibility toggle"), Calibration->IsVisible() || Character->IsVisible());
    TestTrue(TEXT("Calibration transform remains artist-authored"), Calibration->GetRelativeLocation().Equals(AuthoredCalibration));
    Rig->bCalibrator = true;
    Rig->RefreshLighting();
    TestTrue(TEXT("Calibration group can be restored"), Calibration->IsVisible() && Character->IsVisible());
    return true;
}

#endif
