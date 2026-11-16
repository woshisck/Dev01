#include "SYogAirFogWorkbench.h"
#include "YogAirFogWorkbenchDraft.h"
#include "YogAirFogEditorService.h"

#include "DetailsViewArgs.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Misc/TransactionObjectEvent.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "YogAirFogWorkbench"

namespace
{
TSharedRef<SWidget> AirFogSection(const FText& Title, TSharedRef<SWidget> Content)
{
    return SNew(SExpandableArea)
        .InitiallyCollapsed(false)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .HeaderPadding(FMargin(10.f, 7.f)).Padding(FMargin(10.f, 7.f))
        .HeaderContent()[SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle("BoldFont"))]
        .BodyContent()[Content];
}
}

void SYogAirFogWorkbench::Construct(const FArguments&)
{
    Draft.Reset(NewObject<UYogAirFogWorkbenchDraft>(GetTransientPackage(), NAME_None, RF_Transient | RF_Transactional));
    Draft->PaletteMaterial = FYogAirFogEditorService::GetPaletteMaterial();
    RefreshPresetList();
    if (GEditor) GEditor->RegisterForUndo(this);
    FDetailsViewArgs Args;
    Args.ViewIdentifier = TEXT("YogAirFog.Draft");
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    Args.bUpdatesFromSelection = false;
    Args.bLockable = false;
    Args.bAllowSearch = true;
    Args.bHideSelectionTip = true;
    Args.bShowObjectLabel = false;
    Args.bShowOptions = false;
    Args.bShowSectionSelector = false;
    Args.bShowPropertyMatrixButton = false;
    Args.bShowScrollBar = false;
    Args.bShowLooseProperties = false;
    Args.ColumnWidth = .55f;
    Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);
    Details->SetObject(Draft.Get());
    Details->OnFinishedChangingProperties().AddSP(this, &SYogAirFogWorkbench::OnDraftChanged);
    Status = LOCTEXT("Initial", "已打开空气雾草稿。以当前镜头预览，随后可实时调整。");

    ChildSlot
    [
        SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Background")).Padding(14.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
            [SNew(STextBlock).Text(LOCTEXT("Title", "空气雾与远景层次")).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
            [SNew(STextBlock).Text(LOCTEXT("Intro", "世界空间体积雾：笔刷直接改变三维雾密度，由原生体积灯光与阴影积分。镜头移动不会平移笔刷；近处仍保留薄空气。后处理只整理配色和结构。贴图与参数随命名草稿保存。"))
                .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SNew(SSplitter)
                + SSplitter::Slot().Value(.34f).MinSize(280.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 8.f, 10.f)
                        [AirFogSection(LOCTEXT("PreviewSection", "对照与预览"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 9.f)
                            [SNew(STextBlock).Text(this, &SYogAirFogWorkbench::GetCameraLabel).AutoWrapText(true)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Preview", "预览空气雾"), LOCTEXT("PreviewTip", "以当前编辑器镜头开启空气雾。所有距离沿镜头深度计算，选择或移动角色不改变雾层位置。之后编辑草稿实时更新。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::PreviewAirFog))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Baseline", "美术基线"), LOCTEXT("BaselineTip", "临时关闭新空气雾、远景结构和原材质雾混合，保留场景美术调色、纹理与描边。草稿数值保留；编辑参数将返回空气雾预览。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::PreviewBaseline))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("WithFinish", "空气雾 + 远景结构"), LOCTEXT("WithFinishTip", "打开远景结构修饰开关，并预览当前空气雾草稿。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::PreviewWithFinish))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Revert", "撤销预览"), LOCTEXT("RevertTip", "恢复原场景雾、后处理与预览前状态。关闭本页面也会恢复。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::RevertPreview))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
                            [SNew(STextBlock).Text(this, &SYogAirFogWorkbench::GetRuntimeStatus).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())])]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 8.f, 10.f)
                        [AirFogSection(LOCTEXT("PresetsSection", "表现起点"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Balanced", "克制空气"), LOCTEXT("BalancedTip", "空气雾0.10、近景薄空气0.006、镜头近区残留0.003每米。近雾从镜头深度1800厘米起逐步增长；沿用原MI配色。"), FOnClicked::CreateLambda([this] { ApplyPreset(0); return FReply::Handled(); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Dense", "浓雾远景"), LOCTEXT("DenseTip", "空气雾0.16，近景0.006、镜头近区残留0.003每米。近雾从镜头深度1600厘米起增长，增加远处厚度与退散宽度；不覆盖原MI配色。"), FOnClicked::CreateLambda([this] { ApplyPreset(1); return FReply::Handled(); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Readable", "清晰读形"), LOCTEXT("ReadableTip", "空气雾0.045，近景0.006、镜头近区残留0.003每米。近雾从镜头深度2000厘米起增长，便于核对材质与形体；不覆盖原MI配色。"), FOnClicked::CreateLambda([this] { ApplyPreset(2); return FReply::Handled(); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("OilCloud", "钢笔流雾"), LOCTEXT("OilCloudTip", "原钢笔笔触受RG方向流图扰动；强体积集中在视野和灯光交界，远雾保持平整。"), FOnClicked::CreateLambda([this] { ApplyPreset(3); return FReply::Handled(); }))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("StaticMist", "静态厚雾 · 粗笔刷"), LOCTEXT("StaticMistTip", "确认图方向：全高度静态雾团，近处残留薄雾，粗粝擦痕和暗部排线。保持原MI近远配色。此按钮载入起点，命名草稿可另存自己的调整。"), FOnClicked::CreateLambda([this] { ApplyPreset(4); return FReply::Handled(); }))])]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 8.f, 10.f)
                        [AirFogSection(LOCTEXT("DraftSection", "资源与草稿"),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("OpenPalette", "打开原美术配色材质"), LOCTEXT("OpenPaletteTip", "打开原美术MI。启用“使用美术材质配色”后，原MI的近远两色与lerp_fog会实时同步到空气雾和远景层；美术草稿不覆盖这组参数。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::OpenPaletteMaterial))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("Build", "准备空气雾资源"), LOCTEXT("BuildTip", "准备本功能的空气体积与远景修饰材质。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::BuildAssets))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [SNew(STextBlock).Text(LOCTEXT("ChoosePreset", "待读入草稿")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [SAssignNew(PresetCombo, SComboBox<TSharedPtr<FYogAirFogNamedPresetInfo>>)
                                .OptionsSource(&PresetOptions).InitiallySelectedItem(SelectedPreset)
                                .OnGenerateWidget_Lambda([](TSharedPtr<FYogAirFogNamedPresetInfo> Item) -> TSharedRef<SWidget>
                                { return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? Item->DisplayName : FString())); })
                                .OnSelectionChanged_Lambda([this](TSharedPtr<FYogAirFogNamedPresetInfo> Item, ESelectInfo::Type) { SelectedPreset = Item; })
                                [SNew(STextBlock).Text(this, &SYogAirFogWorkbench::GetSelectedPresetLabel)]]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
                            [SNew(STextBlock).Text(LOCTEXT("SaveAsLabel", "另存名称"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(SaveAsName); })
                                .HintText(LOCTEXT("SaveAsHint", "输入新草稿名称"))
                                .ToolTipText(LOCTEXT("SaveAsTip", "可用中文，不能输入路径。另存不覆盖同名文件；保存当前只更新已经读入或保存绑定的文件。"))
                                .OnTextChanged_Lambda([this](const FText& Text) { SaveAsName = Text.ToString(); })]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
                            [MakeButton(LOCTEXT("SaveCurrent", "保存当前"), LOCTEXT("SaveCurrentTip", "保存完整参数、远景开关和配色材质身份到下方显示的当前文件。新草稿或旧版草稿会使用另存名称创建新文件，不覆盖旧版。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::SaveDraft))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("SaveAs", "另存为新草稿"), LOCTEXT("SaveAsButtonTip", "按输入的新名称创建草稿。同名时拒绝覆盖，原文件不变。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::SaveDraftAs))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("LoadSelected", "读入所选草稿"), LOCTEXT("LoadSelectedTip", "读入下拉框所选文件的完整参数、远景开关与配色材质身份。可撤销恢复读入前的草稿及文件身份。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::LoadDraft))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("RefreshPresets", "刷新草稿列表"), LOCTEXT("RefreshPresetsTip", "重新读取 Presets 文件列表，不修改当前草稿或场景。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::RefreshDrafts))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                            [MakeButton(LOCTEXT("ImportLegacy", "导入旧版为命名草稿"), LOCTEXT("ImportLegacyTip", "将旧 ArtDraft.json 复制到“油画卷雾_继承参数”。同名时递增后缀，不覆盖已有草稿；旧版文件原样保留。"), FOnClicked::CreateSP(this, &SYogAirFogWorkbench::ImportLegacyDraft))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
                            [SNew(STextBlock).Text(this, &SYogAirFogWorkbench::GetDocumentStatus)
                                .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
                            [SNew(STextBlock).Text(this, &SYogAirFogWorkbench::GetDirectionalGuide)
                                .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())])]
                    ]
                ]
                + SSplitter::Slot().Value(.66f).MinSize(380.f)
                [
                    SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(8.f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(4.f, 4.f, 4.f, 10.f)
                        [SNew(STextBlock).Text(LOCTEXT("ArtDraft", "空气雾美术草稿")).Font(FAppStyle::GetFontStyle("BoldFont"))]
                        + SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 4.f, 10.f)
                        [SNew(STextBlock).Text(LOCTEXT("DensityGuide", "三种密度均为每米绝对值：远处空气雾 ≥ 近景薄空气 ≥ 镜头近区残留。所有距离以镜头深度为基准；“美术基线”保留草稿。"))
                            .ToolTipText(LOCTEXT("DensityGuideTip", "近雾起点是从镜头出发的绝对深度；清雾范围与退散宽度也沿镜头深度计算。密度为0时三种密度全部归零，不依赖角色选区。"))
                            .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                        + SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 4.f, 10.f)
                        [SNew(STextBlock).Text(LOCTEXT("PaletteGuide", "原MI近远色保留。笔触贴图与方向流图可分别选择；流图按线性RG向量解码。基础密度和修饰共用屏幕范围，实际原生雾覆盖与受光决定最终表现。灯光清雾范围是表现近似，不代替游戏视线遮挡检测。"))
                            .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                        + SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 4.f, 10.f)
                        [SNew(STextBlock).Text(LOCTEXT("TemporalGuide", "体积质量与时序稳定性：先调整网格精度，再观察历史权重的稳定性和移动拖影；局部灯边缘柔化用于聚光灯与矩形灯。"))
                            .AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                        + SVerticalBox::Slot().FillHeight(1.f)
                        [SNew(SScrollBox) + SScrollBox::Slot()[Details.ToSharedRef()]]
                    ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(4.f, 12.f, 4.f, 2.f)
            [SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true)]
        ]
    ];
}

