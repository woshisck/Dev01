#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "SYogVisionWorkbench.h"
#include "YogVisionEditorService.h"
#include "YogVisionSubsystem.h"
#include "YogVisionSettings.h"
#include "YogVisionMotionProbe.h"
#include "YogVisionTargetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LevelEditorViewport.h"
#include "Slate/WidgetRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "ShaderCompiler.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "RenderingThread.h"
#include "UObject/ObjectSaveContext.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogVisionReadOnlyTest, "YogVision.Editor.WorkbenchReadOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogVisionReadOnlyTest::RunTest(const FString& Parameters)
{
    if (!GEditor || !FSlateApplication::IsInitialized()) return false;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    int32 BeforeActors = 0; for (TActorIterator<AActor> It(W); It; ++It) ++BeforeActors;
    const bool Dirty = W->GetOutermost()->IsDirty();
    const bool Enabled = W->GetSubsystem<UYogVisionSubsystem>()->IsEnabled();
    const FYogVisionParameters BeforeParameters = W->GetSubsystem<UYogVisionSubsystem>()->GetParameters();
    const bool DefaultEnabled = GetDefault<UYogVisionSettings>()->bEnabled;
    {
        TSharedRef<SYogVisionWorkbench> Widget = SNew(SYogVisionWorkbench);
        Widget->ApplyPreset(1);
        TestTrue(TEXT("Oil preset changes draft strokes"), Widget->GetDraftParameters().StrokeStrength > .15f);
        Widget->ApplyPreset(2); Widget->SlatePrepass(1.f);
    }
    int32 AfterActors = 0; for (TActorIterator<AActor> It(W); It; ++It) ++AfterActors;
    TestEqual(TEXT("Opening and presets create no actors"), AfterActors, BeforeActors);
    TestEqual(TEXT("Opening retains map dirty state"), W->GetOutermost()->IsDirty(), Dirty);
    TestEqual(TEXT("Opening and presets retain running enable state"), W->GetSubsystem<UYogVisionSubsystem>()->IsEnabled(), Enabled);
    TestEqual(TEXT("Opening and presets retain live parameters"), W->GetSubsystem<UYogVisionSubsystem>()->GetParameters().StrokeStrength, BeforeParameters.StrokeStrength);
    TestEqual(TEXT("Opening and presets retain saved enable default"), GetDefault<UYogVisionSettings>()->bEnabled, DefaultEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogVisionSaveRestoreTest, "YogVision.Editor.SaveRestoresPresentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogVisionSaveRestoreTest::RunTest(const FString& Parameters)
{
    if (!GEditor) return false;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    UYogVisionSubsystem* V = W ? W->GetSubsystem<UYogVisionSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Editor vision subsystem exists"), V)) return false;
    if (!TestFalse(TEXT("Save regression starts without another owned preview"), V->HasPreviewSnapshot())) return false;
    const bool bOriginalEnabled = V->IsEnabled();
    const FYogVisionParameters OriginalParameters = V->GetParameters();
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    Spawn.bTemporaryEditorActor = true;
    Spawn.bHideFromSceneOutliner = true;
    Spawn.bCreateActorPackage = false;
    AYogVisionMotionProbe* Probe = W->SpawnActor<AYogVisionMotionProbe>(Spawn);
    if (!TestNotNull(TEXT("Save regression presentation fixture exists"), Probe)) return false;
    Probe->Mesh->SetVisibility(true);
    FYogVisionParameters PreviewParameters = OriginalParameters;
    PreviewParameters.BaseSightRadius = 1234.f;
    PreviewParameters.FogDensity = .37f;
    V->SetPreviewObserver(Probe);
    V->ApplyParameters(PreviewParameters);
    V->SetEnabled(true);

    auto CheckRestoredPreview = [this, V, Probe, &PreviewParameters]()
    {
        TestTrue(TEXT("Saving resumes the previously enabled preview"), V->IsEnabled());
        TestTrue(TEXT("Saving retains the original undo snapshot"), V->HasPreviewSnapshot());
        TestEqual(TEXT("Saving keeps the same selected observer"), V->GetPreviewObserver(), static_cast<AActor*>(Probe));
        TestEqual(TEXT("Saving keeps the preview sight radius"), V->GetParameters().BaseSightRadius, PreviewParameters.BaseSightRadius);
        TestEqual(TEXT("Saving keeps the preview fog density"), V->GetParameters().FogDensity, PreviewParameters.FogDensity);
    };
    auto CheckSavingPresentation = [this, V, Probe]()
    {
        TestTrue(TEXT("Serializable body visibility is restored before writing"), Probe->Mesh->GetVisibleFlag());
        TestFalse(TEXT("Presentation is suspended only while saving"), V->IsEnabled());
        TestTrue(TEXT("Suspension does not cancel the owned preview"), V->HasPreviewSnapshot());
        TestNull(TEXT("The transient postprocess instance is removed during saving"), V->GetMaterialInstance());
    };

    // Exercise save notifications only: no files are written by this regression.
    for (const uint32 SaveFlags : { uint32(0), uint32(SAVE_FromAutosave) })
    {
        FObjectSaveContextData SaveData;
        SaveData.Object = W;
        SaveData.SaveFlags = SaveFlags;
        SaveData.ObjectSaveContextPhase = EObjectSaveContextPhase::PreSave;
        Probe->VisionTarget->UpdatePresentation(true, false);
        TestFalse(TEXT("The fixture is hidden immediately before saving"), Probe->Mesh->GetVisibleFlag());
        FEditorDelegates::PreSaveWorldWithContext.Broadcast(W, FObjectPreSaveContext(SaveData));
        CheckSavingPresentation();
        SaveData.ObjectSaveContextPhase = EObjectSaveContextPhase::PostSave;
        FEditorDelegates::PostSaveWorldWithContext.Broadcast(W, FObjectPostSaveContext(SaveData));
        CheckRestoredPreview();
    }

    // Manual saves can wrap a world-package save in an external-actor save scope.
    FObjectSaveContextData NestedSave;
    NestedSave.Object = W;
    NestedSave.ObjectSaveContextPhase = EObjectSaveContextPhase::PreSave;
    Probe->VisionTarget->UpdatePresentation(true, false);
    FEditorDelegates::PreSaveExternalActors.Broadcast(W);
    FEditorDelegates::PreSaveWorldWithContext.Broadcast(W, FObjectPreSaveContext(NestedSave));
    CheckSavingPresentation();
    NestedSave.ObjectSaveContextPhase = EObjectSaveContextPhase::PostSave;
    FEditorDelegates::PostSaveWorldWithContext.Broadcast(W, FObjectPostSaveContext(NestedSave));
    CheckSavingPresentation();
    FEditorDelegates::PostSaveExternalActors.Broadcast(W);
    CheckRestoredPreview();

    // A preview explicitly disabled before saving must not be enabled by PostSave.
    V->SetEnabled(false);
    FEditorDelegates::PreSaveExternalActors.Broadcast(W);
    FEditorDelegates::PostSaveExternalActors.Broadcast(W);
    TestFalse(TEXT("Saving preserves a deliberately disabled preview"), V->IsEnabled());
    V->SetEnabled(true);

    // Closing the workbench/reverting while suspended must cancel pending restoration.
    Probe->VisionTarget->UpdatePresentation(true, false);
    FEditorDelegates::PreSaveExternalActors.Broadcast(W);
    V->ClearPreview();
    FEditorDelegates::PostSaveExternalActors.Broadcast(W);
    TestFalse(TEXT("An explicit revert is not undone by a later PostSave"), V->HasPreviewSnapshot());
    TestEqual(TEXT("Reverting still restores the original enabled state"), V->IsEnabled(), bOriginalEnabled);
    TestEqual(TEXT("Reverting still restores the original sight radius"), V->GetParameters().BaseSightRadius, OriginalParameters.BaseSightRadius);
    TestEqual(TEXT("Reverting still restores the original fog density"), V->GetParameters().FogDensity, OriginalParameters.FogDensity);
    TestTrue(TEXT("Reverting preserves the original body visibility"), Probe->Mesh->GetVisibleFlag());
    Probe->Destroy();
    return true;
}

namespace
{
class FVisionCapture final : public IAutomationLatentCommand
{
public:
    explicit FVisionCapture(FAutomationTestBase* InTest) : Test(InTest), Began(FPlatformTime::Seconds()) {}
    ~FVisionCapture() override
    {
        if (World.IsValid()) World->GetSubsystem<UYogVisionSubsystem>()->ClearPreview();
        FlushRenderingCommands();
    }
    bool Update() override
    {
        if (FPlatformTime::Seconds() - Began > 600) { Test->AddError(TEXT("Vision effect capture timeout")); return true; }
        if (State == 0)
        {
            FText Message;
            if (!FYogVisionEditorService::BuildAssets(Message)) { Test->AddError(Message.ToString()); return true; }
            State = 1; return false;
        }
        if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { ReadyAt = FPlatformTime::Seconds() + 3; return false; }
        if (State == 1)
        {
            FText Message;
            if (!FYogVisionEditorService::CreateValidationStage(Message)) { Test->AddError(Message.ToString()); return true; }
            World = GEditor->GetEditorWorldContext().World();
            auto* V = World->GetSubsystem<UYogVisionSubsystem>(); V->SetEnabled(false);
            Directory = FPaths::ProjectSavedDir() / TEXT("VisionFog/Captures");
            IFileManager::Get().MakeDirectory(*Directory,true);
            State = 2; ReadyAt = FPlatformTime::Seconds() + 5; return false;
        }
        auto* Client = GCurrentLevelEditingViewportClient;
        if (!Client || !Client->Viewport) { Test->AddError(TEXT("No real render viewport")); return true; }
        Client->Viewport->Draw(false);
        if (FPlatformTime::Seconds() < ReadyAt) return false;
        TArray<FColor> Pixels;
        const FIntPoint Size = Client->Viewport->GetSizeXY();
        if (!Client->Viewport->ReadPixels(Pixels) || Size.GetMin() < 64) { Test->AddError(TEXT("Real viewport readback failed")); return true; }
        const TCHAR* Names[] = { TEXT("baseline"), TEXT("steel_fog"), TEXT("oil_fog"), TEXT("without_spirit"), TEXT("moving_spirit"), TEXT("blackout") };
        TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
        if (!FFileHelper::SaveArrayToFile(PNG, *(Directory / (FString(Names[State-2])+TEXT(".png"))))) { Test->AddError(TEXT("PNG save failed")); return true; }
        double Sum = 0; for (const FColor& P : Pixels) Sum += P.R + P.G + P.B;
        const double Average = Sum / (3.0 * Pixels.Num());
        Test->AddInfo(FString::Printf(TEXT("VISION_RENDER frame=%s mean=%f size=%dx%d"),Names[State-2],Average,Size.X,Size.Y));
        if (State == 2) { Baseline = Pixels; Test->TestTrue(TEXT("Baseline contains lit scene geometry, not a black or stale frame"),Average>5.0); }
        if (State == 3)
        {
            int32 Changed = 0; for (int32 I=0; I<Pixels.Num() && I<Baseline.Num(); ++I) Changed += Pixels[I] != Baseline[I];
            Test->TestTrue(TEXT("Vision shader changes the real rendered scene"), Changed > Pixels.Num()/20);
            auto* V=World->GetSubsystem<UYogVisionSubsystem>(); Test->TestNotNull(TEXT("Vision MID renders"),V->GetMaterialInstance());
            for (TActorIterator<AYogVisionMotionProbe> It(World.Get());It;++It)
                Test->TestTrue(TEXT("Wall-hidden moving target is spirit sensed"),It->VisionTarget->IsSpiritSensed());
        }
        if (State == 7) Test->TestTrue(TEXT("Blackout is preserved"), Average < 1.0);
        auto* V=World->GetSubsystem<UYogVisionSubsystem>();
        FYogVisionParameters P;
        switch (State)
        {
            case 2: V->SetEnabled(true); V->ApplyParameters(P); break;
            case 3: P.StrokeStrength=.32f;P.FlowSpeed=.25f;P.StrokeScale=1.2f;P.EdgeStrength=.15f;V->ApplyParameters(P); break;
            case 4: P.SpiritStrength=0;V->ApplyParameters(P);break;
            case 5: V->ApplyParameters(P);break;
            case 6:
                for(TActorIterator<APostProcessVolume> It(World.Get());It;++It)
                    if(!(It->GetFlags() & RF_Transient)) { It->Settings.bOverride_ColorGain=true;It->Settings.ColorGain=FVector4(0,0,0,0); }
                break;
            case 7: return CaptureWidget();
        }
        ++State; ReadyAt=FPlatformTime::Seconds()+3;return false;
    }
private:
    bool CaptureWidget()
    {
        auto Widget=SNew(SYogVisionWorkbench);
        FWidgetRenderer Renderer(false,true);
        for(const FIntPoint Size : {FIntPoint(1400,1000),FIntPoint(1000,900)})
        {
            TStrongObjectPtr<UTextureRenderTarget2D> Target(FWidgetRenderer::CreateTargetFor(FVector2D(Size),TF_Bilinear,true));
            for(int32 Draw=0;Draw<6;++Draw) { Widget->SlatePrepass(1.f);Renderer.DrawWidget(Target.Get(),Widget,1.f,FVector2D(Size),0.f); }
            FlushRenderingCommands();TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);
            if(!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags)) { Test->AddError(TEXT("Native workbench readback failed"));return true; }
            TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
            Test->TestTrue(TEXT("Native workbench PNG saved"),FFileHelper::SaveArrayToFile(PNG,*(Directory/FString::Printf(TEXT("workbench_%dx%d.png"),Size.X,Size.Y))));
        }
        return true;
    }
    FAutomationTestBase* Test;
    TWeakObjectPtr<UWorld> World;
    int32 State=0;
    double Began=0,ReadyAt=0;
    FString Directory;
    TArray<FColor> Baseline;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogVisionRenderTest,"YogVision.Render.EffectAndNativeWorkbench",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogVisionRenderTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(),TEXT("YogVisionRender"))) { AddInfo(TEXT("Requires -YogVisionRender in a dedicated real-RHI editor."));return true; }
    if(FParse::Param(FCommandLine::Get(),TEXT("NullRHI"))) { AddError(TEXT("Real RHI required"));return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FVisionCapture(this));return true;
}
#endif
