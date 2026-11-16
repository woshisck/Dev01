#include "SYogVisionWorkbench.h"
#include "YogVisionWorkbenchDraft.h"
#include "YogVisionEditorService.h"
#include "YogVisionSubsystem.h"
#include "YogVisionSourceComponent.h"
#include "YogVisionTargetComponent.h"
#include "YogVisionObserverComponent.h"

#include "Components/LocalLightComponent.h"
#include "Components/SceneComponent.h"
#include "DetailsViewArgs.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Framework/Docking/TabManager.h"
#include "IDetailsView.h"
#include "LevelEditorViewport.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "YogVisionWorkbench"

namespace
{
TSharedRef<SWidget> MakeSection(const FText& Title, TSharedRef<SWidget> Content)
{
    return SNew(SExpandableArea)
        .InitiallyCollapsed(false)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .HeaderPadding(FMargin(10.f, 7.f)).Padding(FMargin(10.f, 7.f))
        .HeaderContent()[SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle("BoldFont"))]
        .BodyContent()[Content];
}

TSharedRef<IDetailsView> MakeDetails(FName Identifier, bool bSearch)
{
    FDetailsViewArgs Args;
    Args.ViewIdentifier = Identifier;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    Args.bUpdatesFromSelection = false;
    Args.bLockable = false;
    Args.bAllowSearch = bSearch;
    Args.bHideSelectionTip = true;
    Args.bShowObjectLabel = false;
    Args.bShowOptions = false;
    Args.bShowSectionSelector = false;
    Args.bShowPropertyMatrixButton = false;
    Args.bShowScrollBar = false;
    Args.bShowLooseProperties = false;
    Args.ColumnWidth = .55f;
    return FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);
}
}

