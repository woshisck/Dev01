#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "YogVisionEditorService.h"
#include "YogAirFogEditorService.h"
#include "YogAirFogVolume.h"
#include "YogPlanarFogActor.h"
#include "YogNinjaFogActor.h"
#include "SYogAirFogWorkbench.h"
#include "YogVisionSubsystem.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectIterator.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectGlobals.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

DEFINE_LOG_CATEGORY_STATIC(LogYogVisionEditor, Log, All);

namespace NinjaFogWorkbench { void Startup(); void Shutdown(); }

namespace
{
const FName AirFogWorkbenchTabId(TEXT("YogAirFogWorkbench"));
TWeakPtr<SYogAirFogWorkbench> AirFogWorkbench;

TSharedRef<SDockTab> SpawnAirFogWorkbench(const FSpawnTabArgs&)
{
    TSharedRef<SYogAirFogWorkbench> Widget = SNew(SYogAirFogWorkbench);
    AirFogWorkbench = Widget;
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[Widget];
}

void OpenAirFogWorkbench()
{
    FGlobalTabmanager::Get()->TryInvokeTab(AirFogWorkbenchTabId);
}

void BuildAirFogAssets()
{
    FText Status;
    const bool bSuccess = FYogAirFogEditorService::BuildAssets(Status);
    UE_LOG(LogYogVisionEditor, Display, TEXT("AirFog BuildAssets %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Status.ToString());
}

void PreviewAirFog()
{
    if (const TSharedPtr<SYogAirFogWorkbench> Widget = AirFogWorkbench.Pin())
    {
        Widget->PreviewAirFog();
        UE_LOG(LogYogVisionEditor, Display, TEXT("AirFog Preview: %s"), *FYogAirFogEditorService::GetPreviewStatus());
        return;
    }
    FText Status;
    const bool bSuccess = FYogAirFogEditorService::Preview(FYogAirFogParameters(), SYogAirFogWorkbench::ResolveFocusActor(), true, Status);
    UE_LOG(LogYogVisionEditor, Display, TEXT("AirFog Preview %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Status.ToString());
}

void PreviewAirFogBaseline()
{
    if (const TSharedPtr<SYogAirFogWorkbench> Widget = AirFogWorkbench.Pin())
    {
        Widget->PreviewBaseline();
        UE_LOG(LogYogVisionEditor, Display, TEXT("AirFog Baseline: %s"), *FYogAirFogEditorService::GetPreviewStatus());
        return;
    }
    FYogAirFogParameters Parameters;
    Parameters.Density = 0.f;
    FText Status;
    const bool bSuccess = FYogAirFogEditorService::Preview(Parameters, SYogAirFogWorkbench::ResolveFocusActor(), false, Status);
    UE_LOG(LogYogVisionEditor, Display, TEXT("AirFog Baseline %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Status.ToString());
}

void PreviewApprovedStaticMist()
{
    OpenAirFogWorkbench();
    if (const TSharedPtr<SYogAirFogWorkbench> Widget = AirFogWorkbench.Pin())
    {
        Widget->RefreshDrafts();
        if (Widget->SelectSavedPreset(TEXT("体积雾_世界空间粗笔刷"))) Widget->LoadDraft();
        else Widget->ApplyPreset(4);
        Widget->PreviewAirFog();
        UE_LOG(LogYogVisionEditor, Display, TEXT("AirFog ApprovedStatic: %s"), *FYogAirFogEditorService::GetPreviewStatus());
    }
}

void BuildVisionAssets()
{
    FText Status;
    const bool bSuccess = FYogVisionEditorService::BuildAssets(Status);
    UE_LOG(LogYogVisionEditor, Display, TEXT("BuildAssets %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Status.ToString());
}

void CreateVisionStage()
{
    FText Status;
    const bool bSuccess = FYogVisionEditorService::CreateValidationStage(Status);
    UE_LOG(LogYogVisionEditor, Display, TEXT("CreateStage %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Status.ToString());
}

void AuditVisionScene()
{
    FText Status;
    const bool bSuccess = FYogVisionEditorService::AuditScene(Status);
    UE_LOG(LogYogVisionEditor, Display, TEXT("AuditScene %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Status.ToString());
}
void EndEditorVisionPreviews()
{
    for (TObjectIterator<UYogVisionSubsystem> It; It; ++It)
        if (It->GetWorld() && It->GetWorld()->WorldType == EWorldType::Editor) It->ClearPreview();
}

void PrepareAirFogForPIE(bool)
{
    FYogAirFogEditorService::RevertPreview();
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World || World->WorldType != EWorldType::Editor) return;
    // API-activated placed actors also suppress authored height fog. Clear their
    // editor activation before duplication without changing BeginPlay policy.
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
        It->SetAirFogEnabled(false);
}

struct FSuspendedVisionPreview
{
    TWeakObjectPtr<UYogVisionSubsystem> System;
    TWeakObjectPtr<AActor> Observer;
    bool bWasEnabled = false;
};
TArray<FSuspendedVisionPreview> SuspendedPreviews;
TMap<TWeakObjectPtr<UWorld>, int32> SaveDepths;
bool bPlanarPIEStarting = false;
bool bPlanarNativeFogRestorePending = false;
TArray<TWeakObjectPtr<AYogPlanarFogActor>> PlanarNativeFogPreviewsBeforePIE;

bool OwnsSavingWorld(const UWorld* PreviewWorld, const UWorld* SavingWorld)
{
    return PreviewWorld && SavingWorld && (PreviewWorld == SavingWorld
        || (SavingWorld->PersistentLevel && PreviewWorld->GetLevels().Contains(SavingWorld->PersistentLevel)));
}

void UpdatePlanarLegacyFogSuspension()
{
    for (TObjectIterator<AYogPlanarFogActor> It; It; ++It)
    {
        UWorld* World = It->GetWorld();
        if (It->IsTemplate() || !World || World->WorldType != EWorldType::Editor || World->bIsTearingDown) continue;
        bool bSuspend = bPlanarPIEStarting;
        for (const auto& Pair : SaveDepths)
            if (Pair.Value > 0 && OwnsSavingWorld(World, Pair.Key.Get())) { bSuspend = true; break; }
        It->SetEditorLegacyFogOverrideSuspended(bSuspend);
    }
}

void PreparePlanarFogForPIE(bool)
{
    bPlanarNativeFogRestorePending = false;
    PlanarNativeFogPreviewsBeforePIE.Reset();
    const IConsoleVariable* FogSwitch = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"));
    if (FogSwitch && FogSwitch->GetInt() == 1)
        for (TObjectIterator<AYogPlanarFogActor> It; It; ++It)
        {
            UWorld* World = It->GetWorld();
            if (!It->IsTemplate() && World && World->WorldType == EWorldType::Editor && !World->bIsTearingDown
                && It->bMistEnabled && It->bRequestNativeFog)
                PlanarNativeFogPreviewsBeforePIE.Add(*It);
        }
    // PreBeginPIE precedes checks, optional saves, and every PIE-world duplication.
    bPlanarPIEStarting = true;
    UpdatePlanarLegacyFogSuspension();
}

void ResumePlanarFogAfterPIEPreparation()
{
    // Also called for cancelled/failed starts; nested saves remain suspended.
    bPlanarPIEStarting = false;
    UpdatePlanarLegacyFogSuspension();
}

void FinishPlanarFogPIE()
{
    ResumePlanarFogAfterPIEPreparation();
    // EndPIE precedes actor EndPlay and the performance-settings/actor CVar restores.
    // Restore only a previously enabled editor preview, after all PIE teardown finishes.
    bPlanarNativeFogRestorePending = !PlanarNativeFogPreviewsBeforePIE.IsEmpty();
}

void RestorePlanarNativeFogPreviewAfterPIE()
{
    if (!bPlanarNativeFogRestorePending || !GEditor || GEditor->PlayWorld || bPlanarPIEStarting
        || UE::IsSavingPackage() || SaveDepths.Num() > 0) return;
    bPlanarNativeFogRestorePending = false;
    TArray<TWeakObjectPtr<AYogPlanarFogActor>> Previews = MoveTemp(PlanarNativeFogPreviewsBeforePIE);
    for (const auto& Entry : Previews)
        if (AYogPlanarFogActor* Fog = Entry.Get())
        {
            UWorld* World = Fog->GetWorld();
            if (World && World->WorldType == EWorldType::Editor && !World->bIsTearingDown
                && Fog->bMistEnabled && Fog->bRequestNativeFog)
                Fog->RefreshPlanarFog();
        }
}

void ResumeCompletedVisionSaves()
{
    UpdatePlanarLegacyFogSuspension();
    for (int32 Index = SuspendedPreviews.Num() - 1; Index >= 0; --Index)
    {
        const FSuspendedVisionPreview& Saved = SuspendedPreviews[Index];
        UYogVisionSubsystem* System = Saved.System.Get();
        UWorld* World = System ? System->GetWorld() : nullptr;
        if (!World || World->bIsTearingDown || !System->HasPreviewSnapshot())
        {
            // Closing the workbench or reverting during a save must remain authoritative.
            SuspendedPreviews.RemoveAtSwap(Index);
            continue;
        }
        bool bStillSaving = false;
        for (const auto& Pair : SaveDepths)
            if (Pair.Value > 0 && OwnsSavingWorld(World, Pair.Key.Get())) { bStillSaving = true; break; }
        if (bStillSaving) continue;
        if (System->GetPreviewObserver() == Saved.Observer.Get())
            System->SetEnabled(Saved.bWasEnabled);
        SuspendedPreviews.RemoveAtSwap(Index);
    }
}

void BeginVisionSave(UWorld* SavingWorld)
{
    if (!SavingWorld) return;
    FYogAirFogEditorService::BeginSave(SavingWorld);
    ++SaveDepths.FindOrAdd(SavingWorld);
    UpdatePlanarLegacyFogSuspension();
    for (TObjectIterator<UYogVisionSubsystem> It; It; ++It)
    {
        UYogVisionSubsystem* System = *It;
        UWorld* World = System->GetWorld();
        if (!World || World->WorldType != EWorldType::Editor || !System->HasPreviewSnapshot()
            || !OwnsSavingWorld(World, SavingWorld)) continue;
        if (!SuspendedPreviews.ContainsByPredicate([System](const FSuspendedVisionPreview& Item) { return Item.System.Get() == System; }))
        {
            FSuspendedVisionPreview Saved;
            Saved.System = System;
            Saved.Observer = System->GetPreviewObserver();
            Saved.bWasEnabled = System->IsEnabled();
            SuspendedPreviews.Add(Saved);
        }
        // Removes the transient volume and restores primitive flags before serialization,
        // while retaining observer, current parameters, and the original undo snapshot.
        System->SetEnabled(false);
    }
}

void EndVisionSave(UWorld* SavingWorld)
{
    if (int32* Depth = SaveDepths.Find(SavingWorld))
    {
        if (--*Depth <= 0) SaveDepths.Remove(SavingWorld);
    }
    FYogAirFogEditorService::EndSave(SavingWorld);
    ResumeCompletedVisionSaves();
}

void RecoverInterruptedVisionSaves()
{
    // An external-actor autosave can return on failure before its PostSave delegate.
    // Saving is synchronous on the editor thread; recover on the next completed frame.
    if (SaveDepths.Num() > 0 && !UE::IsSavingPackage())
    {
        SaveDepths.Reset();
        ResumeCompletedVisionSaves();
    }
    FYogAirFogEditorService::RecoverInterruptedSaves();
    RestorePlanarNativeFogPreviewAfterPIE();
}
}

class FYogVisionEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        NinjaFogWorkbench::Startup();
        bPlanarPIEStarting = false;
        bPlanarNativeFogRestorePending = false;
        PlanarNativeFogPreviewsBeforePIE.Reset();
        UpdatePlanarLegacyFogSuspension();
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AirFogWorkbenchTabId, FOnSpawnTab::CreateStatic(&SpawnAirFogWorkbench))
            .SetDisplayName(FText::FromString(TEXT("空气雾与远景层次")))
            .SetTooltipText(FText::FromString(TEXT("调整空气体积、清雾区域和远景结构，打开页面仅建立草稿。")))
            .SetMenuType(ETabSpawnerMenuType::Hidden);
        PreSaveWorld = FEditorDelegates::PreSaveWorldWithContext.AddLambda([](UWorld* World, FObjectPreSaveContext){ BeginVisionSave(World); });
        PostSaveWorld = FEditorDelegates::PostSaveWorldWithContext.AddLambda([](UWorld* World, FObjectPostSaveContext){ EndVisionSave(World); });
        PreSaveExternal = FEditorDelegates::PreSaveExternalActors.AddStatic(&BeginVisionSave);
        PostSaveExternal = FEditorDelegates::PostSaveExternalActors.AddStatic(&EndVisionSave);
        EndFrame = FCoreDelegates::OnEndFrame.AddStatic(&RecoverInterruptedVisionSaves);
        // BeginPIE is broadcast before world duplication. Restore authored PP
        // entries and height fog now so PIE cannot inherit editor-only changes.
        BeginPIE = FEditorDelegates::BeginPIE.AddStatic(&PrepareAirFogForPIE);
        PlanarPreBeginPIE = FEditorDelegates::PreBeginPIE.AddStatic(&PreparePlanarFogForPIE);
        PlanarPostPIEStarted = FEditorDelegates::PostPIEStarted.AddLambda([](bool) { ResumePlanarFogAfterPIEPreparation(); });
        PlanarCancelPIE = FEditorDelegates::CancelPIE.AddStatic(&FinishPlanarFogPIE);
        PlanarEndPIE = FEditorDelegates::EndPIE.AddLambda([](bool) { FinishPlanarFogPIE(); });
        NinjaEndPIE = FEditorDelegates::EndPIE.AddRaw(this, &FYogVisionEditorModule::QueueNinjaFogRestoreAfterPIE);
        auto& Console = IConsoleManager::Get();
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.Open"), TEXT("Open the air fog and distant structure workbench without starting a preview."),
            FConsoleCommandDelegate::CreateStatic(&OpenAirFogWorkbench), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.BuildAssets"), TEXT("Prepare air fog volume and distant structure materials."),
            FConsoleCommandDelegate::CreateStatic(&BuildAirFogAssets), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.Preview"), TEXT("Preview the workbench draft or the default air fog parameters."),
            FConsoleCommandDelegate::CreateStatic(&PreviewAirFog), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.StaticMist"), TEXT("Open and preview the approved named static mist draft, or its built-in starting point when absent."),
            FConsoleCommandDelegate::CreateStatic(&PreviewApprovedStaticMist), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.Baseline"), TEXT("Preview the artist baseline with original fog, new air fog and distant finish disabled."),
            FConsoleCommandDelegate::CreateStatic(&PreviewAirFogBaseline), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.Revert"), TEXT("Restore the scene state before the air fog preview."),
            FConsoleCommandDelegate::CreateLambda([] { FYogAirFogEditorService::RevertPreview(); }), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.AirFog.CaptureCurrent"), TEXT("Capture the current viewport as an air fog review image."),
            FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
                { FYogAirFogEditorService::Capture(Args.Num() ? Args[0] : TEXT("airfog")); }), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.Vision.BuildAssets"), TEXT("Build the isolated vision fog material assets."),
            FConsoleCommandDelegate::CreateStatic(&BuildVisionAssets), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.Vision.CreateStage"), TEXT("Create and open an unsaved vision fog validation scene. Refuses dirty maps and PIE."),
            FConsoleCommandDelegate::CreateStatic(&CreateVisionStage), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.Vision.AuditScene"), TEXT("Read vision observers, sources and targets in the current editor scene."),
            FConsoleCommandDelegate::CreateStatic(&AuditVisionScene), ECVF_Default));
        Commands.Add(Console.RegisterConsoleCommand(TEXT("Yog.Vision.CaptureCurrent"), TEXT("Capture the active editor viewport for review."),
            FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
                { FYogVisionEditorService::CaptureCurrentView(Args.Num() ? Args[0] : TEXT("scene")); }),ECVF_Default));
    }

    virtual void ShutdownModule() override
    {
        FEditorDelegates::EndPIE.Remove(NinjaEndPIE);
        FCoreDelegates::OnEndFrame.Remove(NinjaRestoreEndFrame);
        NinjaRestoreEndFrame.Reset();
        NinjaFogWorkbench::Shutdown();
        // Leave authored actors restored even when the editor module is unloaded.
        bPlanarPIEStarting = true;
        bPlanarNativeFogRestorePending = false;
        PlanarNativeFogPreviewsBeforePIE.Reset();
        UpdatePlanarLegacyFogSuspension();
        if (const TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(AirFogWorkbenchTabId)) Tab->RequestCloseTab();
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AirFogWorkbenchTabId);
        AirFogWorkbench.Reset();
        FYogAirFogEditorService::RevertPreview();
        EndEditorVisionPreviews();
        SuspendedPreviews.Reset();
        SaveDepths.Reset();
        FEditorDelegates::PreSaveWorldWithContext.Remove(PreSaveWorld);
        FEditorDelegates::PostSaveWorldWithContext.Remove(PostSaveWorld);
        FEditorDelegates::PreSaveExternalActors.Remove(PreSaveExternal);
        FEditorDelegates::PostSaveExternalActors.Remove(PostSaveExternal);
        FCoreDelegates::OnEndFrame.Remove(EndFrame);
        FEditorDelegates::BeginPIE.Remove(BeginPIE);
        FEditorDelegates::PreBeginPIE.Remove(PlanarPreBeginPIE);
        FEditorDelegates::PostPIEStarted.Remove(PlanarPostPIEStarted);
        FEditorDelegates::CancelPIE.Remove(PlanarCancelPIE);
        FEditorDelegates::EndPIE.Remove(PlanarEndPIE);
        for (IConsoleObject* Command : Commands)
        {
            if (Command) IConsoleManager::Get().UnregisterConsoleObject(Command);
        }
        Commands.Empty();
    }

private:
    void QueueNinjaFogRestoreAfterPIE(bool)
    {
        // EndPIE precedes actor EndPlay and GameInstance CVar restoration. Wait
        // until this frame finishes, then refresh the editor preview only once.
        FCoreDelegates::OnEndFrame.Remove(NinjaRestoreEndFrame);
        NinjaRestoreEndFrame = FCoreDelegates::OnEndFrame.AddRaw(this, &FYogVisionEditorModule::RestoreNinjaFogAfterPIE);
    }

    void RestoreNinjaFogAfterPIE()
    {
        FCoreDelegates::OnEndFrame.Remove(NinjaRestoreEndFrame);
        NinjaRestoreEndFrame.Reset();
        if (!GEditor || GEditor->PlayWorld) return;
        UWorld* World = GEditor->GetEditorWorldContext().World();
        if (!World || World->WorldType != EWorldType::Editor || World->bIsTearingDown) return;
        for (TActorIterator<AYogNinjaFogActor> It(World); It; ++It)
            if (!It->IsTemplate() && It->bFogEnabled && It->bRequestNativeFog)
                It->RefreshNinjaFog();
    }

    TArray<IConsoleObject*> Commands;
    FDelegateHandle PreSaveWorld, PostSaveWorld, PreSaveExternal, PostSaveExternal, EndFrame, BeginPIE;
    FDelegateHandle PlanarPreBeginPIE, PlanarPostPIEStarted, PlanarCancelPIE, PlanarEndPIE;
    FDelegateHandle NinjaEndPIE, NinjaRestoreEndFrame;
};

IMPLEMENT_MODULE(FYogVisionEditorModule, YogVisionEditor)