SYogAirFogWorkbench::~SYogAirFogWorkbench()
{
    if (GEditor) GEditor->UnregisterForUndo(this);
    FYogAirFogEditorService::RevertPreview();
    FYogAirFogEditorService::SetPreviewPaletteMaterial(nullptr);
}

AActor* SYogAirFogWorkbench::ResolveFocusActor()
{
    // Retained for existing console entry points. The runtime resolves the
    // current view; editor selection and actor positions are not fog anchors.
    return nullptr;
}

FReply SYogAirFogWorkbench::PreviewAirFog()
{
    if (!PreparePalette()) return FReply::Handled();
    bBaselineMode = false;
    Draft->Parameters.Sanitize();
    FYogAirFogEditorService::Preview(Draft->Parameters, nullptr, Draft->bFinishEnabled, Status);
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::PreviewBaseline()
{
    if (!PreparePalette()) return FReply::Handled();
    FYogAirFogParameters Baseline = Draft->Parameters;
    Baseline.Density = 0.f;
    Baseline.Sanitize();
    bBaselineMode = FYogAirFogEditorService::Preview(Baseline, nullptr, false, Status);
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::PreviewWithFinish()
{
    const FScopedTransaction Transaction(LOCTEXT("EnableFinishTransaction", "启用空气雾远景结构"));
    Draft->Modify();
    Draft->bFinishEnabled = true;
    Details->ForceRefresh();
    return PreviewAirFog();
}

FReply SYogAirFogWorkbench::BuildAssets()
{
    FYogAirFogEditorService::BuildAssets(Status);
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::OpenPaletteMaterial()
{
    UMaterialInterface* Palette = Draft->PaletteMaterial.LoadSynchronous();
    UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
    if (IsValid(Palette) && AssetEditor)
    {
        AssetEditor->OpenEditorForAsset(Palette);
        Status = LOCTEXT("PaletteOpened", "已打开当前草稿指定的配色材质。开启美术材质配色后，近远两色与lerp_fog实时同步预览。");
    }
    else Status = LOCTEXT("PaletteUnavailable", "原美术配色材质暂不可用，请先核对场景原MI。");
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::RevertPreview()
{
    FYogAirFogEditorService::RevertPreview();
    bBaselineMode = false;
    Status = LOCTEXT("Reverted", "空气雾预览已撤销，原场景状态已恢复。");
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::SaveDraft()
{
    if (Draft->PresetId.IsEmpty() || Draft->PresetId == FYogAirFogEditorService::GetLegacyPresetId()) return SaveDraftAs();
    Draft->Parameters.Sanitize();
    const FYogAirFogPresetData Data = GetDraftSnapshot();
    if (FYogAirFogEditorService::SaveNamedPreset(Draft->PresetId, Data, true, Status))
    {
        RememberSavedSnapshot(Draft->PresetId, Data);
        RefreshPresetList(Draft->PresetId);
    }
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::SaveDraftAs()
{
    Draft->Parameters.Sanitize();
    const FYogAirFogPresetData Data = GetDraftSnapshot();
    if (FYogAirFogEditorService::SaveNamedPreset(SaveAsName, Data, false, Status))
    {
        const FScopedTransaction Transaction(LOCTEXT("SaveAsTransaction", "命名空气雾美术草稿"));
        Draft->Modify();
        Draft->PresetId = SaveAsName;
        RememberSavedSnapshot(Draft->PresetId, Data);
        RefreshPresetList(Draft->PresetId);
    }
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::LoadDraft()
{
    if (!SelectedPreset.IsValid())
    {
        Status = LOCTEXT("NoPresetSelected", "请先从下拉框选择要读入的草稿。");
        return FReply::Handled();
    }
    const FString Id = SelectedPreset->Id;
    FYogAirFogPresetData Loaded;
    if (FYogAirFogEditorService::LoadNamedPreset(Id, Loaded, Status))
    {
        const FScopedTransaction Transaction(LOCTEXT("LoadDraftTransaction", "读入空气雾美术草稿"));
        Draft->Modify();
        Draft->Parameters = Loaded.Parameters;
        Draft->bFinishEnabled = Loaded.bFinishEnabled;
        Draft->PaletteMaterial = TSoftObjectPtr<UMaterialInterface>(Loaded.PaletteMaterial);
        Draft->PresetId = Id;
        Draft->Parameters.Sanitize();
        RememberSavedSnapshot(Id, GetDraftSnapshot());
        SaveAsName = Id == FYogAirFogEditorService::GetLegacyPresetId() ? TEXT("油画卷雾_继承参数") : Id;
        bBaselineMode = false;
        Details->ForceRefresh();
        UpdateActivePreview();
    }
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::RefreshDrafts()
{
    RefreshPresetList(SelectedPreset.IsValid() ? SelectedPreset->Id : Draft->PresetId);
    Status = LOCTEXT("PresetsRefreshed", "草稿列表已刷新；当前草稿内容未改变。");
    return FReply::Handled();
}

FReply SYogAirFogWorkbench::ImportLegacyDraft()
{
    FString Imported;
    if (FYogAirFogEditorService::ImportLegacyPreset(Imported, Status))
    {
        RefreshPresetList(Imported);
        return LoadDraft();
    }
    return FReply::Handled();
}

void SYogAirFogWorkbench::SetSaveAsName(const FString& Name) { SaveAsName = Name; }

bool SYogAirFogWorkbench::SelectSavedPreset(const FString& Id)
{
    for (const auto& Option : PresetOptions)
        if (Option->Id == Id)
        {
            SelectedPreset = Option;
            if (PresetCombo.IsValid()) PresetCombo->SetSelectedItem(Option);
            return true;
        }
    return false;
}

void SYogAirFogWorkbench::RefreshPresetList(const FString& PreferredId)
{
    TArray<FYogAirFogNamedPresetInfo> Found;
    FText Message;
    FYogAirFogEditorService::ListNamedPresets(Found, Message);
    PresetOptions.Reset();
    SelectedPreset.Reset();
    for (const auto& Info : Found) PresetOptions.Add(MakeShared<FYogAirFogNamedPresetInfo>(Info));
    if (PresetCombo.IsValid()) PresetCombo->RefreshOptions();
    if (!SelectSavedPreset(PreferredId) && PresetOptions.Num())
    {
        SelectedPreset = PresetOptions[0];
        if (PresetCombo.IsValid()) PresetCombo->SetSelectedItem(SelectedPreset);
    }
}

FYogAirFogPresetData SYogAirFogWorkbench::GetDraftSnapshot() const
{
    FYogAirFogPresetData Data;
    Data.Parameters = Draft->Parameters;
    Data.bFinishEnabled = Draft->bFinishEnabled;
    Data.PaletteMaterial = Draft->PaletteMaterial.ToSoftObjectPath();
    return Data;
}

void SYogAirFogWorkbench::RememberSavedSnapshot(const FString& Id, const FYogAirFogPresetData& Data)
{
    FYogAirFogPresetData Saved = Data;
    Saved.Parameters.Sanitize();
    SavedSnapshots.Add(Id, MoveTemp(Saved));
}

bool SYogAirFogWorkbench::IsDraftDirty() const
{
    const FYogAirFogPresetData* Saved = SavedSnapshots.Find(Draft->PresetId);
    const FYogAirFogPresetData Current = GetDraftSnapshot();
    return !Saved || Saved->bFinishEnabled != Current.bFinishEnabled || Saved->PaletteMaterial != Current.PaletteMaterial
        || !FYogAirFogParameters::StaticStruct()->CompareScriptStruct(&Saved->Parameters, &Current.Parameters, 0);
}

FString SYogAirFogWorkbench::GetCurrentPresetId() const { return Draft->PresetId; }

FString SYogAirFogWorkbench::GetCurrentPresetFilePath() const
{
    if (Draft->PresetId.IsEmpty()) return FString();
    return Draft->PresetId == FYogAirFogEditorService::GetLegacyPresetId()
        ? FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AirFog/ArtDraft.json"))
        : FYogAirFogEditorService::GetPresetDirectory() / (Draft->PresetId + TEXT(".json"));
}

FText SYogAirFogWorkbench::GetDocumentStatus() const
{
    const FString File = GetCurrentPresetFilePath();
    return FText::FromString(File.IsEmpty() ? TEXT("当前：尚未命名的新草稿\n未保存修改。首次保存使用上方另存名称。")
        : FString::Printf(TEXT("当前文件：%s\n%s"), *File, IsDraftDirty() ? TEXT("有未保存修改 *") : TEXT("与已读入或保存的文件一致")));
}

FText SYogAirFogWorkbench::GetSelectedPresetLabel() const
{
    return SelectedPreset.IsValid() ? FText::FromString(SelectedPreset->DisplayName) : LOCTEXT("NoSavedPresets", "尚无草稿，请输入名称另存");
}

bool SYogAirFogWorkbench::PreparePalette()
{
    UMaterialInterface* Material = Draft->PaletteMaterial.LoadSynchronous();
    if (!IsValid(Material) || Material->IsA<UMaterialInstanceDynamic>() || Material->HasAnyFlags(RF_Transient)
        || Material->GetOutermost() == GetTransientPackage())
    {
        Status = LOCTEXT("InvalidDraftPalette", "草稿配色材质无效或是临时MID。请选择持久材质资产；现有预览保持不变。");
        return false;
    }
    FYogAirFogEditorService::SetPreviewPaletteMaterial(Material);
    return true;
}

void SYogAirFogWorkbench::ApplyPreset(int32 PresetIndex)
{
    const FScopedTransaction Transaction(LOCTEXT("PresetTransaction", "调整空气雾表现起点"));
    Draft->Modify();
    FYogAirFogParameters& P = Draft->Parameters;
    P = FYogAirFogParameters();
    P.Density = .10f; P.NearDensity = .006f; P.ClearMinDensity = .003f;
    P.bUseArtistPalette = true;
    P.AmbientIntensity = .25f;
    if (PresetIndex == 1)
    {
        P.Density = .16f;
        P.NearOffset = 1600.f; P.DepthRamp = 1600.f; P.DepthPower = 1.35f;
        P.ClearRadius = 350.f; P.ClearFeather = 800.f;
        P.NoiseAmount = .42f; P.NoiseScale = .004f; P.WindSpeed = 10.f; P.BoundaryNoise = 140.f;
        P.FarStart = 2200.f; P.FarEnd = 5000.f; P.FarFlatten = .92f;
        P.StructureStrength = .48f; P.DetailStrength = .12f; P.EdgeWidth = 1.3f;
        Status = LOCTEXT("DenseDraft", "浓雾远景已读入草稿。");
    }
    else if (PresetIndex == 2)
    {
        P.Density = .045f;
        P.NearOffset = 2000.f; P.DepthRamp = 2200.f; P.DepthPower = 1.f;
        P.ClearRadius = 500.f; P.ClearFeather = 750.f;
        P.NoiseAmount = .18f; P.NoiseScale = .004f; P.WindSpeed = 6.f; P.BoundaryNoise = 50.f;
        P.FarStart = 2600.f; P.FarEnd = 5600.f; P.FarFlatten = .55f;
        P.StructureStrength = .32f; P.DetailStrength = .25f; P.EdgeWidth = 1.f;
        Status = LOCTEXT("ClearDraft", "清晰读形已读入草稿。");
    }
    else if (PresetIndex == 3)
    {
        P.Density = .10f;
        P.NearOffset = 1800.f; P.DepthRamp = 1800.f;
        P.StrokeStrength = .7f; P.StrokeScale = 1.8f;
        P.StrokeFlowSpeed = .3f; P.StrokeWarp = .2f;
        P.BoundaryDetail = .5f; P.VolumeFormStrength = .8f;
        P.OuterVolumeStrength = .025f;
        Status = LOCTEXT("OilCloudDraft", "钢笔流雾：原钢笔纹理 + RG方向流图，体积感集中在受光交界；原MI近远色保留。");
    }
    else if (PresetIndex == 4)
    {
        P.Density = .11f; P.NearDensity = .014f; P.ClearMinDensity = .005f;
        P.NearOffset = 1450.f; P.DepthRamp = 2000.f; P.DepthPower = 1.1f;
        P.ClearRadius = 180.f; P.ClearFeather = 850.f;
        P.CameraHeightOffset = 0.f; P.HeightFalloff = 12000.f;
        P.NoiseAmount = .55f; P.NoiseScale = .006f;
        P.AmbientIntensity = .95f; P.FarFlatten = .7f;
        P.PaletteDistance = 2300.f;
        P.WorldVisionRadius = 450.f; P.WorldVisionFeather = 650.f;
        P.StructureStrength = .30f; P.DetailStrength = .10f;
        P.StrokeStrength = .8f; P.StrokeScale = 1.f;
        P.StrokeTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/YogVision/Textures/T_StaticCharcoal.T_StaticCharcoal")));
        P.GridPixelSize = 4; P.GridSizeZ = 128;
        P.StrokeFlowSpeed = 0.f; P.StrokeWarp = 0.f;
        P.BoundaryDetail = .3f; P.VolumeFormStrength = .6f;
        P.OuterVolumeStrength = .025f;
        P.LightClearStrength = .45f;
        Status = LOCTEXT("StaticMistDraft", "世界空间厚雾起点已读入：笔刷进入真实体积密度，近处保留薄空气。质量设为4像素/128层，可按性能调整。");
    }
    else Status = LOCTEXT("BalancedDraft", "克制空气已读入草稿。");
    P.Sanitize();
    bBaselineMode = false;
    Details->ForceRefresh();
    UpdateActivePreview();
}

void SYogAirFogWorkbench::OnDraftChanged(const FPropertyChangedEvent&)
{
    Draft->Parameters.Sanitize();
    bBaselineMode = false;
    if (FYogAirFogEditorService::IsPreviewActive() && !PreparePalette()) return;
    UpdateActivePreview();
    Status = FYogAirFogEditorService::IsPreviewActive()
        ? LOCTEXT("LiveDraft", "草稿已更新，空气雾预览实时同步。")
        : LOCTEXT("PendingDraft", "草稿已更新。点击预览空气雾后观察效果。");
}

void SYogAirFogWorkbench::UpdateActivePreview()
{
    if (!FYogAirFogEditorService::IsPreviewActive()) return;
    if (!PreparePalette()) return;
    FYogAirFogParameters Effective = Draft->Parameters;
    if (bBaselineMode) Effective.Density = 0.f;
    Effective.Sanitize();
    FYogAirFogEditorService::UpdatePreview(Effective, bBaselineMode ? false : Draft->bFinishEnabled);
}

bool SYogAirFogWorkbench::MatchesContext(const FTransactionContext&, const TArray<TPair<UObject*, FTransactionObjectEvent>>& Objects) const
{
    // A restored tab and an independently opened draft must not both push
    // their values into the one live preview after an unrelated undo.
    return Draft.IsValid() && Objects.ContainsByPredicate([this](const auto& Item) { return Item.Key == Draft.Get(); });
}

void SYogAirFogWorkbench::PostUndo(bool bSuccess)
{
    if (!bSuccess || !Draft.IsValid()) return;
    Draft->Parameters.Sanitize();
    bBaselineMode = false;
    RefreshPresetList(Draft->PresetId);
    Details->ForceRefresh();
    UpdateActivePreview();
}

const FYogAirFogParameters& SYogAirFogWorkbench::GetDraftParameters() const { return Draft->Parameters; }

FText SYogAirFogWorkbench::GetCameraLabel() const
{
    return LOCTEXT("CameraReference", "编辑器：当前视口中心。游戏：玩家投影中心，无玩家时回退屏幕中心。深度始终来自镜头。");
}

FText SYogAirFogWorkbench::GetRuntimeStatus() const
{
    if (bBaselineMode && FYogAirFogEditorService::IsPreviewActive())
        return LOCTEXT("BaselineActive", "美术基线预览中。编辑草稿将返回空气雾。");
    return FText::FromString(FYogAirFogEditorService::GetPreviewStatus());
}

FText SYogAirFogWorkbench::GetDirectionalGuide() const
{
    return Draft->Parameters.bUseSceneDirectionalLight
        ? LOCTEXT("AutomaticFormDirection", "方向塑形自动跟随场景方向光；来源见预览状态。找不到有效方向光时使用手动方向与光色。体积沿用场景受光，不新增灯光Actor；撤销预览恢复临时状态。")
        : LOCTEXT("ManualFormDirection", "方向塑形使用手动方向与光色。它定义空气与形体的美术明暗，不新增灯光Actor，也不新增场景投影；撤销预览恢复临时状态。");
}

TSharedRef<SWidget> SYogAirFogWorkbench::MakeButton(const FText& Label, const FText& Tooltip, FOnClicked Clicked)
{
    return SNew(SButton).Text(Label).ToolTipText(Tooltip).HAlign(HAlign_Center).OnClicked(Clicked);
}

#undef LOCTEXT_NAMESPACE
