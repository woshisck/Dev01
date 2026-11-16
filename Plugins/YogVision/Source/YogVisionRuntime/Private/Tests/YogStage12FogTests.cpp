#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "YogStage12FogActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "Engine/VolumeTexture.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ScopeExit.h"
#include <limits>

namespace Stage12FogTests
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
            MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("Stage12TestWorld")),
            nullptr, true, ERHIFeatureLevel::Num, &Init);
        if (World) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    ~FWorldFixture()
    {
        if (!World) return;
        if (GEngine) GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
    AYogStage12FogActor* SpawnFog()
    {
        FActorSpawnParameters Spawn;
        Spawn.ObjectFlags = RF_Transient;
        return World ? World->SpawnActor<AYogStage12FogActor>(Spawn) : nullptr;
    }
};

UMaterialInterface* RequireMaterial(FAutomationTestBase& Test)
{
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/YogVision/Stage12/M_Stage12SpatialFog.M_Stage12SpatialFog"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    Test.TestNotNull(TEXT("Prepare the saved Stage12 spatial integration material before running native integration tests"), Material);
    return Material;
}

template<typename T>
T* SpawnAnchor(UWorld* World, const FVector& Position)
{
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    T* Actor = World->SpawnActor<T>(Spawn);
    if (!Actor) return nullptr;
    USceneComponent* Root = NewObject<USceneComponent>(Actor);
    Root->SetMobility(EComponentMobility::Movable);
    Actor->SetRootComponent(Root);
    Actor->AddInstanceComponent(Root);
    Root->RegisterComponent();
    Actor->SetActorLocation(Position);
    return Actor;
}

FLinearColor ReadVector(FAutomationTestBase& Test, UMaterialInstanceDynamic* MID, const TCHAR* Name)
{
    FLinearColor Result = FLinearColor::Transparent;
    Test.TestTrue(FString::Printf(TEXT("MID contains vector %s"), Name),
        MID && MID->GetVectorParameterValue(FMaterialParameterInfo(Name), Result));
    return Result;
}

float ReadScalar(FAutomationTestBase& Test, UMaterialInstanceDynamic* MID, const TCHAR* Name)
{
    float Result = -999.f;
    Test.TestTrue(FString::Printf(TEXT("MID contains scalar %s"), Name),
        MID && MID->GetScalarParameterValue(FMaterialParameterInfo(Name), Result));
    return Result;
}

