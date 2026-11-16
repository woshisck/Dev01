#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SYogAirFogWorkbench.h"
#include "YogAirFogEditorService.h"
#include "YogAirFogVolume.h"
#include "YogVisionSubsystem.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectIterator.h"

namespace AirFogEditorTests
{
constexpr const TCHAR* ArtistMaterialPath = TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline");
constexpr const TCHAR* ArtistCompatMaterialPath = TEXT("/YogVision/AirFog/MI_AirFogArtistCompat.MI_AirFogArtistCompat");
const TCHAR* ConsoleNames[] = {
    TEXT("r.Fog"), TEXT("r.VolumetricFog"), TEXT("r.VolumetricFog.GridPixelSize"),
    TEXT("r.VolumetricFog.GridSizeZ"), TEXT("r.VolumetricFog.HistoryWeight"), TEXT("r.VolumetricFog.LightSoftFading")
};

struct FConsoleState
{
    FString Name;
    FString Value;
    uint32 Flags = 0;
};

struct FBlendableState
{
    TWeakObjectPtr<APostProcessVolume> Volume;
    TArray<FWeightedBlendable> Array;
    float Priority = 0.f;
    float Weight = 0.f;
    bool bEnabled = false;
    bool bUnbound = false;
};

struct FFogVisibility
{
    TWeakObjectPtr<UExponentialHeightFogComponent> Component;
    bool bVisible = false;
};

struct FArtistParameterSnapshot
{
    TMap<FMaterialParameterInfo, float> Scalars;
    TMap<FMaterialParameterInfo, FLinearColor> Vectors;

    static FArtistParameterSnapshot Capture(FAutomationTestBase& Test, UMaterialInterface* Material)
    {
        FArtistParameterSnapshot Snapshot;
        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;
        Material->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            float Value = 0.f;
            if (Test.TestTrue(TEXT("Read original scalar ") + Info.Name.ToString(), Material->GetScalarParameterValue(Info, Value)))
                Snapshot.Scalars.Add(Info, Value);
        }
        Infos.Reset(); Ids.Reset();
        Material->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            FLinearColor Value;
            if (Test.TestTrue(TEXT("Read original vector ") + Info.Name.ToString(), Material->GetVectorParameterValue(Info, Value)))
                Snapshot.Vectors.Add(Info, Value);
        }
        return Snapshot;
    }

    void Check(FAutomationTestBase& Test, const FString& Phase, UMaterialInterface* Material) const
    {
        if (!Test.TestNotNull(Phase + TEXT(": material exists"), Material)) return;
        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;
        Material->GetAllScalarParameterInfo(Infos, Ids);
        Test.TestEqual(Phase + TEXT(": complete scalar parameter set"), Infos.Num(), Scalars.Num());
        for (const auto& Saved : Scalars)
        {
            const FString Label = Phase + TEXT(": scalar ") + Saved.Key.Name.ToString();
            Test.TestTrue(Label + TEXT(" retains name/association/index"), Infos.Contains(Saved.Key));
            float Value = 0.f;
            if (Test.TestTrue(Label + TEXT(" can be read"), Material->GetScalarParameterValue(Saved.Key, Value)))
                Test.TestEqual(Label + TEXT(" retains the original artist value"), Value, Saved.Value);
        }
        Infos.Reset(); Ids.Reset();
        Material->GetAllVectorParameterInfo(Infos, Ids);
        Test.TestEqual(Phase + TEXT(": complete vector parameter set"), Infos.Num(), Vectors.Num());
        for (const auto& Saved : Vectors)
        {
            const FString Label = Phase + TEXT(": vector ") + Saved.Key.Name.ToString();
            Test.TestTrue(Label + TEXT(" retains name/association/index"), Infos.Contains(Saved.Key));
            FLinearColor Value;
            if (Test.TestTrue(Label + TEXT(" can be read"), Material->GetVectorParameterValue(Saved.Key, Value)))
                Test.TestTrue(Label + TEXT(" retains the original artist value"), Value == Saved.Value);
        }
    }
};

struct FSceneState
{
    TArray<FBlendableState> Volumes;
    TArray<FFogVisibility> Fogs;
    TArray<FConsoleState> Console;
    int32 ActorCount = 0;
    bool bMapDirty = false;

    static FSceneState Capture(UWorld* World, APostProcessVolume* ExtraTransientVolume = nullptr, bool bAllTransientVolumes = true)
    {
        FSceneState State;
        State.bMapDirty = World->GetOutermost()->IsDirty();
        for (TActorIterator<AActor> It(World); It; ++It) ++State.ActorCount;
        for (TActorIterator<APostProcessVolume> It(World); It; ++It)
        {
            // Authored arrays and our fixture are authoritative. Other features may
            // legitimately rebuild their own transient volumes when re-enabled.
            if (!bAllTransientVolumes && It->HasAnyFlags(RF_Transient) && *It != ExtraTransientVolume) continue;
            FBlendableState Item;
            Item.Volume = *It;
            Item.Array = It->Settings.WeightedBlendables.Array;
            Item.Priority = It->Priority;
            Item.Weight = It->BlendWeight;
            Item.bEnabled = It->bEnabled;
            Item.bUnbound = It->bUnbound;
            State.Volumes.Add(MoveTemp(Item));
        }
        for (TObjectIterator<UExponentialHeightFogComponent> It; It; ++It)
        {
            if (It->GetWorld() != World || !It->IsRegistered()) continue;
            FFogVisibility Item;
            Item.Component = *It;
            Item.bVisible = It->GetVisibleFlag();
            State.Fogs.Add(Item);
        }
        for (const TCHAR* Name : ConsoleNames)
        {
            if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
            {
                FConsoleState Item;
                Item.Name = Name;
                Item.Value = Variable->GetString();
                Item.Flags = static_cast<uint32>(Variable->GetFlags());
                State.Console.Add(MoveTemp(Item));
            }
        }
        return State;
    }