void SYogVisionWorkbench::Construct(const FArguments& InArgs)
{
    BuildAssetsDelegate = InArgs._OnBuildAssets;
    CreateStageDelegate = InArgs._OnCreateStage;
    Draft.Reset(NewObject<UYogVisionWorkbenchDraft>(GetTransientPackage(), NAME_None, RF_Transient | RF_Transactional));
    const UYogVisionSettings* Defaults = GetDefault<UYogVisionSettings>();
    Draft->Parameters = Defaults->Parameters;
    Draft->bEnabled = Defaults->bEnabled;
    if (GEditor) GEditor->RegisterForUndo(this);
    DraftDetails = MakeDetails(TEXT("YogVision.Draft"), true);
    DraftDetails->SetObject(Draft.Get());
    DraftDetails->OnFinishedChangingProperties().AddSP(this, &SYogVisionWorkbench::OnDraftChanged);
    SelectionDetails = MakeDetails(TEXT("YogVision.Selection"), false);
    SelectionDetails->SetDisableCustomDetailLayouts(true);
    SelectionDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Property)
    {
        const UStruct* Owner = Property.Property.GetOwnerStruct();
        return Owner && (Owner == UYogVisionSourceComponent::StaticClass() ||
            Owner == UYogVisionTargetComponent::StaticClass() || Owner == UYogVisionObserverComponent::StaticClass());
    }));
    Status = LOCTEXT("DraftReady", "项目默认已读入草稿。选择观察者后，点击预览开始调整。");
    RefreshReadback();

    ChildSlot
    [
        SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Background")).Padding(12.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
            [SNew(STextBlock).Text(LOCTEXT("Title", "视野迷雾与灵视")).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [SNew(STextBlock).Text(LOCTEXT("Intro", "光照打开视野 · 远景沉入浓雾 · 屏幕笔触与移动雾团"))
                .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [SNew(SButton).Text(LOCTEXT("OpenAirFogWorkbench", "打开空气雾与远景层次"))
                .ToolTipText(LOCTEXT("OpenAirFogWorkbenchTip", "打开独立空气雾草稿页面，分别观察空气体积与远景结构。"))
                .OnClicked_Lambda([]
                {
                    FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("YogAirFogWorkbench")));
                    return FReply::Handled();
                })]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SNew(SSplitter)
                + SSplitter::Slot().Value(.36f).MinSize(270.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 6.f, 8.f)
                        [MakeSection(LOCTEXT("Preview", "观察与预览"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 7.f)
                            [SNew(STextBlock).Text(this, &SYogVisionWorkbench::GetSelectionLabel).AutoWrapText(true)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("PreviewLevel", "预览当前关卡"), LOCTEXT("PreviewTooltip", "以选中Actor为观察者；未选择时使用玩家起点，再回退到视口面向的地面。调节草稿会实时更新已开启的预览。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::PreviewCurrentLevel))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("ApplyPIE", "应用到 PIE"), LOCTEXT("ApplyPIETooltip", "将当前草稿临时应用到正在运行的游戏，以玩家作为观察者。退出游戏后临时结果自然清理。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::ApplyToPIE))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Revert", "撤销预览"), LOCTEXT("RevertTooltip", "恢复开始预览前的参数和启用状态，清除临时观察者。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::RevertPreview))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 7.f, 0.f, 0.f)
                            [SNew(STextBlock).Text(this, &SYogVisionWorkbench::GetRuntimeStatus).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())])]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 6.f, 8.f)
                        [MakeSection(LOCTEXT("Preset", "表现起点"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("PenPreset", "克制钢笔雾"), LOCTEXT("PenTooltip", "浓重远景雾，细密的屏幕钢笔笔触与克制的移动雾团。"), FOnClicked::CreateLambda([this] { ApplyPreset(0); return FReply::Handled(); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("OilPreset", "流动油画雾"), LOCTEXT("OilTooltip", "宽大的屏幕油画笔触、更厚的远景雾与带尾迹的移动雾团。"), FOnClicked::CreateLambda([this] { ApplyPreset(1); return FReply::Handled(); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("ClearPreset", "调试清晰"), LOCTEXT("ClearTooltip", "降低雾与笔触，以检查直接视野和灵视范围。"), FOnClicked::CreateLambda([this] { ApplyPreset(2); return FReply::Handled(); }))])]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 6.f, 8.f)
                        [MakeSection(LOCTEXT("SelectedSetup", "场景标注"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("AddLightSource", "为选中灯光添加视野源"), LOCTEXT("AddSourceTooltip", "给选中Actor添加火光视野源。有灯光时自动绑定其有效范围；可撤销。"), FOnClicked::CreateLambda([this] { return AddSelectedComponent(UYogVisionSourceComponent::StaticClass()); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("AddObserver", "标注选中角色为观察者"), LOCTEXT("AddObserverTooltip", "添加观察者组件，供道具叠加直接视野与灵视感知。可撤销。"), FOnClicked::CreateLambda([this] { return AddSelectedComponent(UYogVisionObserverComponent::StaticClass()); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("AddTarget", "标注选中敌人为感知目标"), LOCTEXT("AddTargetTooltip", "添加灵视感知目标组件，迷雾中呈现随移动变化的扰动雾团。可撤销。"), FOnClicked::CreateLambda([this] { return AddSelectedComponent(UYogVisionTargetComponent::StaticClass()); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 7.f, 0.f, 0.f)
                            [SNew(STextBlock).Text(this, &SYogVisionWorkbench::GetSceneReadback).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
                            [SelectionDetails.ToSharedRef()])]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 6.f, 8.f)
                        [MakeSection(LOCTEXT("Validation", "验证与保存"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Stage", "新建验证场景"), LOCTEXT("StageTooltip", "打开独立验证场景，比较角色提亮、遮挡、火光退散与移动灵体。请先保存当前关卡。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::CreateStage))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Assets", "准备迷雾表现资源"), LOCTEXT("AssetsTooltip", "准备本功能的迷雾材质和笔触纹理。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::BuildAssets))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("SaveDefaults", "保存项目默认"), LOCTEXT("SaveTooltip", "将右侧草稿和游戏默认启用开关写入项目默认。后续新运行的游戏使用这些值。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::SaveProjectDefaults))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("ReloadDefaults", "重新读入项目默认"), LOCTEXT("ReloadTooltip", "用项目默认替换当前草稿；已开启的预览随之更新。"), FOnClicked::CreateSP(this, &SYogVisionWorkbench::ReloadProjectDefaults))])]
                    ]
                ]
                + SSplitter::Slot().Value(.64f).MinSize(340.f)
                [
                    SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(7.f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(4.f, 5.f, 4.f, 8.f)
                        [SNew(STextBlock).Text(LOCTEXT("Parameters", "美术草稿"))
                            .Font(FAppStyle::GetFontStyle("BoldFont"))]
                        + SVerticalBox::Slot().FillHeight(1.f)
                        [SNew(SScrollBox) + SScrollBox::Slot()[DraftDetails.ToSharedRef()]]
                    ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(4.f, 10.f, 4.f, 2.f)
            [SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true)]
        ]
    ];
}

SYogVisionWorkbench::~SYogVisionWorkbench()
{
    if (GEditor) GEditor->UnregisterForUndo(this);
    CleanupPreviews();
}

void SYogVisionWorkbench::Tick(const FGeometry& Geometry, double Time, float DeltaTime)
{
    SCompoundWidget::Tick(Geometry, Time, DeltaTime);
    ReadbackAccumulator += DeltaTime;
    if (ReadbackAccumulator >= .5f)
    {
        ReadbackAccumulator = 0.f;
        RefreshReadback();
    }
}

const FYogVisionParameters& SYogVisionWorkbench::GetDraftParameters() const
{
    return Draft->Parameters;
}

void SYogVisionWorkbench::ApplyPreset(int32 PresetIndex)
{
    if (!Draft.IsValid()) return;
    const FScopedTransaction Transaction(LOCTEXT("ApplyVisionPreset", "调整视野美术预设"));
    Draft->Modify();
    FYogVisionParameters& P = Draft->Parameters;
    P = FYogVisionParameters();
    switch (PresetIndex)
    {
    case 1:
        P.FogDensity = .86f; P.FogContrast = .13f; P.FogBrightness = .17f;
        P.EdgeStrength = .25f; P.StrokeStrength = .28f; P.StrokeScale = 1.2f; P.FlowSpeed = .18f;
        P.BoundaryWidth = 260.f; P.BoundaryNoise = .34f; P.PlayerLift = .22f;
        P.DistanceFogStart = 800.f; P.DistanceFogEnd = 3000.f; P.DistanceFogDensity = .96f;
        P.SpiritDisturbance = 1.2f; P.SpiritVolumeHeight = 220.f; P.SpiritTrailLength = 300.f;
        P.FogColor = FLinearColor(.075f, .12f, .19f, 1.f);
        Status = LOCTEXT("OilApplied", "流动油画雾已读入草稿。");
        break;
    case 2:
        P.FogDensity = .28f; P.FogContrast = .65f; P.FogBrightness = .14f;
        P.DarkFogStrength = .10f; P.EdgeStrength = .12f; P.StrokeStrength = 0.f;
        P.FlowSpeed = 0.f; P.BoundaryNoise = 0.f; P.PlayerLift = .12f;
        P.SpiritStrength = 1.f;
        P.DistanceFogDensity = 0.f;
        P.SpiritDisturbance = .5f; P.SpiritVolumeHeight = 180.f; P.SpiritTrailLength = 140.f;
        Status = LOCTEXT("ClearApplied", "调试清晰已读入草稿。");
        break;
    default:
        P.FogDensity = .80f; P.FogContrast = .16f; P.FogBrightness = .15f;
        P.EdgeStrength = .40f; P.StrokeStrength = .12f; P.StrokeScale = 2.f; P.FlowSpeed = .08f;
        P.BoundaryWidth = 180.f; P.BoundaryNoise = .18f;
        P.DistanceFogStart = 1000.f; P.DistanceFogEnd = 3600.f; P.DistanceFogDensity = .92f;
        P.SpiritDisturbance = .85f; P.SpiritVolumeHeight = 180.f; P.SpiritTrailLength = 220.f;
        Status = LOCTEXT("PenApplied", "克制钢笔雾已读入草稿。");
        break;
    }
    P.Sanitize();
    DraftDetails->ForceRefresh();
    UpdateActivePreviews();
}

UWorld* SYogVisionWorkbench::GetEditorWorld() const
{
    return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

AActor* SYogVisionWorkbench::GetSelectedActor() const
{
    return GEditor && GEditor->GetSelectedActors() ? Cast<AActor>(GEditor->GetSelectedActors()->GetTop(AActor::StaticClass())) : nullptr;
}

AActor* SYogVisionWorkbench::ResolvePreviewObserver(UWorld* World)
{
    if (AActor* Selected = GetSelectedActor(); Selected && Selected->GetWorld() == World) return Selected;
    for (TActorIterator<APlayerStart> It(World); It; ++It) return *It;
    if (TransientObserver.IsValid() && TransientObserver->GetWorld() == World) return TransientObserver.Get();

    FVector GroundPosition = FVector::ZeroVector;
    if (GCurrentLevelEditingViewportClient)
    {
        const FVector Start = GCurrentLevelEditingViewportClient->GetViewLocation();
        const FVector End = Start + GCurrentLevelEditingViewportClient->GetViewRotation().Vector() * 200000.f;
        FHitResult Hit;
        if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility)) GroundPosition = Hit.Location;
        else GroundPosition = FVector(Start.X, Start.Y, 0.f);
    }
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    Spawn.bTemporaryEditorActor = true;
    Spawn.bHideFromSceneOutliner = true;
    Spawn.bCreateActorPackage = false;
    Spawn.Name = MakeUniqueObjectName(World, AActor::StaticClass(), TEXT("VisionPreviewObserver"));
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Observer = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(GroundPosition), Spawn);
    if (Observer)
    {
        USceneComponent* Root = NewObject<USceneComponent>(Observer, NAME_None, RF_Transient);
        Observer->SetRootComponent(Root);
        Root->RegisterComponent();
        Observer->SetActorLocation(GroundPosition);
        Observer->SetActorLabel(TEXT("视野临时观察者"), false);
        TransientObserver = Observer;
    }
    return Observer;
}

FReply SYogVisionWorkbench::PreviewCurrentLevel()
{
    UWorld* World = GetEditorWorld();
    UYogVisionSubsystem* Subsystem = World ? World->GetSubsystem<UYogVisionSubsystem>() : nullptr;
    if (!Subsystem)
    {
        Status = LOCTEXT("NoEditorWorld", "当前没有可预览的编辑关卡。");
        return FReply::Handled();
    }
    Subsystem->SetPreviewObserver(ResolvePreviewObserver(World));
    Subsystem->ApplyParameters(Draft->Parameters);
    Subsystem->SetEnabled(true);
    PreviewSubsystems.AddUnique(Subsystem);
    Status = LOCTEXT("PreviewStarted", "当前关卡预览已开启。调节右侧草稿实时更新，撤销预览恢复原状态。");
    return FReply::Handled();
}

FReply SYogVisionWorkbench::ApplyToPIE()
{
    UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
    UYogVisionSubsystem* Subsystem = World ? World->GetSubsystem<UYogVisionSubsystem>() : nullptr;
    if (!Subsystem)
    {
        Status = LOCTEXT("NoPIE", "请先运行 PIE，再应用当前草稿。");
        return FReply::Handled();
    }
    Subsystem->SetPreviewObserver(nullptr);
    Subsystem->ApplyParameters(Draft->Parameters);
    Subsystem->SetEnabled(true);
    PreviewSubsystems.AddUnique(Subsystem);
    Status = LOCTEXT("PIEApplied", "草稿已应用到当前 PIE，使用游戏中的玩家观察者。");
    return FReply::Handled();
}

void SYogVisionWorkbench::CleanupPreviews()
{
    for (const TWeakObjectPtr<UYogVisionSubsystem>& Subsystem : PreviewSubsystems)
    {
        if (Subsystem.IsValid()) Subsystem->ClearPreview();
    }
    PreviewSubsystems.Empty();
    if (TransientObserver.IsValid()) TransientObserver->Destroy();
    TransientObserver.Reset();
}

FReply SYogVisionWorkbench::RevertPreview()
{
    CleanupPreviews();
    Status = LOCTEXT("PreviewReverted", "预览已撤销，开始预览前的参数和启用状态已恢复。");
    return FReply::Handled();
}

void SYogVisionWorkbench::OnDraftChanged(const FPropertyChangedEvent&)
{
    Draft->Parameters.Sanitize();
    UpdateActivePreviews();
    Status = LOCTEXT("DraftUpdated", "草稿已更新。已开启的预览同步更新；点击保存项目默认后用于后续游戏。");
}

void SYogVisionWorkbench::UpdateActivePreviews()
{
    PreviewSubsystems.RemoveAll([](const TWeakObjectPtr<UYogVisionSubsystem>& Item) { return !Item.IsValid(); });
    for (const TWeakObjectPtr<UYogVisionSubsystem>& Subsystem : PreviewSubsystems) Subsystem->ApplyParameters(Draft->Parameters);
}

FReply SYogVisionWorkbench::SaveProjectDefaults()
{
    UYogVisionSettings* Settings = GetMutableDefault<UYogVisionSettings>();
    const FYogVisionParameters OldParameters = Settings->Parameters;
    const bool bOldEnabled = Settings->bEnabled;
    Settings->Parameters = Draft->Parameters;
    Settings->Parameters.Sanitize();
    Settings->bEnabled = Draft->bEnabled;
    if (Settings->TryUpdateDefaultConfigFile())
        Status = LOCTEXT("DefaultsSaved", "视野美术参数与游戏默认启用开关已保存。新运行的游戏使用这些值。");
    else
    {
        Settings->Parameters = OldParameters;
        Settings->bEnabled = bOldEnabled;
        Status = LOCTEXT("DefaultsSaveFailed", "保存失败，项目默认未更新。请检查默认配置文件是否可写。");
    }
    return FReply::Handled();
}

FReply SYogVisionWorkbench::ReloadProjectDefaults()
{
    const UYogVisionSettings* Settings = GetDefault<UYogVisionSettings>();
    const FScopedTransaction Transaction(LOCTEXT("ReloadVisionDefaults", "重新读入视野美术默认"));
    Draft->Modify();
    Draft->Parameters = Settings->Parameters;
    Draft->bEnabled = Settings->bEnabled;
    DraftDetails->ForceRefresh();
    UpdateActivePreviews();
    Status = LOCTEXT("DefaultsReloaded", "项目默认已重新读入草稿。");
    return FReply::Handled();
}

FReply SYogVisionWorkbench::BuildAssets()
{
    if (GEditor && GEditor->PlayWorld)
    {
        Status = LOCTEXT("AssetsDuringPIE", "请结束 PIE 后再准备表现资源。");
        return FReply::Handled();
    }
    if (BuildAssetsDelegate.IsBound())
    {
        BuildAssetsDelegate.Execute();
        Status = LOCTEXT("AssetsRequested", "已请求准备迷雾表现资源。");
    }
    else FYogVisionEditorService::BuildAssets(Status);
    return FReply::Handled();
}

FReply SYogVisionWorkbench::CreateStage()
{
    if (GEditor && GEditor->PlayWorld)
    {
        Status = LOCTEXT("StageDuringPIE", "请结束 PIE 后再打开验证关卡。");
        return FReply::Handled();
    }
    CleanupPreviews();
    UWorld* PreviousWorld = GetEditorWorld();
    bool bCreated = false;
    if (CreateStageDelegate.IsBound())
    {
        CreateStageDelegate.Execute();
        bCreated = GetEditorWorld() != PreviousWorld;
        Status = LOCTEXT("StageRequested", "已请求新建验证场景。");
    }
    else bCreated = FYogVisionEditorService::CreateValidationStage(Status);
    if (bCreated)
    {
        if (UWorld* World = GetEditorWorld())
        {
            if (UYogVisionSubsystem* Subsystem = World->GetSubsystem<UYogVisionSubsystem>())
                PreviewSubsystems.AddUnique(Subsystem);
        }
    }
    return FReply::Handled();
}

FReply SYogVisionWorkbench::AddSelectedComponent(UClass* ComponentClass)
{
    AActor* Actor = GetSelectedActor();
    if (!Actor || Actor->GetWorld() != GetEditorWorld())
    {
        Status = LOCTEXT("SelectActorFirst", "请先选择当前关卡中的灯光、角色或敌人。");
        return FReply::Handled();
    }
    if (Actor->FindComponentByClass(ComponentClass))
    {
        Status = LOCTEXT("AlreadyTagged", "选中对象已拥有该标注，可在下方调整。");
        UpdateSelectionDetails();
        return FReply::Handled();
    }
    const FScopedTransaction Transaction(LOCTEXT("AddVisionComponent", "添加视野与灵视标注"));
    Actor->Modify();
    UActorComponent* Component = NewObject<UActorComponent>(Actor, ComponentClass, NAME_None, RF_Transactional);
    if (!Component) return FReply::Handled();
    Actor->AddInstanceComponent(Component);
    if (USceneComponent* Scene = Cast<USceneComponent>(Component))
    {
        if (Actor->GetRootComponent()) Scene->SetupAttachment(Actor->GetRootComponent());
        else Actor->SetRootComponent(Scene);
    }
    if (UYogVisionSourceComponent* Source = Cast<UYogVisionSourceComponent>(Component))
        Source->bBindOwnerLight = Actor->FindComponentByClass<ULocalLightComponent>() != nullptr;
    Component->OnComponentCreated();
    Component->RegisterComponent();
    Actor->PostEditChange();
    Actor->MarkPackageDirty();
    UpdateSelectionDetails();
    Status = FText::Format(LOCTEXT("ComponentAdded", "已为「{0}」添加标注，可撤销。请保存关卡以保留。"), FText::FromString(Actor->GetActorLabel()));
    return FReply::Handled();
}

void SYogVisionWorkbench::UpdateSelectionDetails()
{
    AActor* Actor = GetSelectedActor();
    TArray<UObject*> Components;
    InspectedComponents.Empty();
    if (Actor)
    {
        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (Component && (Component->IsA<UYogVisionSourceComponent>() || Component->IsA<UYogVisionTargetComponent>() || Component->IsA<UYogVisionObserverComponent>()))
            {
                Components.Add(Component);
                InspectedComponents.Add(Component);
            }
        }
    }
    SelectionDetails->SetObjects(Components);
    InspectedActor = Actor;
}

FReply SYogVisionWorkbench::RefreshReadback()
{
    AActor* SelectedActor = GetSelectedActor();
    TArray<TWeakObjectPtr<UActorComponent>> CurrentComponents;
    if (SelectedActor)
    {
        for (UActorComponent* Component : SelectedActor->GetComponents())
        {
            if (Component && (Component->IsA<UYogVisionSourceComponent>() || Component->IsA<UYogVisionTargetComponent>() || Component->IsA<UYogVisionObserverComponent>()))
                CurrentComponents.Add(Component);
        }
    }
    if (SelectionDetails.IsValid() && (InspectedActor.Get() != SelectedActor || InspectedComponents != CurrentComponents)) UpdateSelectionDetails();
    UWorld* World = GEditor && GEditor->PlayWorld ? GEditor->PlayWorld.Get() : GetEditorWorld();
    int32 Sources = 0, Targets = 0, Direct = 0, Sensed = 0;
    TArray<FString> TargetNames;
    if (World)
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (const UYogVisionSourceComponent* Source = It->FindComponentByClass<UYogVisionSourceComponent>(); Source && Source->bVisionEnabled) ++Sources;
            if (const UYogVisionTargetComponent* Target = It->FindComponentByClass<UYogVisionTargetComponent>(); Target && Target->bTargetEnabled)
            {
                ++Targets;
                Direct += Target->IsDirectlyVisible() ? 1 : 0;
                Sensed += Target->IsSpiritSensed() ? 1 : 0;
                if (TargetNames.Num() < 5)
                    TargetNames.Add(FString::Printf(TEXT("%s：%s"), *It->GetActorLabel(), Target->IsDirectlyVisible() ? TEXT("直接可见") : Target->IsSpiritSensed() ? TEXT("灵视感知") : TEXT("迷雾遮蔽")));
            }
        }
    }
    const FString TargetList = TargetNames.IsEmpty() ? TEXT("") : TEXT("\n") + FString::Join(TargetNames, TEXT("\n"));
    SceneReadback = FText::FromString(FString::Printf(TEXT("有效火光 %d · 感知目标 %d\n直接可见 %d · 灵视感知 %d%s"), Sources, Targets, Direct, Sensed, *TargetList));
    return FReply::Handled();
}

