#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "YogNativeAtlasFogActor.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include <limits>

namespace NativeAtlasFogTests
{
struct FWorldFixture
{
    UWorld* World = nullptr;
    FWorldFixture()
    {
        if (!GEngine) return;
        const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("NativeAtlasFogTestWorld")),
            nullptr, true, ERHIFeatureLevel::Num, &Init);
        if (World) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    ~FWorldFixture()
    {
        if (!World) return;
        if (GEngine) GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
    AYogNativeAtlasFogActor* Spawn()
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags = RF_Transient;
        return World ? World->SpawnActor<AYogNativeAtlasFogActor>(Params) : nullptr;
    }
};
UMaterialInterface* RequireMaterial(FAutomationTestBase& Test)
{
    auto* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/YogVision/Stage12/Native/M_Stage12NativeAtlasFog.M_Stage12NativeAtlasFog"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    Test.TestNotNull(TEXT("Prepare native volume material before native integration tests"), Material);
    return Material;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogNativeAtlasBindingTest, "YogVision.NativeAtlas.MaterialBindingAndStaticUploadIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogNativeAtlasBindingTest::RunTest(const FString&)
{
    using namespace NativeAtlasFogTests;
    UMaterialInterface* Material = RequireMaterial(*this);
    if (!Material) return false;
    FWorldFixture Fixture;
    AYogNativeAtlasFogActor* Fog = Fixture.Spawn();
    if (!TestNotNull(TEXT("Native fog actor"), Fog)) return false;
    Fog->SetVolumeMaterial(Material);
    Fog->bUseSceneDirectionalLight = false;
    Fog->bUseArtistPalette = false;
    Fog->PaletteMaterial = nullptr;
    Fog->BoundsExtent = FVector(100., 200., 300.);
    const FTransform Placement(FRotator(14., 28., -7.), FVector(300., -500., 220.), FVector(2., 3., 4.));
    Fog->SetActorTransform(Placement);
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("Actual mesh material is the actor's own dynamic volume material"), Fog->IsFogMaterialBound());
    TestTrue(TEXT("Cube scale converts half-extents using its 50cm half-size"),
        Fog->VolumeMesh->GetRelativeScale3D().Equals(FVector(2., 4., 6.), .0001));
    TestTrue(TEXT("The native fog renderer is enabled"), Fog->NativeFog->bEnableVolumetricFog);
    TestFalse(TEXT("Screen-space fog scattering is disabled"), Fog->NativeFog->bEnableFSSS);
    TestEqual(TEXT("Global height density remains only a native-fog carrier"), Fog->NativeFog->FogDensity, .00001f);
    const int32 Uploads = Fog->StaticParameterUploadCount;
    Fog->Tick(.1f);
    Fog->Tick(.1f);
    TestEqual(TEXT("Clock-only ticks do not upload static parameters or repeatedly rebuild the mesh"),
        Fog->StaticParameterUploadCount, Uploads);
    Fog->VolumeMesh->SetMaterial(0, Material);
    Fog->Tick(.1f);
    TestTrue(TEXT("A displaced mesh material is repaired to the actual animated MID"), Fog->IsFogMaterialBound());
    UMaterialInstanceDynamic* BeforePlay = Fog->GetFogMID();
    Fog->DispatchBeginPlay();
    TestTrue(TEXT("BeginPlay creates a fresh owned MID rather than reusing preview state"),
        Fog->GetFogMID() && Fog->GetFogMID() != BeforePlay && Fog->GetFogMID()->GetOuter() == Fog);
    TestTrue(TEXT("BeginPlay also binds the fresh material to the mesh"), Fog->VolumeMesh->GetMaterial(0) == Fog->GetFogMID());
    Fog->ResetFogAnimation();
    Fog->Tick(.25f);
    float BoundTime = -1.f;
    auto* BoundMID = Cast<UMaterialInstanceDynamic>(Fog->VolumeMesh->GetMaterial(0));
    TestTrue(TEXT("Clock updates reach the material actually assigned to the volume primitive"),
        BoundMID && BoundMID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FogTimeSeconds")), BoundTime)
        && FMath::IsNearlyEqual(BoundTime, .25f));
    TestTrue(TEXT("Density animation preserves the placed container transform"), Fog->GetActorTransform().Equals(Placement, .0001));
    Fog->SetFogEnabled(false);
    TestFalse(TEXT("Disabling removes the injected volume primitive"), Fog->VolumeMesh->IsVisible());
    TestFalse(TEXT("Disabling removes this actor's native fog carrier"), Fog->NativeFog->IsVisible());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogNativeAtlasClockTest, "YogVision.NativeAtlas.EditorRuntimeAndCaptureClock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogNativeAtlasClockTest::RunTest(const FString&)
{
    using namespace NativeAtlasFogTests;
    FWorldFixture Fixture;
    AYogNativeAtlasFogActor* Fog = Fixture.Spawn();
    if (!Fog || !Fixture.World) return false;
    Fixture.World->WorldType = EWorldType::Editor;
    Fog->bUseExplicitPreviewFocus = true;
    Fog->PreviewFocusPosition = FVector(110., 230., 150.);
    Fog->bAnimateInEditor = false;
    Fog->ResetFogAnimation();
    Fog->Tick(1.f);
    TestEqual(TEXT("Editor playback can be paused independently"), Fog->FogTimeSeconds, 0.f);
    TestTrue(TEXT("Fixed editor focus supplies a valid world position"),
        Fog->EffectiveObserverPosition.Equals(FLinearColor(110.f, 230.f, 150.f, 1.f)));
    Fog->bAnimateInEditor = true;
    Fog->AnimationSpeed = 2.f;
    Fog->Tick(.25f);
    TestEqual(TEXT("Editor clock incorporates speed exactly once"), Fog->FogTimeSeconds, .5f);
    Fog->bAnimateFog = false;
    Fog->Tick(100.f);
    TestEqual(TEXT("Pause retains the current atlas time"), Fog->FogTimeSeconds, .5f);
    Fog->bAnimateFog = true;
    Fog->bAnimateInEditor = false;
    Fixture.World->WorldType = EWorldType::PIE;
    Fog->Tick(.25f);
    TestEqual(TEXT("Runtime ignores the editor playback toggle"), Fog->FogTimeSeconds, 1.f);
    Fog->SetFogTimeForCapture(20.f);
    Fog->Tick(10.f);
    TestEqual(TEXT("Capture time is deterministic across ticks"), Fog->FogTimeSeconds, 20.f);
    Fog->ClearFogTimeOverride();
    TestEqual(TEXT("Capture override preserves pre-capture elapsed time"), Fog->FogTimeSeconds, 1.f);
    Fog->Tick(std::numeric_limits<float>::quiet_NaN());
    Fog->Tick(-1.f);
    TestEqual(TEXT("Invalid or reversed time cannot contaminate animation"), Fog->FogTimeSeconds, 1.f);
    Fog->bEnableVision = false;
    Fog->Tick(.25f);
    TestEqual(TEXT("Clearing vision disables the observer mask explicitly"), Fog->EffectiveObserverPosition.A, 0.f);
    return !HasAnyErrors();
}

#endif