FVector Unpack(const FLinearColor& Value) { return FVector(Value.R, Value.G, Value.B); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogStage12AnchorTest, "YogVision.Stage12.WorldAnchorAndSafeInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogStage12AnchorTest::RunTest(const FString&)
{
    using namespace Stage12FogTests;
    UMaterialInterface* Material = RequireMaterial(*this);
    if (!Material) return false;
    FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Temporary test world"), Fixture.World)) return false;
    AYogStage12FogActor* Fog = Fixture.SpawnFog();
    if (!TestNotNull(TEXT("Stage12 actor"), Fog)) return false;
    Fog->SetFogMaterial(Material);
    UMaterialInstanceDynamic* MID = Fog->GetFogMID();
    if (!TestNotNull(TEXT("Owned dynamic material"), MID)) return false;
    const FVector Position(320., -270., 910.);
    const FRotator Rotation(20., 47., -12.);
    Fog->BoundsExtent = FVector(100., 200., 300.);
    Fog->SetActorTransform(FTransform(Rotation, Position, FVector(2., 3., 4.)));
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("World centre follows actor, without any camera dependency"),
        Unpack(ReadVector(*this, MID, TEXT("VolumeCenter"))).Equals(Position, .01));
    TestTrue(TEXT("World half-extent includes actor scale once"),
        Unpack(ReadVector(*this, MID, TEXT("VolumeExtent"))).Equals(FVector(200., 600., 1200.), .01));
    const FVector X = Unpack(ReadVector(*this, MID, TEXT("VolumeAxisX")));
    const FVector Y = Unpack(ReadVector(*this, MID, TEXT("VolumeAxisY")));
    const FVector Z = Unpack(ReadVector(*this, MID, TEXT("VolumeAxisZ")));
    TestTrue(TEXT("Volume axes remain orthonormal under nonuniform scale"),
        FMath::IsNearlyEqual(X.Size(), 1., .0001) && FMath::IsNearlyEqual(Y.Size(), 1., .0001)
        && FMath::IsNearlyEqual(Z.Size(), 1., .0001) && FMath::Abs(X.Dot(Y)) < .0001
        && FMath::Abs(Y.Dot(Z)) < .0001 && FMath::Abs(Z.Dot(X)) < .0001);
    TestTrue(TEXT("Axis orientation matches the placed world volume"), X.Equals(Rotation.Quaternion().GetAxisX(), .0001));
    Fog->Density = std::numeric_limits<float>::quiet_NaN();
    Fog->BoundsExtent = FVector(std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(), -2.);
    Fog->StepCount = -10;
    Fog->ObserverFeather = 0.f;
    Fog->ClearDensityFraction = -1.f;
    Fog->RefreshMaterialParameters();
    const FVector SafeExtent = Unpack(ReadVector(*this, MID, TEXT("VolumeExtent")));
    const float SafeDensity = ReadScalar(*this, MID, TEXT("Density"));
    TestTrue(TEXT("Invalid extents never upload NaN/Inf or nonpositive dimensions"),
        !SafeExtent.ContainsNaN() && SafeExtent.X > 0. && SafeExtent.Y > 0. && SafeExtent.Z > 0.);
    TestTrue(TEXT("Invalid density is finite and nonnegative"), FMath::IsFinite(SafeDensity) && SafeDensity >= 0.f);
    TestTrue(TEXT("Integrator count cannot become zero/negative"), ReadScalar(*this, MID, TEXT("StepCount")) >= 8.f);
    TestTrue(TEXT("Observer feather cannot divide by zero"), ReadScalar(*this, MID, TEXT("ObserverFeather")) > 0.f);
    TestTrue(TEXT("Clear density fraction cannot become negative extinction"), ReadScalar(*this, MID, TEXT("ClearDensityFraction")) >= 0.f);
    Fog->SetFogEnabled(false);
    TestFalse(TEXT("Disabling the effect disables only the actor's post process"), !!Fog->PostProcess->bEnabled);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogStage12ObserverTest, "YogVision.Stage12.GameplayObserverIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogStage12ObserverTest::RunTest(const FString&)
{
    using namespace Stage12FogTests;
    UMaterialInterface* Material = RequireMaterial(*this);
    if (!Material) return false;
    FWorldFixture Fixture, ForeignWorld;
    if (!Fixture.World || !ForeignWorld.World) return false;
    AYogStage12FogActor* Fog = Fixture.SpawnFog();
    AActor* Explicit = SpawnAnchor<AActor>(Fixture.World, FVector(110., 220., 330.));
    AActor* Foreign = SpawnAnchor<AActor>(ForeignWorld.World, FVector(900., 900., 900.));
    if (!Fog || !Explicit || !Foreign) return false;
    Fog->SetFogMaterial(Material);
    Fixture.World->WorldType = EWorldType::Editor;
    Fog->SetObserverActor(Explicit);
    TestEqual(TEXT("Editor actor selection never activates a gameplay observer"), Fog->EffectiveGameplayObserver.A, 0.f);
    Fixture.World->WorldType = EWorldType::Game;
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("An explicit observer in the actual game world supplies its world location"),
        Unpack(Fog->EffectiveGameplayObserver).Equals(Explicit->GetActorLocation(), .01)
        && Fog->EffectiveGameplayObserver.A == 1.f);
    Fog->SetObserverActor(Foreign);
    TestEqual(TEXT("A foreign-world observer is rejected"), Fog->EffectiveGameplayObserver.A, 0.f);

    APlayerController* PC = Fixture.World->SpawnActor<APlayerController>();
    APawn* Pawn = SpawnAnchor<APawn>(Fixture.World, FVector(400., 500., 600.));
    if (!PC || !Pawn) return false;
    FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(Fixture.World);
    UGameInstance* PreviousInstance = Context.OwningGameInstance;
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
    Context.OwningGameInstance = Instance;
    Fixture.World->SetGameInstance(Instance);
    Context.SetCurrentWorld(Fixture.World);
    ON_SCOPE_EXIT
    {
        if (IsValid(PC)) PC->Destroy();
        Instance->OnWorldChanged(Fixture.World, nullptr);
        Context.OwningGameInstance = PreviousInstance;
        Fixture.World->SetGameInstance(PreviousInstance);
    };
    ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine, NAME_None, RF_Transient);
    PC->SetPlayer(Player);
    Fixture.World->AddController(PC);
    PC->Possess(Pawn);
    Fog->SetObserverActor(nullptr);
    TestTrue(TEXT("No explicit override resolves the local possessed Pawn"),
        Fog->EffectiveGameplayObserver.A == 1.f
        && Unpack(Fog->EffectiveGameplayObserver).Equals(Pawn->GetActorLocation(), .01)
        && Fog->ObserverSource.Contains(Pawn->GetPathName()));
    PC->SetControlRotation(FRotator(0., 120., 0.));
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("Changing camera/control direction does not move the gameplay clear centre"),
        Unpack(Fog->EffectiveGameplayObserver).Equals(FVector(400., 500., 600.), .01));
    Pawn->SetActorLocation(FVector(-40., 90., 230.));
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("Pawn movement updates the clear centre independently of camera direction"),
        Unpack(ReadVector(*this, Fog->GetFogMID(), TEXT("GameplayObserver"))).Equals(Pawn->GetActorLocation(), .01));
    Fog->SetObserverActor(Foreign);
    TestTrue(TEXT("Foreign override cannot displace a valid local possessed Pawn"),
        Unpack(Fog->EffectiveGameplayObserver).Equals(Pawn->GetActorLocation(), .01));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogStage12LightingTest, "YogVision.Stage12.LightingAndPaletteReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogStage12LightingTest::RunTest(const FString&)
{
    using namespace Stage12FogTests;
    UMaterialInterface* Material = RequireMaterial(*this);
    if (!Material) return false;
    FWorldFixture Fixture, ForeignWorld;
    if (!Fixture.World || !ForeignWorld.World) return false;
    AYogStage12FogActor* Fog = Fixture.SpawnFog();
    APointLight* Local = Fixture.World->SpawnActor<APointLight>();
    APointLight* Foreign = ForeignWorld.World->SpawnActor<APointLight>();
    ADirectionalLight* Sun = Fixture.World->SpawnActor<ADirectionalLight>();
    if (!Fog || !Local || !Foreign || !Sun) return false;
    UPointLightComponent* Light = Cast<UPointLightComponent>(Local->GetLightComponent());
    UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
    if (!Light || !SunComponent) return false;
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensity(100.f);
    Light->SetAttenuationRadius(888.f);
    Light->SetVolumetricScatteringIntensity(.5f);
    Light->SetCastShadows(false);
    SunComponent->SetIntensity(10.f);
    SunComponent->SetVolumetricScatteringIntensity(0.f);
    Fog->SetFogMaterial(Material);
    UMaterialInstanceDynamic* MID = Fog->GetFogMID();
    if (!TestNotNull(TEXT("Test MID"), MID)) return false;
    Fog->RefreshMaterialParameters();
    TestEqual(TEXT("Only active local-world point/spot sources are selected"), Fog->LocalLightCount, 1);
    TestEqual(TEXT("Attenuation radius remains in centimetres"), ReadVector(*this, MID, TEXT("LightPosition0")).A, 888.f);
    TestTrue(TEXT("Scene direction source is identified"), Fog->DirectionalSource.Contains(SunComponent->GetPathName()));
    TestTrue(TEXT("A directional light with zero volumetric scattering emits no fog radiance"),
        Unpack(ReadVector(*this, MID, TEXT("DirectionalColour"))).IsNearlyZero());
    const FLinearColor Before = ReadVector(*this, MID, TEXT("LightColour0"));
    Light->SetIntensity(200.f);
    Fog->RefreshMaterialParameters();
    const FLinearColor After = ReadVector(*this, MID, TEXT("LightColour0"));
    TestTrue(TEXT("Actual scene light brightness changes fog input radiance"), After.R > Before.R && After.G > Before.G);
    TestEqual(TEXT("Fog refresh preserves authored light intensity"), Light->Intensity, 200.f);
    TestEqual(TEXT("Fog refresh preserves authored scattering"), Light->VolumetricScatteringIntensity, .5f);
    TestFalse(TEXT("Fog refresh does not enable scene shadows"), !!Light->CastShadows);
    Light->SetVisibility(false);
    Fog->RefreshMaterialParameters();
    TestEqual(TEXT("Hidden lights are removed"), Fog->LocalLightCount, 0);
    TestEqual(TEXT("Removed lights leave no stale radius"), ReadVector(*this, MID, TEXT("LightPosition0")).A, 0.f);
    TestEqual(TEXT("Removed lights leave no stale enable flag"), ReadVector(*this, MID, TEXT("LightColour0")).A, 0.f);

    UMaterialInterface* Original = Fog->PaletteMaterial;
    if (!TestNotNull(TEXT("Original artist palette resource"), Original)) return false;
    FLinearColor OriginalNear, OriginalFar;
    float OriginalMix = 0.f;
    if (!TestTrue(TEXT("Original MI supplies both colour parameters and its blend"),
        Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), OriginalNear)
        && Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), OriginalFar)
        && Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), OriginalMix))) return false;
    UMaterialInstanceDynamic* PaletteCopy = UMaterialInstanceDynamic::Create(Original, Fog);
    if (!PaletteCopy) return false;
    const FLinearColor Near(.2f, .3f, .4f, 1.f), Far(.6f, .5f, .4f, 1.f);
    PaletteCopy->SetVectorParameterValue(TEXT("Fog Color 01"), Near);
    PaletteCopy->SetVectorParameterValue(TEXT("Fog Color 02"), Far);
    PaletteCopy->SetScalarParameterValue(TEXT("lerp_fog"), .37f);
    Fog->PaletteMaterial = PaletteCopy;
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("Artist near/far parameters are read through inherited MI values"),
        ReadVector(*this, MID, TEXT("NearColour")).Equals(Near, .0001f)
        && ReadVector(*this, MID, TEXT("FarColour")).Equals(Far, .0001f));
    TestTrue(TEXT("Artist blend is preserved"), FMath::IsNearlyEqual(ReadScalar(*this, MID, TEXT("PaletteMix")), .37f));
    FLinearColor UnchangedNear, UnchangedFar;
    float UnchangedMix = 0.f;
    Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), UnchangedNear);
    Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), UnchangedFar);
    Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), UnchangedMix);
    TestTrue(TEXT("Reading a transient palette variant never alters the original MI"),
        OriginalNear.Equals(UnchangedNear, .0001f) && OriginalFar.Equals(UnchangedFar, .0001f)
        && FMath::IsNearlyEqual(OriginalMix, UnchangedMix));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogStage12AnimationClockTest, "YogVision.Stage12.AnimationClockEditorAndRuntime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogStage12AnimationClockTest::RunTest(const FString&)
{
    using namespace Stage12FogTests;
    FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Temporary animation test world"), Fixture.World)) return false;
    AYogStage12FogActor* Fog = Fixture.SpawnFog();
    if (!TestNotNull(TEXT("Animation test actor"), Fog)) return false;
    Fixture.World->WorldType = EWorldType::Editor;
    const FTransform Placement(FRotator(17., 53., -8.), FVector(120., -600., 280.), FVector(2., 1., 3.));
    Fog->SetActorTransform(Placement);
    Fog->ResetFogAnimation();
    Fog->bAnimateInEditor = false;
    Fog->Tick(5.f);
    TestEqual(TEXT("Editor preview toggle pauses the independent clock"), Fog->FogTimeSeconds, 0.f);
    TestTrue(TEXT("Artist readback identifies paused editor preview"), Fog->AnimationStatus.Contains(TEXT("editor paused")));
    Fog->bAnimateInEditor = true;
    Fog->Tick(.5f);
    TestEqual(TEXT("Editor viewport ticks drive fog without PIE"), Fog->FogTimeSeconds, .5f);
    Fog->AnimationSpeed = 2.f;
    Fog->Tick(.25f);
    TestEqual(TEXT("Changing speed affects only subsequent deltas, with no time jump"), Fog->FogTimeSeconds, 1.f);
    Fog->bAnimateFog = false;
    Fog->Tick(120.f);
    TestEqual(TEXT("Pausing freezes the current shape time"), Fog->FogTimeSeconds, 1.f);
    Fog->bAnimateFog = true;
    Fog->AnimationSpeed = .5f;
    Fog->Tick(2.f);
    TestEqual(TEXT("Resuming does not include elapsed paused time"), Fog->FogTimeSeconds, 2.f);
    Fog->bAnimateInEditor = false;
    Fixture.World->WorldType = EWorldType::Game;
    Fog->Tick(2.f);
    TestEqual(TEXT("Runtime advances independently of the editor preview toggle"), Fog->FogTimeSeconds, 3.f);
    TestTrue(TEXT("Artist readback identifies active runtime"), Fog->AnimationStatus.Contains(TEXT("runtime running")));
    Fog->AnimationSpeed = 0.f;
    Fog->Tick(5.f);
    TestEqual(TEXT("Zero playback speed freezes the clock"), Fog->FogTimeSeconds, 3.f);
    Fog->AnimationSpeed = 1.f;
    Fog->Tick(-1.f);
    Fog->Tick(std::numeric_limits<float>::quiet_NaN());
    Fog->Tick(std::numeric_limits<float>::infinity());
    TestEqual(TEXT("Invalid or reversed deltas cannot contaminate or rewind fog time"), Fog->FogTimeSeconds, 3.f);
    TestTrue(TEXT("Animated density never moves or scales the placed container"), Fog->GetActorTransform().Equals(Placement, .0001));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogStage12CaptureClockTest, "YogVision.Stage12.DeterministicCaptureClock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogStage12CaptureClockTest::RunTest(const FString&)
{
    using namespace Stage12FogTests;
    FWorldFixture Fixture;
    if (!Fixture.World) return false;
    AYogStage12FogActor* Fog = Fixture.SpawnFog();
    if (!TestNotNull(TEXT("Capture test actor"), Fog)) return false;
    Fog->ResetFogAnimation();
    Fog->Tick(2.f);
    Fog->SetFogTimeForCapture(100.f);
    Fog->Tick(30.f);
    Fog->RefreshMaterialParameters();
    TestEqual(TEXT("Capture time survives ticks and repeated material refreshes"), Fog->FogTimeSeconds, 100.f);
    TestTrue(TEXT("A capture override is explicit in artist readback"), Fog->AnimationStatus.Contains(TEXT("capture time override")));
    Fog->ClearFogTimeOverride();
    TestEqual(TEXT("Clearing capture override restores the pre-capture elapsed time"), Fog->FogTimeSeconds, 2.f);
    Fog->Tick(.5f);
    TestEqual(TEXT("Live animation resumes without including time spent capturing"), Fog->FogTimeSeconds, 2.5f);
    Fog->SetFogTimeForCapture(std::numeric_limits<float>::quiet_NaN());
    TestEqual(TEXT("Invalid capture input resolves to a finite deterministic time"), Fog->FogTimeSeconds, 0.f);
    Fog->ResetFogAnimation();
    Fog->Tick(.25f);
    TestEqual(TEXT("Reset clears both accumulated time and the capture override"), Fog->FogTimeSeconds, .25f);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogStage12ShapeTextureOverrideTest, "YogVision.Stage12.OptionalVolumeTextureIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogStage12ShapeTextureOverrideTest::RunTest(const FString&)
{
    using namespace Stage12FogTests;
    UMaterialInterface* Material = RequireMaterial(*this);
    if (!Material) return false;
    FWorldFixture Fixture;
    if (!Fixture.World) return false;
    AYogStage12FogActor* Fog = Fixture.SpawnFog();
    if (!Fog) return false;
    Fog->SetFogMaterial(Material);
    UMaterialInstanceDynamic* MID = Fog->GetFogMID();
    if (!TestNotNull(TEXT("Shape test MID"), MID)) return false;
    UTexture* ParentShape = nullptr;
    UTexture* ParentField = nullptr;
    if (!TestTrue(TEXT("Saved integration material provides a three-dimensional shape texture"),
        Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityShapeTexture")), ParentShape)
        && IsValid(Cast<UVolumeTexture>(ParentShape)))) return false;
    if (!TestTrue(TEXT("Saved integration material provides an external field default"),
        Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("FieldTexture")), ParentField))) return false;
    UVolumeTexture* Shape = NewObject<UVolumeTexture>(Fog, NAME_None, RF_Transient);
    UVolumeTexture* Field = NewObject<UVolumeTexture>(Fog, NAME_None, RF_Transient);
    Fog->DensityShapeTexture = Shape;
    Fog->ExternalFieldTexture = Field;
    Fog->RefreshMaterialParameters();
    UTexture* Actual = nullptr;
    TestTrue(TEXT("Explicit volume shape reaches the density sampler"),
        MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityShapeTexture")), Actual) && Actual == Shape);
    Fog->DensityShapeTexture = nullptr;
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("Clearing shape selection restores the saved material's default"),
        MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityShapeTexture")), Actual) && Actual == ParentShape);
    TestTrue(TEXT("Clearing shape selection preserves the separate external-field override"),
        MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("FieldTexture")), Actual) && Actual == Field);
    Fog->DensityShapeTexture = Shape;
    Fog->ExternalFieldTexture = nullptr;
    Fog->RefreshMaterialParameters();
    TestTrue(TEXT("Clearing external field restores its own material default"),
        MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("FieldTexture")), Actual) && Actual == ParentField);
    TestTrue(TEXT("Clearing external field preserves the separate density-shape override"),
        MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityShapeTexture")), Actual) && Actual == Shape);
    Fog->ShapeStretch = FVector(std::numeric_limits<double>::quiet_NaN(), -5., 0.);
    Fog->FlowVelocity = FVector(std::numeric_limits<double>::infinity(), -20., 30.);
    Fog->ShapeEdgeSoftness = 0.f;
    Fog->FlowPeriod = 0.f;
    Fog->RefreshMaterialParameters();
    const FVector Stretch = Unpack(ReadVector(*this, MID, TEXT("ShapeStretch")));
    const FVector Velocity = Unpack(ReadVector(*this, MID, TEXT("FlowVelocity")));
    TestTrue(TEXT("Invalid shape axes never produce zero divisions or NaN density"),
        !Stretch.ContainsNaN() && Stretch.X > 0. && Stretch.Y > 0. && Stretch.Z > 0.);
    TestTrue(TEXT("Invalid velocity components do not contaminate world-space sampling"), !Velocity.ContainsNaN());
    TestTrue(TEXT("Shape feather and animation period remain positive"),
        ReadScalar(*this, MID, TEXT("ShapeEdgeSoftness")) > 0.f && ReadScalar(*this, MID, TEXT("FlowPeriod")) > 0.f);
    return !HasAnyErrors();
}

#endif
