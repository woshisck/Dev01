#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "YogAirFogVolume.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "UObject/UObjectIterator.h"

namespace
{
const TCHAR* const AirFogTestCVarNames[] = {
    TEXT("r.Fog"), TEXT("r.VolumetricFog"), TEXT("r.VolumetricFog.GridPixelSize"),
    TEXT("r.VolumetricFog.GridSizeZ"), TEXT("r.VolumetricFog.HistoryWeight"), TEXT("r.VolumetricFog.LightSoftFading")
};

struct FAirFogTestCVarState
{
    FString Name;
    FString Value;
    EConsoleVariableFlags Priority = ECVF_SetByConstructor;
};

TArray<FAirFogTestCVarState> CaptureAirFogConsole()
{
    TArray<FAirFogTestCVarState> Result;
    for (const TCHAR* Name : AirFogTestCVarNames)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            FAirFogTestCVarState& State = Result.AddDefaulted_GetRef();
            State.Name = Name;
            State.Value = Variable->GetString();
            State.Priority = static_cast<EConsoleVariableFlags>(Variable->GetFlags() & ECVF_SetByMask);
        }
    }
    return Result;
}

void CheckAirFogConsole(FAutomationTestBase& Test, const TArray<FAirFogTestCVarState>& Expected, const TCHAR* Context)
{
    for (const FAirFogTestCVarState& State : Expected)
    {
        IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*State.Name);
        if (!Test.TestNotNull(FString::Printf(TEXT("%s: %s exists"), Context, *State.Name), Variable)) continue;
        Test.TestEqual(FString::Printf(TEXT("%s: %s value restored"), Context, *State.Name), Variable->GetString(), State.Value);
        Test.TestEqual(FString::Printf(TEXT("%s: %s original SetBy restored"), Context, *State.Name),
            static_cast<uint32>(Variable->GetFlags() & ECVF_SetByMask), static_cast<uint32>(State.Priority));
    }
}

// This guard removes only Console values deliberately authored by this test. Actor leases
// remain responsible for their own temporary priorities and are checked before fixture cleanup.
struct FAirFogTestConsoleGuard
{
    TArray<FAirFogTestCVarState> Opening = CaptureAirFogConsole();
    TSet<FString> AuthoredConsoleValues;

    void SetConsole(const TCHAR* Name, int32 Value)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            AuthoredConsoleValues.Add(Name);
            Variable->Set(Value, ECVF_SetByConsole);
        }
    }

    void Restore()
    {
        for (const FAirFogTestCVarState& State : Opening)
        {
            if (!AuthoredConsoleValues.Contains(State.Name)) continue;
            if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*State.Name))
            {
                if (State.Priority == ECVF_SetByConsole) Variable->Set(*State.Value, ECVF_SetByConsole);
                else Variable->Unset(ECVF_SetByConsole);
            }
        }
        AuthoredConsoleValues.Reset();
        IConsoleManager::Get().CallAllConsoleVariableSinks();
    }

    ~FAirFogTestConsoleGuard() { Restore(); }
};

struct FAirFogTestWorld
{
    UWorld* World = nullptr;

