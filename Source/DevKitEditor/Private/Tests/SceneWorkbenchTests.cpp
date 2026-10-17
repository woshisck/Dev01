#if WITH_DEV_AUTOMATION_TESTS
#include "Tools/SceneWorkbench/SDevKitSceneWorkbenchWidget.h"
#include "Tools/SceneWorkbench/SDevKitSceneRoomConfigWidget.h"
#include "Data/RoomDataAsset.h"
#include "LevelEnvironment/YogLevelEnvironment.h"
#include "StylizedLightingSettings.h"
#include "UObject/UnrealType.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Text/STextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneStyleViewFamilyTest,
    "DevKit.SceneWorkbench.StyleViewFamilyIntegration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSceneStyleViewFamilyTest::RunTest(const FString& Parameters)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!TestNotNull(TEXT("Editor world"), World) || !TestNotNull(TEXT("Editor scene"), World->Scene)) return false;
    UYogRenderStyleSettings* Settings = GetMutableDefault<UYogRenderStyleSettings>();
    const FYogNativeRenderStyleSettings Previous = Settings->Style;
    Settings->Style = FYogNativeRenderStyleSettings();
    Settings->Style.GlobalContrast = 1.1f;
    // Old configs can carry an opted-in grading flag. The retired extension
    // must preserve authored postprocess even when constructed explicitly.
    Settings->Style.bEnablePostProcessGrading = true;
    const TSharedRef<FSceneViewExtensionBase, ESPMode::ThreadSafe> Extension = CreateYogNativeRenderStyleExtension();
    FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(nullptr, World->Scene, FEngineShowFlags(ESFIM_Game)));
    Family.ViewMode = VMI_Lit;
    FSceneViewInitOptions Options;
    Options.ViewFamily = &Family;
    Options.SetViewRectangle(FIntRect(0, 0, 64, 64));
    Options.ViewRotationMatrix = FMatrix::Identity;
    Options.ProjectionMatrix = FMatrix::Identity;
    FSceneView* View = new FSceneView(Options);
    Family.Views.Add(View);
    View->FinalPostProcessSettings.ColorContrast.W = 1.17f;
    View->FinalPostProcessSettings.AutoExposureBias = 2.5f;
    View->FinalPostProcessSettings.FilmSlope = .82f;
    Extension->BeginRenderViewFamily(Family);
    TestEqual(TEXT("Retired grading preserves authored Lit contrast"), View->FinalPostProcessSettings.ColorContrast.W, 1.17);
    TestEqual(TEXT("Retired grading preserves authored camera exposure"), View->FinalPostProcessSettings.AutoExposureBias, 2.5f);
    TestEqual(TEXT("Retired grading preserves the authored film curve"), View->FinalPostProcessSettings.FilmSlope, .82f);
    View->FinalPostProcessSettings.ColorContrast.W = 1.f;
    Family.ViewMode = VMI_LightingOnly;
    Extension->BeginRenderViewFamily(Family);
    TestEqual(TEXT("Diagnostic lighting is unchanged"), View->FinalPostProcessSettings.ColorContrast.W, 1.0);
    Family.ViewMode = VMI_Lit;
    View->bIsSceneCapture = true;
    Extension->BeginRenderViewFamily(Family);
    TestEqual(TEXT("Scene capture is unchanged"), View->FinalPostProcessSettings.ColorContrast.W, 1.0);
    View->bIsSceneCapture = false;
    View->bIsReflectionCapture = true;
    Extension->BeginRenderViewFamily(Family);
    TestEqual(TEXT("Reflection capture is unchanged"), View->FinalPostProcessSettings.ColorContrast.W, 1.0);
    View->bIsReflectionCapture = false;
    Family.EngineShowFlags.SetVisualizeBuffer(true);
    Extension->BeginRenderViewFamily(Family);
    TestEqual(TEXT("Buffer diagnostics remain native"), View->FinalPostProcessSettings.ColorContrast.W, 1.0);
    Settings->Style = Previous;
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneEnvironmentInheritanceTest,
    "DevKit.SceneWorkbench.NativeEnvironmentMetadata", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSceneEnvironmentInheritanceTest::RunTest(const FString& Parameters)
{
    const TSharedRef<SDevKitSceneWorkbenchWidget> Widget = SNew(SDevKitSceneWorkbenchWidget);
    // Empty notes do not create a compatibility Actor or touch production data.
    if (UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
    {
        int32 EnvironmentActorCount = 0;
        bool bHasPersistentActor = false;
        for (TActorIterator<AYogLevelEnvironment> It(World); It; ++It)
        {
            ++EnvironmentActorCount;
            bHasPersistentActor |= It->GetLevel() == World->PersistentLevel;
        }
        if (!bHasPersistentActor)
        {
            const bool bWasDirty = World->GetPackage()->IsDirty();
            Widget->Selected = MakeShared<FDevKitSceneAssetRow>();
            Widget->Selected->Map = FAssetData(World);
            Widget->Selected->Name = World->GetName();
            Widget->Selected->Path = World->GetPackage()->GetName();
            Widget->LoadEnvironmentDraft();
            Widget->bEnvironmentDraftDirty = true;
            Widget->ApplyEnvironment();
            int32 AfterActorCount = 0;
            for (TActorIterator<AYogLevelEnvironment> It(World); It; ++It) ++AfterActorCount;
            TestEqual(TEXT("Empty notes do not create an Actor"), AfterActorCount, EnvironmentActorCount);
            TestEqual(TEXT("Empty notes do not dirty the map"), World->GetPackage()->IsDirty(), bWasDirty);
            TestFalse(TEXT("Empty-note Apply clears the draft state"), Widget->bEnvironmentDraftDirty);
        }
    }
    Widget->EnvironmentDraft.Reset(NewObject<UYogLevelEnvironmentComponent>());
    UYogLevelEnvironmentComponent* Draft = Widget->EnvironmentDraft.Get();
    TStrongObjectPtr<UYogLevelEnvironmentProfile> Profile(NewObject<UYogLevelEnvironmentProfile>());
    Profile->Settings.ColorLayerCount = 9;
    Draft->Profile = Profile.Get();
    Draft->Settings.ColorLayerCount = 7;
    Draft->Notes = TEXT("Preserve dormant notes");
    Draft->Priority = 7;
    TestFalse(TEXT("Retired render values cannot be edited"), Widget->CanEditEnvironmentValues());
    Widget->SetEnvironmentOverride(true);
    TestFalse(TEXT("Legacy helper cannot enable an override"), Draft->bOverrideGlobalSettings);
    TestEqual(TEXT("Legacy helper does not rewrite serialized render data"), Draft->Settings.ColorLayerCount, 7);
    Widget->RestoreGlobalEnvironment();
    TestFalse(TEXT("Default cannot reactivate retired render editing"), Widget->CanEditEnvironmentValues());
    TestEqual(TEXT("Default retains dormant local layer settings"), Draft->Settings.ColorLayerCount, 7);
    TestEqual(TEXT("Default retains notes"), Draft->Notes, FString(TEXT("Preserve dormant notes")));
    TestEqual(TEXT("Default retains priority"), Draft->Priority, 7);
    TestTrue(TEXT("Default retains profile reference"), Draft->Profile == Profile.Get());

    Widget->SetNativeEnvironment();
    TestFalse(TEXT("Native helper does not create rendering overrides"), Draft->bOverrideGlobalSettings);
    TestFalse(TEXT("Retired global or local settings have no render effect"), Widget->GetEnvironmentDraftSettings().HasRenderEffect());
    Draft->bOverrideGlobalSettings = true;
    Draft->bOverrideProfile = true;
    Draft->Settings.bEnableColorLayers = true;
    Draft->Settings.bColorLayersAffectCharacters = true;
    Draft->Settings.bColorLayersAffectScene = true;
    TestFalse(TEXT("Old serialized enabled settings remain inert"), Widget->GetEnvironmentDraftSettings().HasRenderEffect());
    TestFalse(TEXT("Old override flags cannot unlock rendering controls"), Widget->CanEditEnvironmentValues());
    TestTrue(TEXT("Level panel describes native rendering rather than active legacy overrides"),
        Widget->GetEnvironmentSourceText().ToString().Contains(TEXT("旧关卡环境中的分层与曝光参数不再参与渲染")));
    Widget->SaveEnvironmentPreset();
    TestTrue(TEXT("Retired preset helper does not claim to save an asset"), Widget->Status.ToString().Contains(TEXT("未创建或保存任何资产")));
    if (UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
    {
        // Inspect widgets only; no Apply or asset save.
        Widget->Selected = MakeShared<FDevKitSceneAssetRow>();
        Widget->Selected->Map = FAssetData(World);
        Widget->Selected->Name = World->GetName();
        Widget->Selected->Path = World->GetPackage()->GetName();
        Widget->LoadEnvironmentDraft();
        const TSharedRef<SWidget> Panel = Widget->MakeEnvironmentPanel();
        FString Labels;
        TFunction<void(TSharedRef<SWidget>)> CollectLabels = [&](TSharedRef<SWidget> Child)
        {
            if (Child->GetType() == TEXT("STextBlock")) Labels += StaticCastSharedRef<STextBlock>(Child)->GetText().ToString() + TEXT("\n");
            FChildren* Children = Child->GetChildren();
            for (int32 Index = 0; Children && Index < Children->Num(); ++Index) CollectLabels(Children->GetChildAt(Index));
        };
        CollectLabels(Panel);
        for (const TCHAR* Field : {TEXT("启用统一颜色分层"), TEXT("参考层数"), TEXT("分层边界柔和度"),
            TEXT("分层强度"), TEXT("分层影响角色"), TEXT("分层影响场景")})
            TestFalse(*FString::Printf(TEXT("Retired layer field is absent: %s"), Field), Labels.Contains(Field));
        TestTrue(TEXT("Environment notes remain editable"), Labels.Contains(TEXT("环境备忘")));
        TestTrue(TEXT("Light and sublevel resources remain discoverable"), Labels.Contains(TEXT("灯光 / 子关卡资源")));
        TestFalse(TEXT("Render preset creation is no longer offered"), Labels.Contains(TEXT("将当前参数保存为环境预设")));
        TestFalse(TEXT("Legacy global override is no longer offered"), Labels.Contains(TEXT("覆盖全局默认（本关卡）")));
        TestFalse(TEXT("Level panel no longer offers character exposure"), Labels.Contains(TEXT("角色曝光补偿 / EV")));
        TestFalse(TEXT("Level panel no longer offers dark exposure range"), Labels.Contains(TEXT("分层暗部范围 / EV")));
        TestFalse(TEXT("Level panel no longer offers bright exposure range"), Labels.Contains(TEXT("分层亮部范围 / EV")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenePrimaryMapFilterTest,
    "DevKit.SceneWorkbench.PrimaryMapFilter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FScenePrimaryMapFilterTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Scene map root stays fixed"), SDevKitSceneWorkbenchWidget::GetMapDataRoot(), FString(TEXT("/Game/Art/Map/Map_Data")));
    const FString Root = SDevKitSceneWorkbenchWidget::GetMapDataRoot();
    // Actual persistent-map names and folders: corridor folders do not match
    // their map names, and InitialRoom does not have the standard L1_ prefix.
    const TArray<FString> PrimaryMaps{
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L1_CommonLevel_corridor_01b"),
        TEXT("L1_CommonLevel_corridor_S_Goth/L1_CommonLevel_Corridor_01a"),
        TEXT("L1_CommonLevel_EmbalmingChamber/L1_CommonLevel_EmbalmingChamber"),
        TEXT("L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom"),
        TEXT("L1_CommonLevel_Prison_S_02/L1_CommonLevel_Prison_S_02"),
        TEXT("L1_CommonLevel_Prison_S_03/L1_CommonLevel_Prison_S_03"),
        TEXT("L1_CommonLevel_Prison_S_09/L1_CommonLevel_Prison_S_09"),
        TEXT("L1_CommonLevel_WaterDungeon/L1_CommonLevel_WaterDungeon"),
        TEXT("L1_InitialRoom/InitialRoom")};
    for (const FString& RelativePath : PrimaryMaps)
        TestTrue(TEXT("Persistent map is listed: ") + RelativePath, SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Root / RelativePath));

    const TArray<FString> Sublevels{
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L_Corridor_01b_Art"),
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L_Corridor_01b_Bake"),
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L_Corridor_01b_DataBake"),
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L_Corridor_01b_Gameplay"),
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L_Corridor_01b_Light"),
        TEXT("L1_CommonLevel_corridor_S_Dungeon/L_Corridor_01b_PLA"),
        TEXT("L1_CommonLevel_Prison_S_03/L_Prison_S_03a_Art"),
        TEXT("L1_CommonLevel_Prison_S_03/L_Prison_S_03_VFX"),
        TEXT("L1_InitialRoom/InitialRoom_GamePlay"),
        TEXT("L1_CommonLevel_Corridor_New/L1_CommonLevel_Corridor_New_Batched"),
        TEXT("L2_CommonLevel_City/L2_CommonLevel_City_light"),
        TEXT("L3_CommonLevel_Church/L3_CommonLevel_Church_DATABAKE")};
    for (const FString& RelativePath : Sublevels)
        TestFalse(TEXT("Sublevel stays out of the primary list: ") + RelativePath, SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Root / RelativePath));

    const TArray<FString> OutOfScope{
        TEXT("/Game/Art/Map/LookDev/LookDevMain"),
        TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp"),
        TEXT("/Game/Art/Map/Test/L1_CommonLevel_Corridor_01a"),
        TEXT("/Game/Art/Map/Map_Data_Test/L1_CommonLevel_Corridor_01a"),
        TEXT("/Game/Art/Map/Map_DataBackup/L1_CommonLevel_Corridor_01a"),
        TEXT("/Game/Test/Art/Map/Map_Data/L1_CommonLevel_Corridor_01a"),
        TEXT("/Plugin/Art/Map/Map_Data/L1_CommonLevel_Corridor_01a"),
        TEXT("/Game/Art/Map/Map_Data"),
        TEXT("/Game/Art/Map/Map_Data/L1_Test.L1_Test"),
        TEXT("/Game/Art/Map/Map_Data/../Test/L1_Test"),
        TEXT("")};
    for (const FString& Package : OutOfScope)
        TestFalse(TEXT("Outside or invalid package is excluded: ") + Package, SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Package));
    TestTrue(TEXT("A primary map can use the Map_Data root itself"), SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Root / TEXT("L2_CommonLevel_City")));
    TestTrue(TEXT("Nested Map_Data folders stay supported"), SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Root / TEXT("L3/Church/L3_CommonLevel_Church")));
    TestTrue(TEXT("Only terminal child suffixes are excluded"), SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Root / TEXT("L2_CommonLevel_Art_District")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneRoomAssociationTest,
    "DevKit.SceneWorkbench.Association", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSceneRoomAssociationTest::RunTest(const FString& Parameters)
{
    UWorld* Map = NewObject<UWorld>(GetTransientPackage(), TEXT("SceneAssociationMap"));
    URoomDataAsset* Room = NewObject<URoomDataAsset>();
    FAssetData Asset(Map);
    Room->RoomName = Map->GetFName();
    TestTrue(TEXT("Unique legacy name stays compatible"), SDevKitSceneWorkbenchWidget::DoesRoomMatchMap(Room, Asset, true));
    TestFalse(TEXT("Ambiguous legacy name is not auto-associated"), SDevKitSceneWorkbenchWidget::DoesRoomMatchMap(Room, Asset, false));
    Room->LevelAsset = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Explicit.Map")));
    TestFalse(TEXT("Explicit map association takes precedence over legacy name"), SDevKitSceneWorkbenchWidget::DoesRoomMatchMap(Room, Asset, true));
    Room->LevelAsset = Map;
    TestTrue(TEXT("Explicit map works even if legacy name is ambiguous"), SDevKitSceneWorkbenchWidget::DoesRoomMatchMap(Room, Asset, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneWorkbenchNativeTest,
    "DevKit.SceneWorkbench.NativeExplorer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSceneWorkbenchNativeTest::RunTest(const FString& Parameters)
{
    const TSharedRef<SDevKitSceneWorkbenchWidget> Widget = SNew(SDevKitSceneWorkbenchWidget);
    Widget->SlatePrepass();
    TestTrue(TEXT("Native explorer has usable content"), Widget->GetDesiredSize().X > 0 && Widget->GetDesiredSize().Y > 0);
    // Refreshing a map's associations must not switch the room that SaveSelected
    // will save. In particular, a newly associated second variant stays selected.
    TStrongObjectPtr<URoomDataAsset> FirstRoom(NewObject<URoomDataAsset>());
    TStrongObjectPtr<URoomDataAsset> AssociatedRoom(NewObject<URoomDataAsset>());
    auto BeforeRefresh = MakeShared<FDevKitSceneAssetRow>();
    BeforeRefresh->Path = TEXT("/Game/TransientSelectionTest");
    BeforeRefresh->Rooms = {FirstRoom.Get(), AssociatedRoom.Get()};
    Widget->SelectAsset(BeforeRefresh, ESelectInfo::Direct);
    Widget->SelectedRoom = AssociatedRoom.Get();
    auto AfterRefresh = MakeShared<FDevKitSceneAssetRow>(*BeforeRefresh);
    Widget->SelectAsset(AfterRefresh, ESelectInfo::Direct);
    TestEqual(TEXT("Refreshing same map preserves the associated room to save"), Widget->SelectedRoom.Get(), AssociatedRoom.Get());
    Widget->RefreshAssets();
    for (const auto& Row : Widget->AllRows)
    {
        TestTrue(TEXT("Explorer contains only map rows, not orphan RoomData"), Row->Map.IsValid());
        TestTrue(TEXT("Every explorer row is a Map_Data primary map"), SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(Row->Path));
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("SceneWorkbenchRender")))
    {
        constexpr int32 Width = 1600, Height = 1000;
        const FString Directory = FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports");
        IFileManager::Get().MakeDirectory(*Directory, true);
        auto RenderPage = [&](const FString& Filename)
        {
            FWidgetRenderer Renderer(false);
            UTextureRenderTarget2D* Target = nullptr;
            for (int32 Frame = 0; Frame < 3; ++Frame)
            {
                Widget->SlatePrepass(); Target = Renderer.DrawWidget(Widget, FVector2D(Width, Height)); FlushRenderingCommands();
            }
            if (!TestNotNull(TEXT("Native component target"), Target)) return false;
            TArray<FColor> Pixels;
            FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
            if (!TestTrue(TEXT("Native component pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags))) return false;
            for (FColor& Pixel : Pixels) Pixel = Pixel.ReinterpretAsLinear().ToFColor(true);
            TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Width, Height, Pixels, Png);
            return TestTrue(TEXT("Native explorer PNG saved"), FFileHelper::SaveArrayToFile(Png, *(Directory / Filename)));
        };
        if (!RenderPage(TEXT("SceneWorkbench.png"))) return false;
        // Preview the real environment form without applying or saving any production data.
        if (UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
        {
            Widget->Selected = MakeShared<FDevKitSceneAssetRow>();
            Widget->Selected->Map = FAssetData(World);
            Widget->Selected->Name = World->GetName();
            Widget->Selected->Path = World->GetPackage()->GetName();
            Widget->ActivePage = 2;
            Widget->LoadEnvironmentDraft();
            Widget->RebuildRightPanel();
            if (!RenderPage(TEXT("SceneEnvironment.png"))) return false;
        }
    }
    return true;
}
#endif