FText SYogVisionWorkbench::GetRuntimeStatus() const
{
    UWorld* World = GEditor && GEditor->PlayWorld ? GEditor->PlayWorld.Get() : GetEditorWorld();
    const UYogVisionSubsystem* Subsystem = World ? World->GetSubsystem<UYogVisionSubsystem>() : nullptr;
    return Subsystem ? FText::FromString(Subsystem->GetStatusText()) : LOCTEXT("NoRuntime", "等待可预览的关卡。");
}

FText SYogVisionWorkbench::GetSceneReadback() const { return SceneReadback; }

FText SYogVisionWorkbench::GetSelectionLabel() const
{
    if (AActor* Actor = GetSelectedActor())
        return FText::Format(LOCTEXT("ObserverSelected", "选中观察者：{0}"), FText::FromString(Actor->GetActorLabel()));
    return LOCTEXT("ObserverFallback", "观察者：玩家起点 / 视口地面");
}

TSharedRef<SWidget> SYogVisionWorkbench::MakeButton(const FText& Label, const FText& Tooltip, FOnClicked Clicked)
{
    return SNew(SButton).Text(Label).ToolTipText(Tooltip).HAlign(HAlign_Center).OnClicked(Clicked);
}

void SYogVisionWorkbench::PostUndo(bool bSuccess)
{
    if (!bSuccess || !Draft.IsValid()) return;
    Draft->Parameters.Sanitize();
    DraftDetails->ForceRefresh();
    UpdateSelectionDetails();
    UpdateActivePreviews();
    RefreshReadback();
}

#undef LOCTEXT_NAMESPACE