    void CheckArrays(FAutomationTestBase& Test, const FString& Phase, bool bArtistReplaced) const
    {
        for (const FBlendableState& Saved : Volumes)
        {
            APostProcessVolume* Volume = Saved.Volume.Get();
            if (!Test.TestNotNull(Phase + TEXT(": original postprocess volume survives"), Volume)) continue;
            const FString Label = Phase + TEXT(": ") + Volume->GetName();
            const auto& Current = Volume->Settings.WeightedBlendables.Array;
            Test.TestEqual(Label + TEXT(" array size"), Current.Num(), Saved.Array.Num());
            Test.TestEqual(Label + TEXT(" volume priority"), Volume->Priority, Saved.Priority);
            Test.TestEqual(Label + TEXT(" volume weight"), Volume->BlendWeight, Saved.Weight);
            Test.TestEqual(Label + TEXT(" enabled"), bool(Volume->bEnabled), Saved.bEnabled);
            Test.TestEqual(Label + TEXT(" unbound"), bool(Volume->bUnbound), Saved.bUnbound);
            for (int32 Index = 0; Index < FMath::Min(Current.Num(), Saved.Array.Num()); ++Index)
            {
                const FString Slot = Label + FString::Printf(TEXT(" slot %d"), Index);
                Test.TestEqual(Slot + TEXT(" weight preserved"), Current[Index].Weight, Saved.Array[Index].Weight);
                UObject* Original = Saved.Array[Index].Object.Get();
                if (bArtistReplaced && Original && Original->GetPathName() == ArtistMaterialPath)
                {
                    UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Current[Index].Object.Get());
                    if (Test.TestNotNull(Slot + TEXT(" uses a temporary artist MID"), MID))
                    {
                        UMaterialInterface* Compatible = LoadObject<UMaterialInterface>(nullptr, ArtistCompatMaterialPath);
                        if (Test.TestNotNull(Slot + TEXT(" isolated artist compatibility MI exists"), Compatible))
                            Test.TestEqual(Slot + TEXT(" uses the exact isolated compatibility MI as parent"), static_cast<UObject*>(MID->Parent.Get()), static_cast<UObject*>(Compatible));
                        float Height = 0.f, Contrast = 0.f;
                        Test.TestTrue(Slot + TEXT(" exposes FogHeight"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FogHeight")), Height));
                        Test.TestTrue(Slot + TEXT(" exposes FogContrast"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FogContrast")), Contrast));
                        Test.TestEqual(Slot + TEXT(" original fog branch disabled"), Height, -2750.f);
                        Test.TestEqual(Slot + TEXT(" safe fog denominator"), Contrast, 2750.f);
                    }
                }
                else Test.TestEqual(Slot + TEXT(" exact original object restored"), Current[Index].Object.Get(), Original);
            }
        }
    }

    void CheckFog(FAutomationTestBase& Test, const FString& Phase, bool bSuppressed) const
    {
        for (const FFogVisibility& Saved : Fogs)
        {
            UExponentialHeightFogComponent* Fog = Saved.Component.Get();
            if (Test.TestNotNull(Phase + TEXT(": original height fog survives"), Fog))
                Test.TestEqual(Phase + TEXT(": original height fog visibility ") + Fog->GetPathName(), Fog->GetVisibleFlag(), bSuppressed ? false : Saved.bVisible);
        }
    }

    void CheckConsole(FAutomationTestBase& Test, const FString& Phase) const
    {
        Test.TestEqual(Phase + TEXT(": every leased CVar was captured"), Console.Num(), int32(UE_ARRAY_COUNT(ConsoleNames)));
        for (const FConsoleState& Saved : Console)
        {
            IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Saved.Name);
            if (!Test.TestNotNull(Phase + TEXT(": CVar exists ") + Saved.Name, Variable)) continue;
            Test.TestEqual(Phase + TEXT(": CVar value ") + Saved.Name, Variable->GetString(), Saved.Value);
            Test.TestEqual(Phase + TEXT(": full CVar flags ") + Saved.Name, static_cast<uint32>(Variable->GetFlags()), Saved.Flags);
        }
    }

    void CheckOriginal(FAutomationTestBase& Test, const FString& Phase) const
    {
        CheckArrays(Test, Phase, false);
        CheckFog(Test, Phase, false);
        CheckConsole(Test, Phase);
    }
};

struct FFixture
{
    TWeakObjectPtr<APostProcessVolume> PostProcess;
    TWeakObjectPtr<AExponentialHeightFog> VisibleFog;
    TWeakObjectPtr<AExponentialHeightFog> HiddenFog;
    TWeakObjectPtr<AYogAirFogVolume> PlacedAir;
    ~FFixture()
    {
        FYogAirFogEditorService::RevertPreview();
        if (PlacedAir.IsValid())
        {
            PlacedAir->SetAirFogEnabled(false);
            // The fixture was a real non-transient actor during the test. Make
            // only its teardown transient so Destroy cannot dirty the map.
            PlacedAir->SetFlags(RF_Transient);
            PlacedAir->Destroy();
        }
        if (PostProcess.IsValid()) PostProcess->Destroy();
        if (VisibleFog.IsValid()) VisibleFog->Destroy();
        if (HiddenFog.IsValid()) HiddenFog->Destroy();
    }
};

FActorSpawnParameters FixtureSpawn()
{
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient | RF_DuplicateTransient;
    Spawn.bTemporaryEditorActor = true;
    Spawn.bHideFromSceneOutliner = true;
    Spawn.bCreateActorPackage = false;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return Spawn;
}

AYogAirFogVolume* FindPreviewActor(UWorld* World)
{
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
        if (It->HasAnyFlags(RF_Transient) && It->IsAirFogEnabled()) return *It;
    return nullptr;
}

UMaterialInstanceDynamic* FindFinishMID(UWorld* World, AYogAirFogVolume* Air)
{
    return Air ? Air->GetFinishMID() : nullptr;
}

int32 CountLightActors(UWorld* World)
{
    int32 Count = 0;
    for (TActorIterator<ALight> It(World); It; ++It) ++Count;
    return Count;
}

struct FScopedDraftFile
{
    FAutomationTestBase& Test;
    FString Path = FPaths::ProjectSavedDir() / TEXT("AirFog/ArtDraft.json");
    TArray<uint8> OriginalBytes;
    bool bExisted = false;
    bool bCaptured = false;

    explicit FScopedDraftFile(FAutomationTestBase& InTest, const FString& OverridePath = FString()) : Test(InTest)
    {
        if (!OverridePath.IsEmpty()) Path = OverridePath;
        bExisted = IFileManager::Get().FileExists(*Path);
        bCaptured = !bExisted || FFileHelper::LoadFileToArray(OriginalBytes, *Path);
    }
    ~FScopedDraftFile()
    {
        if (!bCaptured) return;
        const bool bRestored = bExisted ? FFileHelper::SaveArrayToFile(OriginalBytes, *Path)
            : IFileManager::Get().Delete(*Path, false, false, true);
        Test.TestTrue(TEXT("Saved art draft is restored to its original bytes or absence"), bRestored);
    }
};

void CheckDensities(FAutomationTestBase& Test, const FString& Phase, const FYogAirFogParameters& Parameters, float Density, float Near, float Clear)
{
    Test.TestEqual(Phase + TEXT(": far density"), Parameters.Density, Density);
    Test.TestEqual(Phase + TEXT(": near absolute density"), Parameters.NearDensity, Near);
    Test.TestEqual(Phase + TEXT(": clear-region absolute density"), Parameters.ClearMinDensity, Clear);
    Test.TestTrue(Phase + TEXT(": density order is physical"), Parameters.ClearMinDensity >= 0.f
        && Parameters.ClearMinDensity <= Parameters.NearDensity && Parameters.NearDensity <= Parameters.Density);
}

void CheckArtistPaletteSettings(FAutomationTestBase& Test, const FString& Phase, const FYogAirFogParameters& Parameters)
{
    const FYogAirFogParameters Defaults;
    Test.TestTrue(Phase + TEXT(": the original artist palette remains authoritative"), Parameters.bUseArtistPalette);
    Test.TestEqual(Phase + TEXT(": artist palette distance"), Parameters.PaletteDistance, 3500.f);
    Test.TestEqual(Phase + TEXT(": artist palette strength"), Parameters.PaletteStrength, 1.f);
    Test.TestEqual(Phase + TEXT(": camera-relative height offset"), Parameters.CameraHeightOffset, 1100.f);
    Test.TestEqual(Phase + TEXT(": selected ambient intensity"), Parameters.AmbientIntensity, .25f);
    Test.TestTrue(Phase + TEXT(": presets do not recolour manual ambient fallback"), Parameters.AmbientColor == Defaults.AmbientColor);
    Test.TestTrue(Phase + TEXT(": presets do not recolour manual distant fallback"), Parameters.FarColor == Defaults.FarColor);
    Test.TestTrue(Phase + TEXT(": presets do not recolour structure edges"), Parameters.EdgeColor == Defaults.EdgeColor);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogWorkbenchReadOnlyTest, "YogAirFog.Editor.WorkbenchReadOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogWorkbenchReadOnlyTest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || !FSlateApplication::IsInitialized()) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    if (!TestFalse(TEXT("Read-only regression starts without an owned air fog preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    const FSceneState Before = FSceneState::Capture(World);
    const int32 OriginalLights = CountLightActors(World);
    UYogVisionSubsystem* Legacy = World->GetSubsystem<UYogVisionSubsystem>();
    const bool bLegacyEnabled = Legacy && Legacy->IsEnabled();
    {
        TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
        Widget->SlatePrepass(1.f);
        const FSceneState Opened = FSceneState::Capture(World);
        TestEqual(TEXT("Opening spawns no actor"), Opened.ActorCount, Before.ActorCount);
        TestEqual(TEXT("Opening adds no postprocess volume"), Opened.Volumes.Num(), Before.Volumes.Num());
        TestEqual(TEXT("Opening adds no height fog component"), Opened.Fogs.Num(), Before.Fogs.Num());
        TestEqual(TEXT("Opening adds no light actor"), CountLightActors(World), OriginalLights);
        TestEqual(TEXT("Opening retains the map dirty state"), Opened.bMapDirty, Before.bMapDirty);
        Before.CheckOriginal(*this, TEXT("Opening"));
        CheckDensities(*this, TEXT("New workbench defaults"), Widget->GetDraftParameters(), .10f, .006f, .003f);
        CheckArtistPaletteSettings(*this, TEXT("New workbench defaults"), Widget->GetDraftParameters());
        TestEqual(TEXT("New workbench defaults use an absolute camera depth"), Widget->GetDraftParameters().NearOffset, 1800.f);
        TestEqual(TEXT("New workbench defaults use selected depth ramp"), Widget->GetDraftParameters().DepthRamp, 1800.f);
        TestNull(TEXT("Workbench and existing console callers do not resolve a selected actor"), SYogAirFogWorkbench::ResolveFocusActor());
        for (const int32 Preset : {1, 2, 3, 0})
        {
            Widget->ApplyPreset(Preset);
            const FYogAirFogParameters& P = Widget->GetDraftParameters();
            CheckArtistPaletteSettings(*this, TEXT("Draft preset palette"), P);
            TestTrue(TEXT("Every preset uses positive absolute camera depth and growth distance"), P.NearOffset > 0.f && P.DepthRamp > 0.f);
            TestEqual(TEXT("Every preset preserves near thin-air density"), P.NearDensity, .006f);
            TestEqual(TEXT("Every preset preserves camera-near residual density"), P.ClearMinDensity, .003f);
            TestTrue(TEXT("Every draft preset retains thin air inside the clear region"), P.ClearMinDensity > 0.f
                && P.ClearMinDensity <= P.NearDensity && P.NearDensity <= P.Density);
        }
        TestFalse(TEXT("Draft presets do not activate the real service"), FYogAirFogEditorService::IsPreviewActive());
        Before.CheckOriginal(*this, TEXT("Editing presets before preview"));
    }
    const FSceneState Closed = FSceneState::Capture(World);
    TestEqual(TEXT("Closing an untouched draft spawns no actor"), Closed.ActorCount, Before.ActorCount);
    TestEqual(TEXT("Closing retains the map dirty state"), Closed.bMapDirty, Before.bMapDirty);
    TestEqual(TEXT("Opening and closing retain legacy enable state"), Legacy && Legacy->IsEnabled(), bLegacyEnabled);
    Before.CheckOriginal(*this, TEXT("Closing untouched draft"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogDensityDraftTest, "YogAirFog.Editor.DensityDraftSaveLoadAndUndo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogDensityDraftTest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized()) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    if (!TestFalse(TEXT("Draft round trip does not take over an active preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    FScopedDraftFile RestoreDraft(*this);
    if (!TestTrue(TEXT("The user's draft file can be preserved before the round trip"), RestoreDraft.bCaptured)) return false;
    const FString Name = TEXT("AirFogDensity_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FScopedDraftFile RestoreNamed(*this, FYogAirFogEditorService::GetPresetDirectory() / (Name + TEXT(".json")));
    if (!TestTrue(TEXT("The test-owned named file can be preserved"), RestoreNamed.bCaptured)) return false;
    const FSceneState Before = FSceneState::Capture(World);
    TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
    CheckDensities(*this, TEXT("Initial draft"), Widget->GetDraftParameters(), .10f, .006f, .003f);
    Widget->ApplyPreset(1);
    const FYogAirFogParameters Saved = Widget->GetDraftParameters();
    CheckDensities(*this, TEXT("Dense preset"), Saved, .16f, .006f, .003f);
    Widget->SetSaveAsName(Name);
    Widget->SaveDraftAs();
    FYogAirFogParameters Disk;
    FYogAirFogPresetData NamedDisk;
    FText Status;
    if (!TestTrue(TEXT("The actual selected named draft can be read"), FYogAirFogEditorService::LoadNamedPreset(Name, NamedDisk, Status)))
    {
        AddError(Status.ToString());
        return false;
    }
    Disk = NamedDisk.Parameters;
    TestTrue(TEXT("Legacy SavePreset remains explicitly callable"), FYogAirFogEditorService::SavePreset(Saved, Status));
    FYogAirFogParameters LegacyDisk;
    TestTrue(TEXT("Legacy LoadPreset remains explicitly callable"), FYogAirFogEditorService::LoadPreset(LegacyDisk, Status));
    TestTrue(TEXT("Legacy wrappers retain every parameter including runtime bools"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&LegacyDisk, &Saved, 0));
    CheckDensities(*this, TEXT("Saved absolute densities"), Disk, Saved.Density, Saved.NearDensity, Saved.ClearMinDensity);
    TestTrue(TEXT("Saved draft round trip retains every parameter"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Disk, &Saved, 0));

    Widget->ApplyPreset(2);
    const FYogAirFogParameters BeforeLoad = Widget->GetDraftParameters();
    CheckDensities(*this, TEXT("Readable preset"), BeforeLoad, .045f, .006f, .003f);
    Widget->LoadDraft();
    CheckDensities(*this, TEXT("Workbench load"), Widget->GetDraftParameters(), Saved.Density, Saved.NearDensity, Saved.ClearMinDensity);
    TestTrue(TEXT("Workbench load retains the saved draft"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &Saved, 0));
    TestTrue(TEXT("The actual editor transaction undoes draft loading"), GEditor->UndoTransaction());
    TestTrue(TEXT("Undo restores the complete previous draft"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &BeforeLoad, 0));
    CheckDensities(*this, TEXT("Undo absolute densities"), Widget->GetDraftParameters(), BeforeLoad.Density, BeforeLoad.NearDensity, BeforeLoad.ClearMinDensity);
    TestTrue(TEXT("The actual editor transaction redoes draft loading"), GEditor->RedoTransaction());
    TestTrue(TEXT("Redo restores the complete loaded draft"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &Saved, 0));
    TestFalse(TEXT("Save/load/undo does not start a presentation"), FYogAirFogEditorService::IsPreviewActive());
    const FSceneState After = FSceneState::Capture(World);
    TestEqual(TEXT("Save/load/undo spawns no scene actor"), After.ActorCount, Before.ActorCount);
    TestEqual(TEXT("Save/load/undo retains map dirty state"), After.bMapDirty, Before.bMapDirty);
    Before.CheckOriginal(*this, TEXT("Draft save/load/undo"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogDensityPreviewTest, "YogAirFog.Editor.DensityPreviewAndBaselineReadback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogDensityPreviewTest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized()) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    if (!TestFalse(TEXT("Density readback does not take over an active service preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
        if (!TestFalse(TEXT("Density readback does not take over an active user actor"), It->IsAirFogEnabled())) return false;
    UYogVisionSubsystem* Legacy = World->GetSubsystem<UYogVisionSubsystem>();
    if (Legacy && !TestFalse(TEXT("Density readback does not take over a user's legacy preview"), Legacy->HasPreviewSnapshot())) return false;
    if (!TestNotNull(TEXT("Prepare the V5 density asset before preview readback"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity")))
        || !TestNotNull(TEXT("Prepare the V5 finish asset before preview readback"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish")))) return false;
    const FSceneState Original = FSceneState::Capture(World, nullptr, false);
    TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
    Widget->ApplyPreset(1);
    Widget->PreviewAirFog();
    AYogAirFogVolume* Air = FindPreviewActor(World);
    if (!TestNotNull(TEXT("The workbench starts a real preview actor"), Air)) return false;
    TestTrue(TEXT("Preview retains the default local-light management policy"), Air->bManageLocalLightScattering);
    auto CheckReadback = [this, Air](const FString& Phase, float Density, float Near, float Clear)
    {
        CheckDensities(*this, Phase, Air->GetParameters(), Density, Near, Clear);
        UMaterialInstanceDynamic* MID = Air->GetDensityMID();
        if (!TestNotNull(Phase + TEXT(": the real density MID exists"), MID)) return;
        const FName Names[] = { FName(TEXT("Density")), FName(TEXT("NearDensity")), FName(TEXT("ClearMinDensity")) };
        const float Expected[] = { Density, Near, Clear };
        for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Names)); ++Index)
        {
            float Value = -1.f;
            TestTrue(Phase + TEXT(": MID parameter exists ") + Names[Index].ToString(), MID->GetScalarParameterValue(FMaterialParameterInfo(Names[Index]), Value));
            TestEqual(Phase + TEXT(": MID absolute density ") + Names[Index].ToString(), Value, Expected[Index]);
        }
    };
    TestFalse(TEXT("Workbench preview is independent of actor selection"), Air->FocusActor.IsValid());
    CheckReadback(TEXT("Dense preview"), .16f, .006f, .003f);
    Widget->ApplyPreset(2);
    TestEqual(TEXT("A live preset edit keeps the same preview actor"), FindPreviewActor(World), Air);
    CheckReadback(TEXT("Live readable preset"), .045f, .006f, .003f);
    const FYogAirFogParameters ReadableDraft = Widget->GetDraftParameters();
    Widget->ApplyPreset(3);
    const FYogAirFogParameters OilDraft = Widget->GetDraftParameters();
    const FYogAirFogParameters Defaults;
    CheckReadback(TEXT("Live oil-cloud preset"), .10f, .006f, .003f);
    CheckArtistPaletteSettings(*this, TEXT("Oil-cloud preset retains artist authority"), OilDraft);
    TestTrue(TEXT("Oil-cloud preset uses broader and stronger strokes than the balanced default"), OilDraft.StrokeScale < Defaults.StrokeScale && OilDraft.StrokeStrength > Defaults.StrokeStrength);
    TestEqual(TEXT("Oil-cloud preset retains the default form direction"), OilDraft.FormLightYaw, Defaults.FormLightYaw);
    TestEqual(TEXT("Oil-cloud preset retains the default form pitch"), OilDraft.FormLightPitch, Defaults.FormLightPitch);
    TestEqual(TEXT("Oil-cloud preset retains the default form intensity"), OilDraft.FormLightStrength, Defaults.FormLightStrength);
    TestEqual(TEXT("Oil-cloud preset retains the default form shadow intensity"), OilDraft.FormShadowStrength, Defaults.FormShadowStrength);
    TestTrue(TEXT("Oil-cloud preset retains the default form colour"), OilDraft.FormLightColor == Defaults.FormLightColor);
    auto CheckOilMaterials = [this, World, Air, &OilDraft]()
    {
        TestEqual(TEXT("Oil-cloud edit and undo/redo retain the live preview actor"), FindPreviewActor(World), Air);
        const FYogAirFogParameters Active = Air->GetParameters();
        TestTrue(TEXT("The real preview receives the complete oil-cloud draft"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Active, &OilDraft, 0));
        for (UMaterialInstanceDynamic* MID : {Air->GetDensityMID(), FindFinishMID(World, Air)})
        {
            if (!TestNotNull(TEXT("Oil-cloud preset retains both real materials"), MID)) continue;
            float Value = -1.f;
            if (TestTrue(TEXT("The live material exposes oil-cloud stroke scale"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("StrokeScale")), Value)))
                TestEqual(TEXT("Both live materials receive the broader oil-cloud scale"), Value, OilDraft.StrokeScale);
            if (TestTrue(TEXT("The live material exposes oil-cloud flow speed"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("StrokeFlowSpeed")), Value)))
                TestEqual(TEXT("Both live materials receive the oil-cloud flow speed"), Value, OilDraft.StrokeFlowSpeed);
        }
    };
    CheckOilMaterials();
    TestTrue(TEXT("The actual editor transaction undoes the live oil-cloud preset"), GEditor->UndoTransaction());
    TestTrue(TEXT("Undo restores every readable draft parameter"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &ReadableDraft, 0));
    CheckReadback(TEXT("Undo restores the live readable densities"), .045f, .006f, .003f);
    const FYogAirFogParameters ActiveAfterUndo = Air->GetParameters();
    TestTrue(TEXT("Undo synchronizes the complete live readable draft"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&ActiveAfterUndo, &ReadableDraft, 0));
    TestTrue(TEXT("The actual editor transaction redoes the live oil-cloud preset"), GEditor->RedoTransaction());
    CheckOilMaterials();
    const FYogAirFogParameters SavedDraft = Widget->GetDraftParameters();
    Widget->PreviewBaseline();
    CheckReadback(TEXT("Baseline disables all absolute densities"), 0.f, 0.f, 0.f);
    TestFalse(TEXT("Baseline disables the distant finish"), Air->bFinishEnabled);
    TestTrue(TEXT("Baseline preserves every draft parameter"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &SavedDraft, 0));
    Widget->PreviewAirFog();
    CheckReadback(TEXT("Return from baseline"), SavedDraft.Density, SavedDraft.NearDensity, SavedDraft.ClearMinDensity);

    const FYogAirFogPresetData BeforeNamedLoad = Widget->GetDraftSnapshot();
    const FString NamedId = TEXT("AirFogLiveIdentity_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FScopedDraftFile RestoreNamed(*this, FYogAirFogEditorService::GetPresetDirectory() / (NamedId + TEXT(".json")));
    if (!TestTrue(TEXT("The live identity test owns its named file"), RestoreNamed.bCaptured)) return false;
    FYogAirFogPresetData OtherIdentity = BeforeNamedLoad;
    OtherIdentity.bFinishEnabled = false;
    UMaterialInterface* OtherPalette = UMaterial::GetDefaultMaterial(MD_PostProcess);
    OtherIdentity.PaletteMaterial = FSoftObjectPath(OtherPalette);
    FText NamedMessage;
    if (!TestTrue(TEXT("Save a separate complete preview identity"), FYogAirFogEditorService::SaveNamedPreset(NamedId, OtherIdentity, false, NamedMessage))) return false;
    Widget->RefreshDrafts();
    if (!TestTrue(TEXT("Choose the complete named preview identity"), Widget->SelectSavedPreset(NamedId))) return false;
    Widget->LoadDraft();
    TestEqual(TEXT("Named loading retains the active preview actor"), FindPreviewActor(World), Air);
    TestEqual(TEXT("Named palette identity reaches the actual runtime actor"), Air->PaletteMaterial.Get(), OtherPalette);
    TestFalse(TEXT("The named finish switch reaches the actual runtime actor"), Air->bFinishEnabled);
    TestTrue(TEXT("Undo the named load with the real editor transaction"), GEditor->UndoTransaction());
    TestEqual(TEXT("Undo named loading restores the actual palette identity"), FSoftObjectPath(Air->PaletteMaterial.Get()).ToString(), BeforeNamedLoad.PaletteMaterial.ToString());
    TestEqual(TEXT("Undo named loading restores the actual finish switch"), Air->bFinishEnabled, BeforeNamedLoad.bFinishEnabled);
    CheckReadback(TEXT("Undo named loading retains the original density draft"), SavedDraft.Density, SavedDraft.NearDensity, SavedDraft.ClearMinDensity);

    FYogAirFogParameters Invalid = SavedDraft;
    Invalid.Density = .006f;
    Invalid.NearDensity = .04f;
    Invalid.ClearMinDensity = .08f;
    FYogAirFogEditorService::UpdatePreview(Invalid, true);
    CheckReadback(TEXT("Preview enforces clear <= near <= far"), .006f, .006f, .006f);
    FYogAirFogEditorService::RevertPreview();
    TestFalse(TEXT("Density readback releases preview ownership"), FYogAirFogEditorService::IsPreviewActive());
    Original.CheckOriginal(*this, TEXT("Density preview revert"));
    TestEqual(TEXT("Density preview retains map dirty state"), World->GetOutermost()->IsDirty(), Original.bMapDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogArtistPaletteSyncTest, "YogAirFog.Editor.ArtistPaletteLiveMIDSync", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogArtistPaletteSyncTest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized()) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("A real editor world exists"), World)) return false;
    if (!TestFalse(TEXT("Palette regression does not take over a service preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
        if (!TestFalse(TEXT("Palette regression does not take over an active user actor"), It->IsAirFogEnabled())) return false;
    UYogVisionSubsystem* Legacy = World->GetSubsystem<UYogVisionSubsystem>();
    if (Legacy && !TestFalse(TEXT("Palette regression does not take over a user's legacy preview"), Legacy->HasPreviewSnapshot())) return false;
    UMaterialInterface* Artist = LoadObject<UMaterialInterface>(nullptr, ArtistMaterialPath);
    if (!TestNotNull(TEXT("The persistent artist source exists"), Artist)
        || !TestNotNull(TEXT("Prepare the V5 density asset before palette readback"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity")))
        || !TestNotNull(TEXT("Prepare the V5 finish asset before palette readback"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish")))) return false;
    const FArtistParameterSnapshot OriginalArtist = FArtistParameterSnapshot::Capture(*this, Artist);
    const bool bArtistDirty = Artist->GetOutermost()->IsDirty();
    FLinearColor OriginalNear, OriginalFar;
    float OriginalMix = -1.f;
    if (!TestTrue(TEXT("The real MI exposes Fog Color 01"), Artist->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), OriginalNear))
        || !TestTrue(TEXT("The real MI exposes Fog Color 02"), Artist->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), OriginalFar))
        || !TestTrue(TEXT("The real MI exposes lerp_fog"), Artist->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), OriginalMix))) return false;

    const FSceneState OriginalScene = FSceneState::Capture(World, nullptr, false);
    const int32 OriginalLights = CountLightActors(World);
    FFixture Cleanup;
    TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
    Widget->PreviewWithFinish();
    AYogAirFogVolume* Air = FindPreviewActor(World);
    if (!TestNotNull(TEXT("The workbench creates a real camera-based preview"), Air)) return false;
    TestEqual(TEXT("Direction and normal shaping create no light actor"), CountLightActors(World), OriginalLights);
    TestFalse(TEXT("The palette preview has no selected-actor anchor"), Air->FocusActor.IsValid());
    TStrongObjectPtr<UMaterialInstanceDynamic> Source(UMaterialInstanceDynamic::Create(Artist, GetTransientPackage()));
    if (!TestNotNull(TEXT("Palette changes use a transient source MID"), Source.Get())) return false;
    Source->SetFlags(RF_Transient);
    Air->SetPaletteMaterial(Source.Get());
    TestEqual(TEXT("The runtime retains the supplied source interface"), Air->PaletteMaterial.Get(), static_cast<UMaterialInterface*>(Source.Get()));

    auto CheckPalette = [this, World, Air](const FString& Phase, const FLinearColor& Near, const FLinearColor& Far, float Mix, bool bUsePalette)
    {
        UMaterialInstanceDynamic* DensityMID = Air->GetDensityMID();
        UMaterialInstanceDynamic* FinishMID = FindFinishMID(World, Air);
        if (!TestNotNull(Phase + TEXT(": actual density MID exists"), DensityMID)
            || !TestNotNull(Phase + TEXT(": actual finish bridge MID exists"), FinishMID)) return;
        for (UMaterialInstanceDynamic* MID : {DensityMID, FinishMID})
        {
            const FString Label = Phase + TEXT(": ") + MID->GetName();
            for (const TPair<FName, FLinearColor>& Parameter : {
                TPair<FName, FLinearColor>(FName(TEXT("ArtistNearColor")), Near),
                TPair<FName, FLinearColor>(FName(TEXT("ArtistFarColor")), Far)})
            {
                FLinearColor Value;
                const FString Name = Label + TEXT(" ") + Parameter.Key.ToString();
                if (TestTrue(Name + TEXT(" parameter exists"), MID->GetVectorParameterValue(FMaterialParameterInfo(Parameter.Key), Value)))
                    TestTrue(Name + TEXT(" reads the effective inherited artist value"), Value == Parameter.Value);
            }
            float Value = -1.f;
            if (TestTrue(Label + TEXT(" exposes ArtistFogMix"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ArtistFogMix")), Value)))
                TestEqual(Label + TEXT(" synchronizes lerp_fog"), Value, Mix);
            if (TestTrue(Label + TEXT(" exposes UseArtistPalette"), MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseArtistPalette")), Value)))
                TestEqual(Label + TEXT(" applies the artist/manual switch"), Value, bUsePalette ? 1.f : 0.f);
        }
    };
    CheckPalette(TEXT("Inherited original values"), OriginalNear, OriginalFar, OriginalMix, true);
    const FLinearColor NearA(.11f, .023f, .047f, 1.f), FarA(.08f, .04f, .019f, 1.f);
    Source->SetVectorParameterValue(TEXT("Fog Color 01"), NearA);
    Air->ApplyParameters(Widget->GetDraftParameters());
    CheckPalette(TEXT("ApplyParameters resolves partial overrides and inheritance"), NearA, OriginalFar, OriginalMix, true);
    Source->SetVectorParameterValue(TEXT("Fog Color 02"), FarA);
    Source->SetScalarParameterValue(TEXT("lerp_fog"), .37f);
    Air->Tick(0.f);
    CheckPalette(TEXT("Tick observes both colours and lerp_fog without restarting"), NearA, FarA, .37f, true);

    // A save suspends the presentation, while in-memory edits to the source
    // interface must be reflected when the same real preview resumes.
    const FLinearColor NearB(.031f, .071f, .12f, 1.f), FarB(.009f, .017f, .033f, 1.f);
    FYogAirFogEditorService::BeginSave(World);
    Source->SetVectorParameterValue(TEXT("Fog Color 01"), NearB);
    Source->SetVectorParameterValue(TEXT("Fog Color 02"), FarB);
    Source->SetScalarParameterValue(TEXT("lerp_fog"), .61f);
    FYogAirFogEditorService::EndSave(World);
    TestEqual(TEXT("Save resumes the same palette source interface"), Air->PaletteMaterial.Get(), static_cast<UMaterialInterface*>(Source.Get()));
    CheckPalette(TEXT("Save resume reads the current source values"), NearB, FarB, .61f, true);

    FYogAirFogParameters Manual = Widget->GetDraftParameters();
    Manual.bUseArtistPalette = false;
    Manual.AmbientColor = FLinearColor(.017f, .027f, .037f, 1.f);
    Manual.FarColor = FLinearColor(.007f, .013f, .023f, 1.f);
    Air->ApplyParameters(Manual);
    CheckPalette(TEXT("Manual fallback is selected explicitly"), NearB, FarB, .61f, false);
    for (UMaterialInstanceDynamic* MID : {Air->GetDensityMID(), FindFinishMID(World, Air)})
    {
        if (!TestNotNull(TEXT("Manual fallback retains both real materials"), MID)) continue;
        FLinearColor Value;
        if (TestTrue(TEXT("Manual ambient parameter exists in the real MID"), MID->GetVectorParameterValue(FMaterialParameterInfo(TEXT("AmbientColor")), Value)))
            TestTrue(TEXT("Manual ambient colour reaches the real MID"), Value == Manual.AmbientColor);
        if (TestTrue(TEXT("Manual far parameter exists in the real MID"), MID->GetVectorParameterValue(FMaterialParameterInfo(TEXT("FarColor")), Value)))
            TestTrue(TEXT("Manual far colour reaches the real MID"), Value == Manual.FarColor);
    }
    TestTrue(TEXT("Reading artist colours does not rewrite the manual ambient draft"), Air->GetParameters().AmbientColor == Manual.AmbientColor);
    TestTrue(TEXT("Reading artist colours does not rewrite the manual far draft"), Air->GetParameters().FarColor == Manual.FarColor);
    Manual.bUseArtistPalette = true;
    Air->ApplyParameters(Manual);
    CheckPalette(TEXT("Returning to the original palette uses the current artist values"), NearB, FarB, .61f, true);

    FYogAirFogEditorService::RevertPreview();
    OriginalScene.CheckOriginal(*this, TEXT("Palette preview revert"));
    OriginalArtist.Check(*this, TEXT("Persistent original MI after live palette edits"), Artist);
    TestEqual(TEXT("The persistent artist package dirty state is untouched"), Artist->GetOutermost()->IsDirty(), bArtistDirty);
    TestEqual(TEXT("Palette preview and revert add no light actor"), CountLightActors(World), OriginalLights);
    TestEqual(TEXT("Palette preview retains the map dirty state"), World->GetOutermost()->IsDirty(), OriginalScene.bMapDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogSaveLifecycleTest, "YogAirFog.Editor.SaveSuspendResumeAndRevert", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogSaveLifecycleTest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || GEditor->PlayWorld) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    if (!TestFalse(TEXT("Lifecycle regression starts without another air fog preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    UYogVisionSubsystem* Legacy = World->GetSubsystem<UYogVisionSubsystem>();
    if (Legacy && !TestFalse(TEXT("Lifecycle regression does not take over a user's legacy preview"), Legacy->HasPreviewSnapshot())) return false;
    UMaterialInterface* Artist = LoadObject<UMaterialInterface>(nullptr, ArtistMaterialPath);
    if (!TestNotNull(TEXT("Audited artist MI exists"), Artist)) return false;
    UMaterialInterface* Compatible = LoadObject<UMaterialInterface>(nullptr, ArtistCompatMaterialPath);
    if (!TestNotNull(TEXT("Prepare the isolated compatibility MI before lifecycle regression"), Compatible)) return false;
    const FArtistParameterSnapshot OriginalArtistParameters = FArtistParameterSnapshot::Capture(*this, Artist);
    TestTrue(TEXT("Artist scalar parity check is non-empty"), OriginalArtistParameters.Scalars.Num() > 0);
    TestTrue(TEXT("Artist vector parity check is non-empty"), OriginalArtistParameters.Vectors.Num() > 0);
    // Compare the persistent compatibility instance before the temporary MID's
    // FogHeight/FogContrast overrides. Every authored value must be preserved.
    OriginalArtistParameters.Check(*this, TEXT("Compatibility MI artist parity"), Compatible);
    UMaterial* Density = LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity"));
    UMaterial* Finish = LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish"));
    if (!TestNotNull(TEXT("Prepare the real air density asset before lifecycle regression"), Density)
        || !TestNotNull(TEXT("Prepare the real distant finish asset before lifecycle regression"), Finish)) return false;
    TestEqual(TEXT("Density asset uses the actual Volume domain"), Density->MaterialDomain, TEnumAsByte<EMaterialDomain>(MD_Volume));
    TestEqual(TEXT("Finish asset uses the actual postprocess domain"), Finish->MaterialDomain, TEnumAsByte<EMaterialDomain>(MD_PostProcess));

    FFixture Fixture;
    const FActorSpawnParameters Spawn = FixtureSpawn();
    Fixture.PostProcess = World->SpawnActor<APostProcessVolume>(Spawn);
    Fixture.VisibleFog = World->SpawnActor<AExponentialHeightFog>(Spawn);
    Fixture.HiddenFog = World->SpawnActor<AExponentialHeightFog>(Spawn);
    if (!TestNotNull(TEXT("Transient postprocess fixture exists"), Fixture.PostProcess.Get())
        || !TestNotNull(TEXT("Visible height fog fixture exists"), Fixture.VisibleFog.Get())
        || !TestNotNull(TEXT("Hidden height fog fixture exists"), Fixture.HiddenFog.Get())) return false;
    Fixture.VisibleFog->GetComponent()->SetVisibility(true, false);
    Fixture.HiddenFog->GetComponent()->SetVisibility(false, false);
    Fixture.PostProcess->bUnbound = true;
    Fixture.PostProcess->Priority = 3.5f;
    Fixture.PostProcess->BlendWeight = .83f;
    // Duplicate artist entries with different weights exercise slot identity;
    // an object-wide replacement must not merge or reweight legal array entries.
    Fixture.PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(.23f, Artist));
    Fixture.PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(.61f, Artist));
    Fixture.PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(.49f, UMaterial::GetDefaultMaterial(MD_PostProcess)));
    FSceneState Original = FSceneState::Capture(World, Fixture.PostProcess.Get(), false);
    const bool bLegacyEnabled = Legacy && Legacy->IsEnabled();
    float OriginalFogHeight = 0.f;
    TestTrue(TEXT("Artist asset exposes its fog height"), Artist->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FogHeight")), OriginalFogHeight));

    FYogAirFogParameters Draft;
    Draft.Density = .073f;
    Draft.ClearRadius = 517.f;
    Draft.GridPixelSize = 12;
    Draft.GridSizeZ = 80;
    Draft.HistoryWeight = .63f;
    FText Status;
    if (!TestTrue(TEXT("Real service creates the air fog preview"), FYogAirFogEditorService::Preview(Draft, nullptr, true, Status)))
    {
        AddError(Status.ToString());
        return false;
    }
    AYogAirFogVolume* Air = FindPreviewActor(World);
    if (!TestNotNull(TEXT("The real preview actor is active"), Air)) return false;
    TWeakObjectPtr<AYogAirFogVolume> PreviewIdentity(Air);
    const FSceneState Active = FSceneState::Capture(World);

    auto CheckActive = [this, World, Air, &Original, &Active, &Draft, &Fixture]()
    {
        TestTrue(TEXT("Save retains service preview ownership"), FYogAirFogEditorService::IsPreviewActive());
        TestEqual(TEXT("Save keeps the same air fog actor"), FindPreviewActor(World), Air);
        TestTrue(TEXT("Save resumes native carrier visibility"), Air->Carrier->GetVisibleFlag());
        TestTrue(TEXT("Save resumes native volume visibility"), Air->VolumeBounds->GetVisibleFlag());
        TestFalse(TEXT("Save retains camera-based preview without a focus actor"), Air->FocusActor.IsValid());
        TestEqual(TEXT("Save retains density"), Air->GetParameters().Density, Draft.Density);
        TestEqual(TEXT("Save retains near density"), Air->GetParameters().NearDensity, Draft.NearDensity);
        TestEqual(TEXT("Save retains clear-region density"), Air->GetParameters().ClearMinDensity, Draft.ClearMinDensity);
        TestEqual(TEXT("Save retains clear radius"), Air->GetParameters().ClearRadius, Draft.ClearRadius);
        TestTrue(TEXT("Save retains distant structure switch"), Air->bFinishEnabled);
        Original.CheckArrays(*this, TEXT("Active preview"), true);
        Original.CheckFog(*this, TEXT("Active preview"), true);
        Active.CheckConsole(*this, TEXT("Resumed quality lease"));
    };
    auto CheckSuspended = [this, Air, &Original]()
    {
        TestTrue(TEXT("Saving suspends rather than discards preview ownership"), FYogAirFogEditorService::IsPreviewActive());
        TestTrue(TEXT("Saving preserves desired enable state"), Air->IsAirFogEnabled());
        TestFalse(TEXT("Saving removes the native carrier from presentation"), Air->Carrier->GetVisibleFlag());
        TestFalse(TEXT("Saving removes the volume bounds from presentation"), Air->VolumeBounds->GetVisibleFlag());
        Original.CheckOriginal(*this, TEXT("Save serialization boundary"));
    };

    TestFalse(TEXT("Active preview suspends the legacy presentation"), Legacy && Legacy->IsEnabled());
    TestEqual(TEXT("The real preview acquires the requested grid size"), IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.GridPixelSize"))->GetInt(), Draft.GridPixelSize);
    CheckActive();
    // Drive real module save hooks. These notifications do not write a package.
    for (const uint32 Flags : {uint32(0), uint32(SAVE_FromAutosave)})
    {
        FObjectSaveContextData Save;
        Save.Object = World;
        Save.SaveFlags = Flags;
        Save.ObjectSaveContextPhase = EObjectSaveContextPhase::PreSave;
        FEditorDelegates::PreSaveWorldWithContext.Broadcast(World, FObjectPreSaveContext(Save));
        CheckSuspended();
        Save.ObjectSaveContextPhase = EObjectSaveContextPhase::PostSave;
        FEditorDelegates::PostSaveWorldWithContext.Broadcast(World, FObjectPostSaveContext(Save));
        CheckActive();
    }
    FObjectSaveContextData Nested;
    Nested.Object = World;
    Nested.ObjectSaveContextPhase = EObjectSaveContextPhase::PreSave;
    FEditorDelegates::PreSaveExternalActors.Broadcast(World);
    FEditorDelegates::PreSaveWorldWithContext.Broadcast(World, FObjectPreSaveContext(Nested));
    CheckSuspended();
    Nested.ObjectSaveContextPhase = EObjectSaveContextPhase::PostSave;
    FEditorDelegates::PostSaveWorldWithContext.Broadcast(World, FObjectPostSaveContext(Nested));
    CheckSuspended();
    FEditorDelegates::PostSaveExternalActors.Broadcast(World);
    CheckActive();

    FYogAirFogEditorService::BeginSave(World);
    CheckSuspended();
    FYogAirFogEditorService::RecoverInterruptedSaves();
    CheckActive();

    // The preview changes material identity, not blend strength. A human weight
    // adjustment made while the temporary MID is present must survive save/revert.
    Fixture.PostProcess->Settings.WeightedBlendables.Array[0].Weight = .4f;
    for (FBlendableState& Saved : Original.Volumes)
        if (Saved.Volume.Get() == Fixture.PostProcess.Get()) Saved.Array[0].Weight = .4f;
    FEditorDelegates::PreSaveExternalActors.Broadcast(World);
    CheckSuspended();
    FEditorDelegates::PostSaveExternalActors.Broadcast(World);
    CheckActive();
    TestEqual(TEXT("Human blend weight survives a complete save cycle"), Fixture.PostProcess->Settings.WeightedBlendables.Array[0].Weight, .4f);

    FEditorDelegates::PreSaveExternalActors.Broadcast(World);
    CheckSuspended();
    FYogAirFogEditorService::RevertPreview();
    FEditorDelegates::PostSaveExternalActors.Broadcast(World);
    FYogAirFogEditorService::RecoverInterruptedSaves();
    TestFalse(TEXT("Revert during saving cannot be resurrected by PostSave"), FYogAirFogEditorService::IsPreviewActive());
    TestFalse(TEXT("Revert destroys the owned transient air actor"), PreviewIdentity.IsValid());
    TestEqual(TEXT("Revert restores the original legacy enabled state"), Legacy && Legacy->IsEnabled(), bLegacyEnabled);
    Original.CheckOriginal(*this, TEXT("Explicit revert"));
    TestEqual(TEXT("Human blend weight survives explicit revert"), Fixture.PostProcess->Settings.WeightedBlendables.Array[0].Weight, .4f);
    float CurrentFogHeight = 0.f;
    Artist->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FogHeight")), CurrentFogHeight);
    TestEqual(TEXT("The persistent artist MI was never modified"), CurrentFogHeight, OriginalFogHeight);
    OriginalArtistParameters.Check(*this, TEXT("Persistent original MI after revert"), Artist);
    OriginalArtistParameters.Check(*this, TEXT("Persistent compatibility MI after revert"), LoadObject<UMaterialInterface>(nullptr, ArtistCompatMaterialPath));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogBeginPIETest, "YogAirFog.Editor.BeginPIERestoresBeforeDuplication", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogBeginPIETest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized()) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    if (!TestFalse(TEXT("PIE regression does not take over another air fog preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
        if (!TestFalse(TEXT("PIE regression does not take over an active user actor"), It->IsAirFogEnabled())) return false;
    UYogVisionSubsystem* Legacy = World->GetSubsystem<UYogVisionSubsystem>();
    if (Legacy && !TestFalse(TEXT("PIE regression does not take over a user's legacy preview"), Legacy->HasPreviewSnapshot())) return false;
    UMaterialInterface* Artist = LoadObject<UMaterialInterface>(nullptr, ArtistMaterialPath);
    if (!TestNotNull(TEXT("Audited artist MI exists"), Artist)
        || !TestNotNull(TEXT("Prepare the real air density asset before PIE regression"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity")))
        || !TestNotNull(TEXT("Prepare the real distant finish asset before PIE regression"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish")))) return false;

    FFixture Fixture;
    const FActorSpawnParameters Spawn = FixtureSpawn();
    Fixture.PostProcess = World->SpawnActor<APostProcessVolume>(Spawn);
    Fixture.VisibleFog = World->SpawnActor<AExponentialHeightFog>(Spawn);
    Fixture.HiddenFog = World->SpawnActor<AExponentialHeightFog>(Spawn);
    if (!TestNotNull(TEXT("PIE postprocess fixture exists"), Fixture.PostProcess.Get())
        || !TestNotNull(TEXT("PIE visible fog fixture exists"), Fixture.VisibleFog.Get())
        || !TestNotNull(TEXT("PIE hidden fog fixture exists"), Fixture.HiddenFog.Get())) return false;
    Fixture.VisibleFog->GetComponent()->SetVisibility(true, false);
    Fixture.HiddenFog->GetComponent()->SetVisibility(false, false);
    Fixture.PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(.4f, Artist));
    const FSceneState Original = FSceneState::Capture(World, Fixture.PostProcess.Get(), false);
    const bool bLegacyEnabled = Legacy && Legacy->IsEnabled();

    TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
    Widget->ApplyPreset(1);
    const FYogAirFogParameters SavedDraft = Widget->GetDraftParameters();
    for (const bool bSimulating : {false, true})
    {
        FText Status;
        if (!TestTrue(TEXT("The real service activates before the PIE notification"), FYogAirFogEditorService::Preview(SavedDraft, nullptr, true, Status)))
        {
            AddError(Status.ToString());
            return false;
        }
        TWeakObjectPtr<AYogAirFogVolume> PreviewIdentity(FindPreviewActor(World));
        TestTrue(TEXT("PIE fixture owns a live transient preview"), PreviewIdentity.IsValid());
        Original.CheckArrays(*this, TEXT("Before BeginPIE"), true);
        Original.CheckFog(*this, TEXT("Before BeginPIE"), true);

        // Drive the real module delegates at the pre-duplication boundary. No
        // PIE world or package is created by this regression test.
        FEditorDelegates::BeginPIE.Broadcast(bSimulating);
        TestFalse(TEXT("BeginPIE discards service preview ownership before duplication"), FYogAirFogEditorService::IsPreviewActive());
        TestFalse(TEXT("BeginPIE destroys the transient preview actor"), PreviewIdentity.IsValid());
        TestEqual(TEXT("BeginPIE restores legacy presentation"), Legacy && Legacy->IsEnabled(), bLegacyEnabled);
        Original.CheckOriginal(*this, TEXT("BeginPIE pre-duplication state"));
        TestTrue(TEXT("BeginPIE retains every workbench draft parameter"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &SavedDraft, 0));

        FEditorDelegates::EndPIE.Broadcast(bSimulating);
        TestFalse(TEXT("EndPIE does not silently reactivate the editor preview"), FYogAirFogEditorService::IsPreviewActive());
        Original.CheckOriginal(*this, TEXT("EndPIE retains authored scene state"));
        TestTrue(TEXT("EndPIE retains every workbench draft parameter"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Widget->GetDraftParameters(), &SavedDraft, 0));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogAirFogPlacedActorBoundaryTest, "YogAirFog.Editor.PlacedActorSaveAndPIEBoundary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogAirFogPlacedActorBoundaryTest::RunTest(const FString&)
{
    using namespace AirFogEditorTests;
    if (!GEditor || GEditor->PlayWorld) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("An editor world exists"), World)) return false;
    if (!TestFalse(TEXT("Placed actor regression does not take over a service preview"), FYogAirFogEditorService::IsPreviewActive())) return false;
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
        if (!TestFalse(TEXT("Placed actor regression does not take over an active user actor"), It->IsAirFogEnabled())) return false;
    if (!TestNotNull(TEXT("Prepare the real density asset before placed actor regression"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity")))
        || !TestNotNull(TEXT("Prepare the real finish asset before placed actor regression"), LoadObject<UMaterial>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish")))) return false;

    FFixture Fixture;
    const FActorSpawnParameters Spawn = FixtureSpawn();
    Fixture.PostProcess = World->SpawnActor<APostProcessVolume>(Spawn);
    Fixture.VisibleFog = World->SpawnActor<AExponentialHeightFog>(Spawn);
    Fixture.HiddenFog = World->SpawnActor<AExponentialHeightFog>(Spawn);
    if (!TestNotNull(TEXT("Placed actor postprocess fixture exists"), Fixture.PostProcess.Get())
        || !TestNotNull(TEXT("Placed actor visible fog fixture exists"), Fixture.VisibleFog.Get())
        || !TestNotNull(TEXT("Placed actor hidden fog fixture exists"), Fixture.HiddenFog.Get())) return false;
    Fixture.VisibleFog->GetComponent()->SetVisibility(true, false);
    Fixture.HiddenFog->GetComponent()->SetVisibility(false, false);
    Fixture.PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(.37f, UMaterial::GetDefaultMaterial(MD_PostProcess)));
    // Capture authored fog before adding the test actor's own native carrier.
    const FSceneState Original = FSceneState::Capture(World, Fixture.PostProcess.Get(), false);
    FActorSpawnParameters PlacedSpawn = FixtureSpawn();
    PlacedSpawn.ObjectFlags = RF_Transactional;
    PlacedSpawn.bTemporaryEditorActor = false;
    Fixture.PlacedAir = World->SpawnActor<AYogAirFogVolume>(PlacedSpawn);
    AYogAirFogVolume* Air = Fixture.PlacedAir.Get();
    if (!TestNotNull(TEXT("A real editor placed actor exists outside the service"), Air)) return false;
    TestFalse(TEXT("Placed actor is not transient"), Air->HasAnyFlags(RF_Transient | RF_DuplicateTransient));
    TestTrue(TEXT("Placed actor retains its default BeginPlay activation policy"), Air->bAutoActivate);
    FYogAirFogParameters Parameters;
    Parameters.Density = .081f;
    Parameters.ClearRadius = 473.f;
    Air->ApplyParameters(Parameters);
    if (!TestTrue(TEXT("Direct runtime API activates the placed actor"), Air->SetAirFogEnabled(true))) return false;
    TestFalse(TEXT("Direct activation has no service preview owner"), FYogAirFogEditorService::IsPreviewActive());
    const FSceneState Active = FSceneState::Capture(World);

    auto CheckActive = [this, Air, &Original, &Active]()
    {
        TestTrue(TEXT("Save retains the placed actor's desired enable state"), Air->IsAirFogEnabled());
        TestTrue(TEXT("Save resumes the placed actor's native carrier"), Air->Carrier->GetVisibleFlag());
        TestTrue(TEXT("Save retains BeginPlay auto activation"), Air->bAutoActivate);
        Original.CheckArrays(*this, TEXT("Placed actor active"), false);
        Original.CheckFog(*this, TEXT("Placed actor active"), true);
        Active.CheckConsole(*this, TEXT("Placed actor resumed quality lease"));
    };
    CheckActive();
    // Use the actual module save notifications; no package is written.
    FEditorDelegates::PreSaveExternalActors.Broadcast(World);
    TestTrue(TEXT("Save suspension retains the placed actor's desired state"), Air->IsAirFogEnabled());
    TestFalse(TEXT("Save suspension hides the placed actor's carrier"), Air->Carrier->GetVisibleFlag());
    TestFalse(TEXT("Save suspension hides the placed actor's density volume"), Air->VolumeBounds->GetVisibleFlag());
    Original.CheckOriginal(*this, TEXT("Placed actor serialization boundary"));
    FEditorDelegates::PostSaveExternalActors.Broadcast(World);
    CheckActive();

    FEditorDelegates::BeginPIE.Broadcast(false);
    TestTrue(TEXT("BeginPIE keeps the persistent placed actor"), IsValid(Air));
    TestFalse(TEXT("BeginPIE clears API activation before duplication"), Air->IsAirFogEnabled());
    TestFalse(TEXT("BeginPIE hides the placed actor's carrier"), Air->Carrier->GetVisibleFlag());
    TestTrue(TEXT("BeginPIE preserves BeginPlay auto activation"), Air->bAutoActivate);
    TestTrue(TEXT("BeginPIE preserves all placed actor parameters"), FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Air->Parameters, &Parameters, 0));
    Original.CheckOriginal(*this, TEXT("Placed actor BeginPIE pre-duplication state"));
    FEditorDelegates::EndPIE.Broadcast(false);
    TestFalse(TEXT("EndPIE leaves the placed actor disabled in the editor"), Air->IsAirFogEnabled());
    TestTrue(TEXT("EndPIE preserves BeginPlay auto activation"), Air->bAutoActivate);
    Original.CheckOriginal(*this, TEXT("Placed actor EndPIE state"));
    TestEqual(TEXT("API activation and lifecycle hooks retain map dirty state"), World->GetOutermost()->IsDirty(), Original.bMapDirty);
    return true;
}

#endif