    explicit FAirFogTestWorld(const TCHAR* Name)
    {
        if (!GEngine) return;
        const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
            .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
            .ShouldSimulatePhysics(false).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), Name), nullptr, true,
            ERHIFeatureLevel::Num, &Init);
        if (World) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }

    ~FAirFogTestWorld()
    {
        if (!World) return;
        for (TActorIterator<AYogAirFogVolume> It(World); It; ++It) It->SetAirFogEnabled(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
};

AActor* SpawnAirFogTestAnchor(UWorld* World, FVector Position = FVector::ZeroVector)
{
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    AActor* Actor = World->SpawnActor<AActor>(Spawn);
    if (!Actor) return nullptr;
    USceneComponent* Root = NewObject<USceneComponent>(Actor);
    Root->SetMobility(EComponentMobility::Movable);
    Actor->SetRootComponent(Root);
    Actor->AddInstanceComponent(Root);
    Root->RegisterComponent();
    Actor->SetActorLocation(Position);
    return Actor;
}

AYogAirFogVolume* SpawnAirFogTestVolume(UWorld* World)
{
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    AYogAirFogVolume* Actor = World->SpawnActor<AYogAirFogVolume>(Spawn);
    if (Actor)
    {
        Actor->bAutoActivate = false;
        Actor->bFollowPlayer = false;
    }
    return Actor;
}

UExponentialHeightFogComponent* SpawnAirFogTestExistingFog(UWorld* World, bool bVisible)
{
    AActor* Actor = SpawnAirFogTestAnchor(World);
    if (!Actor) return nullptr;
    UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(Actor);
    Actor->AddInstanceComponent(Fog);
    Fog->SetupAttachment(Actor->GetRootComponent());
    Fog->SetVisibility(bVisible, false);
    Fog->RegisterComponent();
    return Fog;
}

int32 CountAirFogTestPostProcess(UWorld* World)
{
    int32 Count = 0;
    for (TActorIterator<APostProcessVolume> It(World); It; ++It) ++Count;
    return Count;
}

bool AirFogRuntimeResourcesReady(FAutomationTestBase& Test)
{
    // Do not change an unrelated running preview's process-wide console lease.
    for (TObjectIterator<AYogAirFogVolume> It; It; ++It)
    {
        if (It->GetWorld() && It->IsAirFogEnabled())
        {
            Test.AddWarning(TEXT("AirFog lifecycle test skipped: another live air-fog preview is enabled. Close that preview before running this test."));
            return false;
        }
    }
    if (!FPackageName::DoesPackageExist(TEXT("/YogVision/AirFog/M_AirFogDensity"))
        || !FPackageName::DoesPackageExist(TEXT("/YogVision/AirFog/M_AirFogFinish")))
    {
        Test.AddWarning(TEXT("AirFog lifecycle test skipped: prepare and save the native AirFog material assets first. No shared material was modified."));
        return false;
    }
    UMaterialInterface* Density = LoadObject<UMaterialInterface>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity"));
    UMaterialInterface* Finish = LoadObject<UMaterialInterface>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish"));
    const bool bDensityReady = Density && Density->GetMaterial() && Density->GetMaterial()->MaterialDomain == MD_Volume
        && Density->GetMaterial()->GetShadingModels().HasShadingModel(MSM_DefaultLit);
    const bool bFinishReady = Finish && Finish->GetMaterial() && Finish->GetMaterial()->MaterialDomain == MD_PostProcess;
    return Test.TestTrue(TEXT("Existing density resource supports native volume extinction"), bDensityReady)
        && Test.TestTrue(TEXT("Existing finish resource is a post-process material"), bFinishReady);
}

void CheckAirFogFocus(FAutomationTestBase& Test, UMaterialInstanceDynamic* MID, const FVector& Expected, const TCHAR* Context)
{
    if (!Test.TestNotNull(FString::Printf(TEXT("%s: density MID"), Context), MID)) return;
    const FLinearColor Actual = MID->K2_GetVectorParameterValue(TEXT("Focus"));
    Test.TestTrue(FString::Printf(TEXT("%s: Focus follows expected position"), Context),
        FVector(Actual.R, Actual.G, Actual.B).Equals(Expected, .01f));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogFailOpenTest, "YogVision.AirFog.FailOpen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogFailOpenTest::RunTest(const FString& Parameters)
{
    const TArray<FAirFogTestCVarState> Opening = CaptureAirFogConsole();
    FAirFogTestWorld Fixture(TEXT("AirFogFailOpen"));
    if (!TestNotNull(TEXT("Temporary World"), Fixture.World)) return false;
    UExponentialHeightFogComponent* ExistingFog = SpawnAirFogTestExistingFog(Fixture.World, true);
    UExponentialHeightFogComponent* AuthoredHidden = SpawnAirFogTestExistingFog(Fixture.World, false);
    AYogAirFogVolume* AirFog = SpawnAirFogTestVolume(Fixture.World);
    if (!TestNotNull(TEXT("Air-fog Actor"), AirFog)
        || !TestNotNull(TEXT("Existing fog"), ExistingFog) || !TestNotNull(TEXT("Authored-hidden fog"), AuthoredHidden)) return false;
    const int32 BeforePostProcessCount = CountAirFogTestPostProcess(Fixture.World);
    // A temporary component-only fault exercises resource failure without modifying shared assets.
    AirFog->VolumeBounds->SetStaticMesh(nullptr);
    TestFalse(TEXT("Incomplete resources refuse activation"), AirFog->SetAirFogEnabled(true));
    TestFalse(TEXT("Missing resources never expose the opaque Cube"), AirFog->VolumeBounds->GetVisibleFlag());
    TestFalse(TEXT("Missing resources leave the native carrier hidden"), AirFog->Carrier->GetVisibleFlag());
    TestNull(TEXT("Missing resources do not create a density MID"), AirFog->GetDensityMID());
    TestTrue(TEXT("Existing visible fog remains visible"), ExistingFog->GetVisibleFlag());
    TestFalse(TEXT("Authored-hidden fog stays hidden"), AuthoredHidden->GetVisibleFlag());
    TestEqual(TEXT("Missing resources add no finish volume"), CountAirFogTestPostProcess(Fixture.World), BeforePostProcessCount);
    CheckAirFogConsole(*this, Opening, TEXT("Fail-open"));
    AirFog->SetAirFogEnabled(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogWorldLeaseTest, "YogVision.AirFog.WorldLeaseLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogWorldLeaseTest::RunTest(const FString& Parameters)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    const TArray<FAirFogTestCVarState> Opening = CaptureAirFogConsole();
    FAirFogTestWorld First(TEXT("AirFogFirstWorld"));
    FAirFogTestWorld Second(TEXT("AirFogSecondWorld"));
    if (!TestNotNull(TEXT("First World"), First.World) || !TestNotNull(TEXT("Second World"), Second.World)) return false;
    UExponentialHeightFogComponent* FirstFog = SpawnAirFogTestExistingFog(First.World, true);
    UExponentialHeightFogComponent* FirstHidden = SpawnAirFogTestExistingFog(First.World, false);
    UExponentialHeightFogComponent* SecondFog = SpawnAirFogTestExistingFog(Second.World, true);
    AYogAirFogVolume* FirstAir = SpawnAirFogTestVolume(First.World);
    AYogAirFogVolume* Rival = SpawnAirFogTestVolume(First.World);
    AYogAirFogVolume* SecondAir = SpawnAirFogTestVolume(Second.World);
    if (!TestNotNull(TEXT("First fog"), FirstFog) || !TestNotNull(TEXT("Hidden fog"), FirstHidden)
        || !TestNotNull(TEXT("Second fog"), SecondFog) || !TestNotNull(TEXT("First air"), FirstAir)
        || !TestNotNull(TEXT("Rival"), Rival) || !TestNotNull(TEXT("Second air"), SecondAir)) return false;
    if (!TestTrue(TEXT("First World can activate"), FirstAir->SetAirFogEnabled(true))) return false;
    TestFalse(TEXT("First World's old fog is temporarily hidden"), FirstFog->GetVisibleFlag());
    TestTrue(TEXT("Activating first World does not hide second World's fog"), SecondFog->GetVisibleFlag());
    TestFalse(TEXT("Same-World second active owner is refused"), Rival->SetAirFogEnabled(true));
    TestTrue(TEXT("Refused owner does not disable the first carrier"), FirstAir->Carrier->GetVisibleFlag());
    Rival->SetAirFogEnabled(false);
    if (!TestTrue(TEXT("Another World can share the global lease"), SecondAir->SetAirFogEnabled(true))) return false;
    TestFalse(TEXT("Second World's old fog is now hidden locally"), SecondFog->GetVisibleFlag());
    FirstAir->Destroy();
    TestTrue(TEXT("Destroying first owner restores only its own visible fog"), FirstFog->GetVisibleFlag());
    TestFalse(TEXT("Destroying first owner preserves authored-hidden fog"), FirstHidden->GetVisibleFlag());
    TestFalse(TEXT("First World's teardown does not restore second World's old fog"), SecondFog->GetVisibleFlag());
    TestTrue(TEXT("Second carrier remains active"), SecondAir->Carrier->GetVisibleFlag());
    TestEqual(TEXT("Shared r.Fog remains enabled until last owner leaves"), IConsoleManager::Get().FindConsoleVariable(TEXT("r.Fog"))->GetInt(), 1);
    TestEqual(TEXT("Shared volumetric fog remains enabled until last owner leaves"), IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"))->GetInt(), 1);
    SecondAir->Destroy();
    TestTrue(TEXT("Last owner destruction restores its World's fog"), SecondFog->GetVisibleFlag());
    TestEqual(TEXT("No transient finish volumes remain in first World"), CountAirFogTestPostProcess(First.World), 0);
    TestEqual(TEXT("No transient finish volumes remain in second World"), CountAirFogTestPostProcess(Second.World), 0);
    CheckAirFogConsole(*this, Opening, TEXT("Last lease released"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogConsolePreservationTest, "YogVision.AirFog.ConsolePreservation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogConsolePreservationTest::RunTest(const FString& Parameters)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    FAirFogTestConsoleGuard Console;
    FAirFogTestWorld Fixture(TEXT("AirFogConsoleWorld"));
    if (!TestNotNull(TEXT("Temporary World"), Fixture.World)) return false;
    AYogAirFogVolume* AirFog = SpawnAirFogTestVolume(Fixture.World);
    if (!TestNotNull(TEXT("Air-fog Actor"), AirFog) || !TestTrue(TEXT("Air fog activates"), AirFog->SetAirFogEnabled(true))) return false;
    Console.SetConsole(TEXT("r.Fog"), 0);
    Console.SetConsole(TEXT("r.VolumetricFog.GridPixelSize"), 23);
    FYogAirFogParameters NewValues = AirFog->GetParameters();
    NewValues.GridPixelSize = 32;
    AirFog->ApplyParameters(NewValues);
    IConsoleVariable* Fog = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Fog"));
    IConsoleVariable* Grid = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.GridPixelSize"));
    TestEqual(TEXT("Applying artist settings respects newer console fog switch"), Fog->GetInt(), 0);
    TestEqual(TEXT("Applying artist settings respects newer console quality"), Grid->GetInt(), 23);
    AirFog->SetAirFogEnabled(false);
    TestEqual(TEXT("Disabling preserves the user's newer fog value"), Fog->GetInt(), 0);
    TestEqual(TEXT("Disabling preserves the user's newer quality value"), Grid->GetInt(), 23);
    TestEqual(TEXT("New console SetBy remains Console"), static_cast<uint32>(Grid->GetFlags() & ECVF_SetByMask), static_cast<uint32>(ECVF_SetByConsole));
    // Exercise the special non-array Console snapshot path, without overwriting new user intent.
    if (!TestTrue(TEXT("A new activation can start from existing Console settings"), AirFog->SetAirFogEnabled(true))) return false;
    TestEqual(TEXT("Activation temporarily enables an existing Console fog setting"), Fog->GetInt(), 1);
    TestEqual(TEXT("Activation temporarily applies current quality"), Grid->GetInt(), 32);
    AirFog->SetAirFogEnabled(false);
    TestEqual(TEXT("Untouched Console snapshot restores original fog"), Fog->GetInt(), 0);
    TestEqual(TEXT("Untouched Console snapshot restores original quality"), Grid->GetInt(), 23);
    Console.Restore();
    CheckAirFogConsole(*this, Console.Opening, TEXT("Console fixture restored"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogFocusSaveTest, "YogVision.AirFog.CameraAndSave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogFocusSaveTest::RunTest(const FString& Parameters)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    const TArray<FAirFogTestCVarState> Opening = CaptureAirFogConsole();
    FAirFogTestWorld Fixture(TEXT("AirFogFocusWorld"));
    if (!TestNotNull(TEXT("Temporary World"), Fixture.World)) return false;
    UExponentialHeightFogComponent* ExistingFog = SpawnAirFogTestExistingFog(Fixture.World, true);
    AActor* Focus = SpawnAirFogTestAnchor(Fixture.World, FVector(325, -140, 80));
    AYogAirFogVolume* AirFog = SpawnAirFogTestVolume(Fixture.World);
    if (!TestNotNull(TEXT("Existing fog"), ExistingFog) || !TestNotNull(TEXT("Focus Actor"), Focus)
        || !TestNotNull(TEXT("Air-fog Actor"), AirFog)) return false;
    FVector CameraPosition(1230, -450, 1730);
    Fixture.World->ViewLocationsRenderedLastFrame = { CameraPosition };
    AirFog->SetFocusActor(Focus);
    if (!TestTrue(TEXT("Air fog activates with existing materials"), AirFog->SetAirFogEnabled(true))) return false;
    CheckAirFogFocus(*this, AirFog->GetDensityMID(), CameraPosition, TEXT("Rendered camera wins over legacy actor focus"));
    TestTrue(TEXT("Activation immediately centres volume bounds on the camera"), AirFog->GetActorLocation().Equals(CameraPosition, .01f));
    Focus->SetActorLocation(FVector(510, 95, 120));
    AirFog->Tick(.1f);
    CheckAirFogFocus(*this, AirFog->GetDensityMID(), CameraPosition, TEXT("Moving legacy focus cannot move the camera fog"));
    CameraPosition += FVector(-70, 180, 35);
    Fixture.World->ViewLocationsRenderedLastFrame[0] = CameraPosition;
    AirFog->Tick(.1f);
    CheckAirFogFocus(*this, AirFog->GetDensityMID(), CameraPosition, TEXT("Moving rendered camera"));
    TestTrue(TEXT("Volume bounds follow the camera"), AirFog->GetActorLocation().Equals(CameraPosition, .01f));
    FYogAirFogParameters Values = AirFog->GetParameters();
    Values.Density = .22f;
    AirFog->ApplyParameters(Values);
    TestTrue(TEXT("Artist density reaches the native volume MID"), FMath::IsNearlyEqual(AirFog->GetDensityMID()->K2_GetScalarParameterValue(TEXT("Density")), .22f));
    AirFog->GetDensityMID()->SetScalarParameterValue(TEXT("TimeSeconds"), -999.f);
    AirFog->Tick(.1f);
    TestTrue(TEXT("Each tick refreshes exact TimeSeconds parameter from World time"), FMath::IsNearlyEqual(
        AirFog->GetDensityMID()->K2_GetScalarParameterValue(TEXT("TimeSeconds")), static_cast<float>(Fixture.World->GetTimeSeconds()), .0001f));
    TestNotNull(TEXT("Active air fog exposes its per-view finish template"), AirFog->GetFinishMID());
    TestEqual(TEXT("Per-view finish never creates an extra post-process Actor"), CountAirFogTestPostProcess(Fixture.World), 0);
    AirFog->SuspendForSave(true);
    AirFog->SuspendForSave(true);
    TestTrue(TEXT("Saving preserves requested enabled state"), AirFog->IsAirFogEnabled());
    TestFalse(TEXT("Saving hides native carrier"), AirFog->Carrier->GetVisibleFlag());
    TestFalse(TEXT("Saving hides the density bounds"), AirFog->VolumeBounds->GetVisibleFlag());
    TestTrue(TEXT("Saving restores original height-fog visibility"), ExistingFog->GetVisibleFlag());
    TestNull(TEXT("Saving unregisters the per-view finish template"), AirFog->GetFinishMID());
    TestEqual(TEXT("Saving leaves no temporary finish Actor"), CountAirFogTestPostProcess(Fixture.World), 0);
    CheckAirFogConsole(*this, Opening, TEXT("Save suspension"));
    const FVector SavedFocus(88, -44, 55);
    AirFog->SetFocusPosition(SavedFocus);
    CameraPosition += FVector(25, 50, -10);
    Fixture.World->ViewLocationsRenderedLastFrame[0] = CameraPosition;
    Values.Density = .31f;
    AirFog->ApplyParameters(Values);
    AirFog->SuspendForSave(false);
    TestFalse(TEXT("Nested save still suspends rendering after one completion"), AirFog->Carrier->GetVisibleFlag());
    AirFog->SuspendForSave(false);
    TestTrue(TEXT("Last save completion resumes carrier"), AirFog->Carrier->GetVisibleFlag());
    TestNotNull(TEXT("Last save completion resumes the per-view finish template"), AirFog->GetFinishMID());
    TestFalse(TEXT("Resume locally suppresses original fog again"), ExistingFog->GetVisibleFlag());
    CheckAirFogFocus(*this, AirFog->GetDensityMID(), CameraPosition, TEXT("Save resumes current camera rather than legacy explicit focus"));
    TestTrue(TEXT("Save preserves updated artist density"), FMath::IsNearlyEqual(AirFog->GetDensityMID()->K2_GetScalarParameterValue(TEXT("Density")), .31f));
    AirFog->SetAirFogEnabled(false);
    // Old reflected focus data remains serializable, but cannot override the camera
    // after template duplication or a different Actor spawn transform.
    FActorSpawnParameters CopySpawn;
    CopySpawn.ObjectFlags = RF_Transient;
    CopySpawn.Template = AirFog;
    AYogAirFogVolume* Copy = Fixture.World->SpawnActor<AYogAirFogVolume>(AYogAirFogVolume::StaticClass(),
        FVector(7000, 8000, 9000), FRotator::ZeroRotator, CopySpawn);
    if (!TestNotNull(TEXT("Reflected-state Actor copy"), Copy)) return false;
    Copy->bAutoActivate = false;
    Copy->bFollowPlayer = false;
    if (!TestTrue(TEXT("Copied Actor can activate"), Copy->SetAirFogEnabled(true))) return false;
    TestTrue(TEXT("Copied Actor owns its density MID independently of the template"), Copy->GetDensityMID()->GetOuter() == Copy);
    TestTrue(TEXT("Copied Actor owns its finish MID independently of the template"), Copy->GetFinishMID() && Copy->GetFinishMID()->GetOuter() == Copy);
    Copy->Tick(.1f);
    TestTrue(TEXT("Legacy explicit position remains available for compatibility"), Copy->FocusPosition.Equals(SavedFocus, .01f));
    CheckAirFogFocus(*this, Copy->GetDensityMID(), CameraPosition, TEXT("Copied Actor resolves the rendered camera"));
    TestTrue(TEXT("Copied bounds use camera rather than saved legacy focus or spawn location"), Copy->GetActorLocation().Equals(CameraPosition, .01f));
    FActorSpawnParameters CameraSpawn;
    CameraSpawn.ObjectFlags = RF_Transient;
    APlayerController* PC = Fixture.World->SpawnActor<APlayerController>(CameraSpawn);
    if (!TestNotNull(TEXT("A local player controller exists"), PC)) return false;
    // The source engine requires a real ULocalPlayer when no NetDriver exists.
    // Supply that identity through SetPlayer, without starting a gameplay map,
    // initializing project GameInstance subsystems, or forcing the local flag.
    FWorldContext& CameraContext = GEngine->GetWorldContextFromWorldChecked(Fixture.World);
    UGameInstance* OriginalWorldInstance = Fixture.World->GetGameInstance();
    UGameInstance* OriginalContextInstance = CameraContext.OwningGameInstance;
    UGameInstance* CameraInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
    CameraContext.OwningGameInstance = CameraInstance;
    Fixture.World->SetGameInstance(CameraInstance);
    CameraContext.SetCurrentWorld(Fixture.World);
    ON_SCOPE_EXIT
    {
        if (IsValid(PC)) PC->Destroy();
        CameraInstance->OnWorldChanged(Fixture.World, nullptr);
        CameraContext.OwningGameInstance = OriginalContextInstance;
        Fixture.World->SetGameInstance(OriginalWorldInstance);
    };
    // LocalPlayer.h declares Within=Engine; GameInstance::CreateLocalPlayer
    // likewise constructs it under GetEngine(), not under the game instance.
    if (!TestTrue(TEXT("The engine satisfies LocalPlayer's declared ClassWithin"),
        GEngine->IsA(ULocalPlayer::StaticClass()->ClassWithin))) return false;
    ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, NAME_None, RF_Transient);
    PC->SetPlayer(LocalPlayer);
    // This isolated World deliberately does not initialize actors for play. UE only
    // calls Controller::PostInitializeComponents (and AddController) after that
    // boundary, so register this fixture controller explicitly, as gameplay does.
    Fixture.World->AddController(PC);
    if (!TestEqual(TEXT("The fixture game instance resolves its isolated World"), CameraInstance->GetWorld(), Fixture.World)
        || !TestEqual(TEXT("The fixture controller resolves the isolated game instance through its World"), PC->GetGameInstance(), CameraInstance)
        || !TestTrue(TEXT("The local player's Outer is the engine as required by ClassWithin"), LocalPlayer->GetOuter() == GEngine)
        || !TestEqual(TEXT("The fixture controller has a real local player"), PC->GetLocalPlayer(), LocalPlayer)
        || !TestEqual(TEXT("The local player points back to its controller"), LocalPlayer->PlayerController.Get(), PC)
        || !TestEqual(TEXT("The standalone fixture controller has authority"), PC->GetLocalRole(), ROLE_Authority)
        || !TestEqual(TEXT("The fixture camera controller is in the World's runtime controller list"), Fixture.World->GetFirstPlayerController(), PC)
        || !TestTrue(TEXT("The fixture controller is local"), PC->IsLocalController())) return false;
    if (!PC->PlayerCameraManager) PC->PlayerCameraManager = Fixture.World->SpawnActor<APlayerCameraManager>(CameraSpawn);
    if (!TestNotNull(TEXT("A native camera manager exists"), PC->PlayerCameraManager.Get())) return false;
    // Advance only this fixture's clock, then populate the native final-view cache.
    Fixture.World->TimeSeconds = 1.25;
    FMinimalViewInfo View;
    View.Location = FVector(750, -210, 1950);
    View.Rotation = FRotator(-35, -40, 0);
    PC->PlayerCameraManager->FillCameraCache(View);
    FVector NativeCameraPosition;
    FRotator NativeCameraRotation;
    PC->GetPlayerViewPoint(NativeCameraPosition, NativeCameraRotation);
    if (!TestTrue(TEXT("The fixture has a populated final camera cache"), PC->PlayerCameraManager->GetCameraCacheTime() > 0.f)
        || !TestTrue(TEXT("The native game-camera API returns the supplied final view"), NativeCameraPosition.Equals(View.Location, .01f))) return false;
    TestNull(TEXT("The final camera fixture does not require a Pawn"), PC->GetPawn());
    Copy->Tick(.1f);
    CheckAirFogFocus(*this, Copy->GetDensityMID(), View.Location, TEXT("Local final camera cache takes precedence over last rendered locations"));
    TestTrue(TEXT("Volume bounds follow the final game camera without requiring a Pawn"), Copy->GetActorLocation().Equals(View.Location, .01f));
    PC->Destroy();
    Copy->Destroy();
    TestTrue(TEXT("Copied Actor destruction restores scene fog"), ExistingFog->GetVisibleFlag());
    TestEqual(TEXT("Copied Actor destruction leaves no finish volume"), CountAirFogTestPostProcess(Fixture.World), 0);
    CheckAirFogConsole(*this, Opening, TEXT("Focus/save/copy lifecycle finished"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogArtistPaletteTest, "YogVision.AirFog.ArtistPalette", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogArtistPaletteTest::RunTest(const FString&)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    UMaterialInterface* Compatible = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/YogVision/AirFog/MI_AirFogArtistCompat.MI_AirFogArtistCompat"));
    if (!TestNotNull(TEXT("The saved compatible artist palette is available"), Compatible)) return false;
    FLinearColor OriginalNear;
    FLinearColor OriginalFar;
    float OriginalMix = 0.f;
    if (!TestTrue(TEXT("Compatible palette exposes the effective near colour"), Compatible->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), OriginalNear))
        || !TestTrue(TEXT("Compatible palette exposes the effective far colour"), Compatible->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), OriginalFar))
        || !TestTrue(TEXT("Compatible palette exposes the effective blend"), Compatible->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), OriginalMix))) return false;
    FAirFogTestWorld Fixture(TEXT("AirFogArtistPaletteWorld"));
    if (!TestNotNull(TEXT("A temporary World exists"), Fixture.World)) return false;
    AYogAirFogVolume* Air = SpawnAirFogTestVolume(Fixture.World);
    if (!TestNotNull(TEXT("Air-fog Actor exists"), Air) || !TestTrue(TEXT("Prepared air fog activates"), Air->SetAirFogEnabled(true))) return false;
    const FYogAirFogParameters OriginalParameters = Air->GetParameters();
    UMaterialInstanceDynamic* Palette = UMaterialInstanceDynamic::Create(Compatible, Air);
    if (!TestNotNull(TEXT("An isolated editable palette instance exists"), Palette)) return false;
    const FLinearColor Near(.21f, .32f, .43f, 1.f);
    Palette->SetVectorParameterValue(TEXT("Fog Color 01"), Near);
    Palette->SetScalarParameterValue(TEXT("lerp_fog"), .17f);
    Air->SetPaletteMaterial(Palette);
    auto CheckPalette = [this, Air](const FLinearColor& ExpectedNear, const FLinearColor& ExpectedFar, float ExpectedMix, float ExpectedUse)
    {
        TArray<UMaterialInstanceDynamic*> MIDs = { Air->GetDensityMID(), Air->GetFinishMID() };
        TestEqual(TEXT("Both volume and per-view finish MIDs receive palette changes"), MIDs.Num(), 2);
        for (UMaterialInstanceDynamic* MID : MIDs)
        {
            if (!TestNotNull(TEXT("Palette destination MID exists"), MID)) continue;
            TestTrue(TEXT("Effective artist near colour reaches the MID"), MID->K2_GetVectorParameterValue(TEXT("ArtistNearColor")).Equals(ExpectedNear, .00001f));
            TestTrue(TEXT("Effective inherited artist far colour reaches the MID"), MID->K2_GetVectorParameterValue(TEXT("ArtistFarColor")).Equals(ExpectedFar, .00001f));
            TestTrue(TEXT("Effective artist blend reaches the MID"), FMath::IsNearlyEqual(MID->K2_GetScalarParameterValue(TEXT("ArtistFogMix")), ExpectedMix));
            TestEqual(TEXT("Artist palette enable flag is sent explicitly"), MID->K2_GetScalarParameterValue(TEXT("UseArtistPalette")), ExpectedUse);
        }
    };
    CheckPalette(Near, OriginalFar, .17f, 1.f);
    const FLinearColor ChangedNear(.09f, .18f, .27f, 1.f);
    Palette->SetVectorParameterValue(TEXT("Fog Color 01"), ChangedNear);
    Palette->SetScalarParameterValue(TEXT("lerp_fog"), .23f);
    Air->Tick(.01f);
    CheckPalette(ChangedNear, OriginalFar, .23f, 1.f);
    FYogAirFogParameters Manual = OriginalParameters;
    Manual.bUseArtistPalette = false;
    Air->ApplyParameters(Manual);
    CheckPalette(ChangedNear, OriginalFar, .23f, 0.f);
    TestTrue(TEXT("Palette synchronisation never overwrites draft manual colours"),
        Air->GetParameters().AmbientColor == OriginalParameters.AmbientColor && Air->GetParameters().FarColor == OriginalParameters.FarColor);
    Manual.bUseArtistPalette = true;
    Air->ApplyParameters(Manual);
    UMaterialInstanceDynamic* Incomplete = UMaterialInstanceDynamic::Create(UMaterial::GetDefaultMaterial(MD_Surface), Air);
    if (!TestNotNull(TEXT("An isolated incomplete palette exists"), Incomplete)) return false;
    Incomplete->SetVectorParameterValue(TEXT("Fog Color 01"), Near);
    Air->SetPaletteMaterial(Incomplete);
    CheckPalette(Near, FLinearColor(.1106f, .1505f, .35f, 1.f), .07f, 1.f);
    TestTrue(TEXT("Missing palette fields are reported without refusing rendering"), Air->GetStatusText().Contains(TEXT("配色材质缺少")));
    Air->SetPaletteMaterial(Palette);
    Air->SuspendForSave(true);
    const FLinearColor ChangedFar(.15f, .12f, .35f, 1.f);
    Palette->SetVectorParameterValue(TEXT("Fog Color 02"), ChangedFar);
    Air->SuspendForSave(false);
    CheckPalette(ChangedNear, ChangedFar, .23f, 1.f);
    Air->SetPaletteMaterial(nullptr);
    CheckPalette(OriginalNear, OriginalFar, OriginalMix, 1.f);
    FLinearColor StillOriginal;
    Compatible->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), StillOriginal);
    TestTrue(TEXT("Shared original palette remains unchanged throughout the fixture"), StillOriginal.Equals(OriginalNear, .00001f));
    Air->Destroy();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogSceneDirectionalLightTest, "YogVision.AirFog.SceneDirectionalLight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogSceneDirectionalLightTest::RunTest(const FString&)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    FAirFogTestWorld Fixture(TEXT("AirFogDirectionalWorld"));
    FAirFogTestWorld OtherFixture(TEXT("AirFogForeignDirectionalWorld"));
    if (!TestNotNull(TEXT("Temporary World"), Fixture.World) || !TestNotNull(TEXT("Foreign World"), OtherFixture.World)) return false;
    AYogAirFogVolume* Air = SpawnAirFogTestVolume(Fixture.World);
    if (!TestNotNull(TEXT("Air fog exists"), Air)) return false;
    FYogAirFogParameters Values = Air->GetParameters();
    TestTrue(TEXT("Automatic scene direction is enabled by default"), Values.bUseSceneDirectionalLight);
    Values.FormLightYaw = 13.f;
    Values.FormLightPitch = 27.f;
    Values.FormLightColor = FLinearColor(.2f, .3f, .4f, 1.f);
    Values.FormLightStrength = .37f;
    Values.FormShadowStrength = .19f;
    Air->ApplyParameters(Values);
    if (!TestTrue(TEXT("Prepared air fog activates"), Air->SetAirFogEnabled(true))) return false;

    auto CheckDirection = [this, Air, &Values](const FVector& Expected, const TCHAR* Phase)
    {
        for (UMaterialInstanceDynamic* MID : { Air->GetDensityMID(), Air->GetFinishMID() })
        {
            if (!TestNotNull(FString::Printf(TEXT("%s destination MID"), Phase), MID)) continue;
            const FVector Actual = FRotator(MID->K2_GetScalarParameterValue(TEXT("FormLightPitch")),
                MID->K2_GetScalarParameterValue(TEXT("FormLightYaw")), 0.f).Vector();
            TestTrue(FString::Printf(TEXT("%s sends the point-to-light direction to both MIDs"), Phase), Actual.Equals(Expected, .0001f));
            TestEqual(TEXT("Automatic light preserves artist light strength"), MID->K2_GetScalarParameterValue(TEXT("FormLightStrength")), Values.FormLightStrength);
            TestEqual(TEXT("Automatic light preserves artist shadow strength"), MID->K2_GetScalarParameterValue(TEXT("FormShadowStrength")), Values.FormShadowStrength);
        }
    };
    const FVector ManualDirection = FRotator(Values.FormLightPitch, Values.FormLightYaw, 0.f).Vector();
    CheckDirection(ManualDirection, TEXT("No scene light fallback"));
    TestTrue(TEXT("No-light status explains the manual fallback"), Air->GetFormLightStatusText().Contains(TEXT("未找到有效场景灯")));

    auto SpawnLight = [](UWorld* World, const TCHAR* Name, bool bSun, int32 Index, float Intensity, FRotator Rotation)
    {
        FActorSpawnParameters Spawn;
        Spawn.ObjectFlags = RF_Transient;
        Spawn.Name = FName(Name);
        AActor* Owner = World->SpawnActor<AActor>(Spawn);
        if (!Owner) return static_cast<UDirectionalLightComponent*>(nullptr);
        UDirectionalLightComponent* Light = NewObject<UDirectionalLightComponent>(Owner);
        Owner->SetRootComponent(Light);
        Owner->AddInstanceComponent(Light);
        Light->SetMobility(EComponentMobility::Movable);
        Light->bAtmosphereSunLight = bSun;
        Light->AtmosphereSunLightIndex = Index;
        Light->Intensity = Intensity;
        Light->LightColor = FColor::White;
        Light->RegisterComponent();
        Light->SetWorldRotation(Rotation);
        return Light;
    };
    UDirectionalLightComponent* Foreign = SpawnLight(OtherFixture.World, TEXT("ForeignSun"), true, 0, 100000.f, FRotator(-10, 90, 0));
    if (!TestNotNull(TEXT("A foreign-world sun exists"), Foreign)) return false;
    Air->Tick(.01f);
    CheckDirection(ManualDirection, TEXT("Foreign-world light is ignored"));
    UDirectionalLightComponent* Key = SpawnLight(Fixture.World, TEXT("FogArtistKey"), true, 0, 10.f, FRotator(-30, 20, 0));
    if (!TestNotNull(TEXT("A real registered scene sun exists"), Key)) return false;
    Key->SetLightColor(FLinearColor(1.f, .5f, .25f));
    Air->Tick(.01f);
    CheckDirection(-Key->GetForwardVector(), TEXT("Scene sun"));
    TestTrue(TEXT("Status identifies the actual scene source"), Air->GetFormLightStatusText().Contains(Key->GetOwner()->GetActorNameOrLabel()));
    const FLinearColor ExpectedColor = Key->GetLightColor() * .5f;
    for (UMaterialInstanceDynamic* MID : { Air->GetDensityMID(), Air->GetFinishMID() })
        if (MID) TestTrue(TEXT("Source colour uses bounded illuminance, not raw HDR intensity"),
            MID->K2_GetVectorParameterValue(TEXT("FormLightColor")).Equals(FLinearColor(ExpectedColor.R, ExpectedColor.G, ExpectedColor.B, 1.f), .00001f));

    Key->SetWorldRotation(FRotator(-55, -70, 0));
    Key->SetIntensity(50000.f);
    Key->SetUseTemperature(true);
    Key->SetTemperature(3000.f);
    Air->Tick(.01f);
    CheckDirection(-Key->GetForwardVector(), TEXT("Runtime light rotation"));
    const FLinearColor TemperatureColor = Key->GetLightColor() * Key->GetColorTemperature();
    const float Normalizer = FMath::Max(1.f, FMath::Max3(TemperatureColor.R, TemperatureColor.G, TemperatureColor.B));
    FLinearColor ExpectedWarm = TemperatureColor * ((50000.f / 50010.f) / Normalizer);
    ExpectedWarm.A = 1.f;
    TestTrue(TEXT("Runtime colour temperature reaches the native MID in working colour space"),
        Air->GetDensityMID()->K2_GetVectorParameterValue(TEXT("FormLightColor")).Equals(ExpectedWarm, .00001f));
    TestTrue(TEXT("Source changes never overwrite manual draft direction or colour"),
        Air->GetParameters().FormLightYaw == Values.FormLightYaw && Air->GetParameters().FormLightPitch == Values.FormLightPitch
        && Air->GetParameters().FormLightColor == Values.FormLightColor);
    Key->SetVisibility(false, false);
    Air->Tick(.01f);
    CheckDirection(ManualDirection, TEXT("Hidden component fallback"));
    Key->SetVisibility(true, false);
    Key->GetOwner()->SetActorHiddenInGame(true);
    Air->Tick(.01f);
    CheckDirection(ManualDirection, TEXT("Hidden owner fallback"));
    Key->GetOwner()->SetActorHiddenInGame(false);
    Key->SetIntensity(0.f);
    Air->Tick(.01f);
    CheckDirection(ManualDirection, TEXT("Zero-intensity fallback"));
    Key->SetIntensity(5.f);
    Key->bAffectsWorld = false;
    Air->Tick(.01f);
    CheckDirection(ManualDirection, TEXT("Disabled world contribution fallback"));
    Key->bAffectsWorld = true;

    UDirectionalLightComponent* HigherIndex = SpawnLight(Fixture.World, TEXT("SecondSun"), true, 1, 100000.f, FRotator(-15, 110, 0));
    UDirectionalLightComponent* BrighterRegular = SpawnLight(Fixture.World, TEXT("RegularKey"), false, 0, 200000.f, FRotator(-40, -5, 0));
    if (!TestNotNull(TEXT("Second sun exists"), HigherIndex) || !TestNotNull(TEXT("Ordinary key exists"), BrighterRegular)) return false;
    Air->Tick(.01f);
    CheckDirection(-Key->GetForwardVector(), TEXT("Lowest atmosphere sun index wins"));
    HigherIndex->SetAtmosphereSunLightIndex(0);
    Air->Tick(.01f);
    CheckDirection(-HigherIndex->GetForwardVector(), TEXT("Brighter source wins within one sun index"));
    Key->SetUseTemperature(false);
    Key->SetLightColor(FLinearColor::White);
    Key->SetIntensity(HigherIndex->Intensity);
    const UDirectionalLightComponent* StableWinner = Key->GetPathName().Compare(HigherIndex->GetPathName(), ESearchCase::CaseSensitive) < 0 ? Key : HigherIndex;
    Air->Tick(.01f);
    CheckDirection(-StableWinner->GetForwardVector(), TEXT("Equal brightness resolves by stable component path"));
    Values.bUseSceneDirectionalLight = false;
    Air->ApplyParameters(Values);
    CheckDirection(ManualDirection, TEXT("Explicit artist manual mode"));
    TestTrue(TEXT("Manual status explains the selected mode"), Air->GetFormLightStatusText().StartsWith(TEXT("手动方向光")));
    TestTrue(TEXT("Reading scene lighting never changes the source intensity"), Key->Intensity == 100000.f && HigherIndex->Intensity == 100000.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogFinishBridgeLifecycleTest, "YogVision.AirFog.FinishBridgeLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogFinishBridgeLifecycleTest::RunTest(const FString&)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    const TArray<FAirFogTestCVarState> Opening = CaptureAirFogConsole();
    FAirFogTestWorld Fixture(TEXT("AirFogFinishBridgeWorld"));
    if (!TestNotNull(TEXT("Temporary World"), Fixture.World)) return false;
    AYogAirFogVolume* Air = SpawnAirFogTestVolume(Fixture.World);
    UExponentialHeightFogComponent* OriginalFog = SpawnAirFogTestExistingFog(Fixture.World, true);
    if (!TestNotNull(TEXT("Air fog exists"), Air) || !TestNotNull(TEXT("Original fog exists"), OriginalFog)) return false;
    Air->bFinishEnabled = false;
    if (!TestTrue(TEXT("Density-only fog activates"), Air->SetAirFogEnabled(true))) return false;
    TestNull(TEXT("Density-only activation exposes no finish template"), Air->GetFinishMID());
    const TArray<FAirFogTestCVarState> ActiveQuality = CaptureAirFogConsole();
    Air->bFinishEnabled = true;
    Air->Tick(.01f);
    UMaterialInstanceDynamic* Finish = Air->GetFinishMID();
    if (!TestNotNull(TEXT("Enabling finish at runtime prepares the per-view template"), Finish)) return false;
    TestTrue(TEXT("Finish template belongs to the air-fog Actor"), Finish->GetOuter() == Air);
    TestTrue(TEXT("Finish template is transient"), Finish->HasAnyFlags(RF_Transient));
    TestEqual(TEXT("Bridge activation never creates an unbound post-process Actor"), CountAirFogTestPostProcess(Fixture.World), 0);
    Air->bFinishEnabled = false;
    Air->ApplyParameters(Air->GetParameters());
    TestNull(TEXT("Disabling finish immediately excludes the template from the bridge"), Air->GetFinishMID());
    TestTrue(TEXT("Finish toggle retains native carrier"), Air->Carrier->GetVisibleFlag());
    TestFalse(TEXT("Finish toggle keeps original fog suppressed"), OriginalFog->GetVisibleFlag());
    CheckAirFogConsole(*this, ActiveQuality, TEXT("Finish toggle keeps the same quality scope"));
    Air->bFinishEnabled = true;
    Air->ApplyParameters(Air->GetParameters());
    TestTrue(TEXT("Finish re-enable reuses the Actor's template"), Air->GetFinishMID() == Finish);
    Air->SuspendForSave(true);
    TestNull(TEXT("Save suspension removes the live finish template"), Air->GetFinishMID());
    TestTrue(TEXT("Save suspension restores authored fog"), OriginalFog->GetVisibleFlag());
    CheckAirFogConsole(*this, Opening, TEXT("Bridge save suspension"));
    Air->SuspendForSave(false);
    TestTrue(TEXT("Save completion resumes the same template"), Air->GetFinishMID() == Finish);
    Air->SetAirFogEnabled(false);
    TestNull(TEXT("Explicit disable removes the live finish template"), Air->GetFinishMID());
    TestEqual(TEXT("No finish Actor remains after lifecycle"), CountAirFogTestPostProcess(Fixture.World), 0);
    CheckAirFogConsole(*this, Opening, TEXT("Bridge lifecycle finished"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogScreenVisionTest, "YogVision.AirFog.ScreenFlow.CentreAndLightIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogScreenVisionTest::RunTest(const FString&)
{
    if (!AirFogRuntimeResourcesReady(*this)) return !HasAnyErrors();
    FAirFogTestWorld Fixture(TEXT("AirFogScreenWorld")), Other(TEXT("AirFogScreenOtherWorld"));
    if (!Fixture.World || !Other.World) return false;
    AYogAirFogVolume* Air = SpawnAirFogTestVolume(Fixture.World);
    if (!TestNotNull(TEXT("Air volume"), Air)) return false;
    Air->bManageLocalLightScattering = false;
    AActor* Observer = SpawnAirFogTestAnchor(Fixture.World, FVector(120, 250, 300));
    AActor* Foreign = SpawnAirFogTestAnchor(Other.World, FVector(999, 999, 999));
    Air->SetFocusActor(Observer);
    TestEqual(TEXT("Game explicit observer has an active projection flag"), Air->ScreenVisionSource.A, 1.f);
    TestEqual(TEXT("Game source tracks observer world position"), Air->ScreenVisionSource.G, 250.f);
    Fixture.World->WorldType = EWorldType::Editor;
    Air->ApplyParameters(Air->GetParameters());
    TestEqual(TEXT("Editor selection never replaces the per-view screen centre"), Air->ScreenVisionSource.A, 0.f);
    Fixture.World->WorldType = EWorldType::Game;
    Air->SetFocusActor(Foreign);
    TestEqual(TEXT("A foreign-world observer is rejected and falls back to screen centre"), Air->ScreenVisionSource.A, 0.f);
    APlayerController* PC = Fixture.World->SpawnActor<APlayerController>();
    APawn* Pawn = Fixture.World->SpawnActor<APawn>();
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
    ULocalPlayer* ScreenLocalPlayer = NewObject<ULocalPlayer>(GEngine, NAME_None, RF_Transient);
    PC->SetPlayer(ScreenLocalPlayer);
    Fixture.World->AddController(PC);
    auto* Root = NewObject<USceneComponent>(Pawn);
    Root->SetMobility(EComponentMobility::Movable);
    Pawn->SetRootComponent(Root); Pawn->AddInstanceComponent(Root); Root->RegisterComponent();
    Pawn->SetActorLocation(FVector(400, 500, 600));
    PC->Possess(Pawn);
    Air->ApplyParameters(Air->GetParameters());
    TestEqual(TEXT("Game automatically resolves the possessed player"), Air->ScreenVisionSource.R, 400.f);
    TestEqual(TEXT("Possessed player flag"), Air->ScreenVisionSource.A, 1.f);
    APointLight* Local = Fixture.World->SpawnActor<APointLight>();
    APointLight* OtherLight = Other.World->SpawnActor<APointLight>();
    if (!Local || !OtherLight) return false;
    auto* Component = Cast<UPointLightComponent>(Local->GetLightComponent());
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetAttenuationRadius(888.f); Component->SetIntensity(1000.f);
    Component->SetVolumetricScatteringIntensity(1.f);
    TestTrue(TEXT("Screen fog activates"), Air->SetAirFogEnabled(true));
    TestEqual(TEXT("Only the current world's active local light is uploaded"), Air->ScreenLightCount, 1);
    FLinearColor Uploaded;
    TestTrue(TEXT("Light radius uniform exists"), Air->GetDensityMID()->GetVectorParameterValue(FMaterialParameterInfo(TEXT("LightPosition0")), Uploaded));
    TestEqual(TEXT("Actual attenuation radius is uploaded in centimetres"), Uploaded.A, 888.f);
    Component->SetVisibility(false);
    Air->ApplyParameters(Air->GetParameters());
    TestEqual(TEXT("Hiding a light removes stale range data immediately"), Air->ScreenLightCount, 0);
    Air->GetDensityMID()->GetVectorParameterValue(FMaterialParameterInfo(TEXT("LightColour0")), Uploaded);
    TestEqual(TEXT("Unused light slot is disabled"), Uploaded.A, 0.f);
    return true;
}

#endif
