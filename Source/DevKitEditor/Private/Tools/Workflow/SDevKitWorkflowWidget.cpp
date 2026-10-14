#include "Tools/Workflow/SDevKitWorkflowWidget.h"
#include "Tools/Workflow/SDevKitGraphicsQualityWorkbenchWidget.h"
#include "Tools/Workflow/SDevKitRenderStyleWorkbenchWidget.h"
#include "Tools/Workflow/DevKitWorkbenchUI.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"

#include "Dom/JsonObject.h"
#include "Async/Async.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CollectionManagerModule.h"
#include "ICollectionManager.h"
#include "ICollectionContainer.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "RuneEditor/SRuneEditorWidget.h"
#include "SStylizedEmissiveLibraryWidget.h"
#include "Tools/DevKitArtToolUI.h"
#include "Tools/SActionBalanceWidget.h"
#include "Tools/SBuffFlowDebugWidget.h"
#include "Tools/SCharacterBalanceWidget.h"
#include "Tools/SDataEditorWidget.h"
#include "Tools/SEnvBatchTaggerWidget.h"
#include "Tools/SLevelDataWorkbenchWidget.h"
#include "Tools/SMaterialTextureRulesWidget.h"
#include "Tools/SMetaProgressionWorkbenchWidget.h"
#include "Tools/SModelAssetComplianceWidget.h"
#include "Tools/SRuntimeGMSettingsWidget.h"
#include "Tools/SRVTMaterialManagerWidget.h"
#include "Tools/STextureVTAuditWidget.h"
#include "Tools/SVirtualTextureCollectionManagerWidget.h"
#include "Tools/EnemyManager/SEnemyManagerWidget.h"
#include "Tools/WeaponManager/SWeaponManagerWidget.h"
#include "Tools/StoryEncounter/SStoryEncounterWorkbenchWidget.h"
#include "Tools/MapCreator/SDevKitMapCreatorWidget.h"
#include "Tools/SceneWorkbench/SDevKitSceneWorkbenchWidget.h"
#include "Tools/CharacterLookDev/SDevKitCharacterLookDevWidget.h"
#include "Tools/LevelBatch/SDevKitLevelBatchProcessorWidget.h"
#include "Tools/LevelRVT/SDevKitLevelRVTWidget.h"
#include "Tools/RVTMeshDecal/SDevKitRVTMeshDecalWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Docking/SDockTab.h"

namespace
{
    FAutoConsoleCommand OpenGlobalEnvironmentCommand(TEXT("DevKit.Rendering.GlobalEnvironment.Open"),
        TEXT("Open Rendering / Lighting / Render Style. Opening reads drafts only; Apply and config saving are explicit."),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            if (TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitMaterialsWorkbench"))))
                StaticCastSharedRef<SDevKitWorkflowWidget>(Tab->GetContent())->SelectStepById(TEXT("stylized_lighting"));
        }));
}

void SDevKitWorkflowWidget::LoadCatalog()
{
    FString Json;
    TSharedPtr<FJsonObject> Root;
    const FString Path = FPaths::ProjectDir() / TEXT("Build/Dev01WorkflowCatalog.json");
    if (!FFileHelper::LoadFileToString(Json, *Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
    {
        LoadError = TEXT("无法读取工作流目录：") + Path;
        return;
    }
    const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
    if (!Root->TryGetArrayField(TEXT("workflows"), Items))
    {
        LoadError = TEXT("工作流目录缺少 workflows。");
        return;
    }
    for (const auto& Value : *Items)
    {
        const TSharedPtr<FJsonObject> Item = Value->AsObject();
        if (!Item) continue;
        FWorkflow Workflow;
        Item->TryGetStringField(TEXT("id"), Workflow.Id);
        if (!WorkflowId.IsEmpty() && Workflow.Id != WorkflowId) continue;
        Item->TryGetStringField(TEXT("title"), Workflow.Title);
        Item->TryGetStringField(TEXT("summary"), Workflow.Summary);
        const TArray<TSharedPtr<FJsonValue>>* Steps = nullptr;
        if (!Item->TryGetArrayField(TEXT("steps"), Steps)) continue;
        for (const auto& StepValue : *Steps)
        {
            const TSharedPtr<FJsonObject> Data = StepValue->AsObject();
            if (!Data) continue;
            FStep Step;
            FString Tab;
            Data->TryGetStringField(TEXT("id"), Step.Id);
            Data->TryGetStringField(TEXT("group"), Step.Group);
            Data->TryGetStringField(TEXT("title"), Step.Title);
            Data->TryGetStringField(TEXT("input"), Step.Input);
            Data->TryGetStringField(TEXT("output"), Step.Output);
            Data->TryGetStringField(TEXT("tab"), Tab);
            Data->TryGetBoolField(TEXT("external"), Step.bExternal);
            Step.Tab = FName(*Tab);
            if (!Step.Id.IsEmpty() && !Step.Title.IsEmpty()) Workflow.Steps.Add(MoveTemp(Step));
        }
        if (Workflow.Steps.Num()) Workflows.Add(MoveTemp(Workflow));
    }
}

void SDevKitWorkflowWidget::Construct(const FArguments& InArgs)
{
    WorkflowId = InArgs._WorkflowId.IsEmpty() ? TEXT("materials") : InArgs._WorkflowId;
    OpenDecalPlacement = InArgs._OnOpenDecalPlacement;
    LoadCatalog();
    const FString ConfigSection = FString::Printf(TEXT("DevKit.Workflow.%s"), *WorkflowId);
    GConfig->GetInt(*ConfigSection, TEXT("Step"), ActiveStep, GEditorPerProjectIni);
    // Scene configuration is an asset editor, not the old creation wizard.
    if (WorkflowId == TEXT("scene")) ActiveStep = 0;
    ActiveWorkflow = FMath::Clamp(ActiveWorkflow, 0, FMath::Max(0, Workflows.Num() - 1));

    ChildSlot
    [
        SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Background"))).Padding(0)
        [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(16.f, 12.f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [SNew(STextBlock).Text_Lambda([this]()
                {
                    return Workflows.IsValidIndex(ActiveWorkflow)
                        ? FText::FromString(TEXT("DEV01  /  ") + Workflows[ActiveWorkflow].Title + TEXT("工作台"))
                        : FText::FromString(TEXT("DEV01  /  工作台"));
                })
                .Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(FPaths::FileExists(FPaths::EngineDir() / TEXT("Build/InstalledBuild.txt"))
                ? TEXT("已发布引擎 / Installed Build") : TEXT("源码引擎 / 本地开发")))
                .ColorAndOpacity(FStyleColors::Foreground)]
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SSplitter)
            + SSplitter::Slot().Value(0.22f).MinSize(230.f)
            [
                SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed"))).Padding(10.f)
                [SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
                    [SNew(SSearchBox).HintText(FText::FromString(TEXT("查找工具或制作步骤")))
                        .OnTextChanged(this, &SDevKitWorkflowWidget::SetSearchText)]
                    + SVerticalBox::Slot().FillHeight(1.f)
                    [SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Navigation, SVerticalBox)]]]
            ]
            + SSplitter::Slot().Value(0.78f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(16.f, 12.f, 16.f, 4.f)
                [SNew(STextBlock).Text(this, &SDevKitWorkflowWidget::GetStepTitle)
                    .Visibility_Lambda([this](){return WorkflowId == TEXT("scene") && ActiveStep == 0 ? EVisibility::Collapsed : EVisibility::Visible;})
                    .Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle"))).AutoWrapText(true)]
                + SVerticalBox::Slot().AutoHeight().Padding(16.f, 2.f, 16.f, 12.f)
                [SNew(SUniformGridPanel).SlotPadding(FMargin(0.f, 0.f, 12.f, 0.f))
                    .Visibility_Lambda([this](){return WorkflowId == TEXT("scene") && ActiveStep == 0 ? EVisibility::Collapsed : EVisibility::Visible;})
                    + SUniformGridPanel::Slot(0, 0)
                    [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).Padding(10.f)
                        [SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [SNew(STextBlock).Text(FText::FromString(TEXT("准备什么"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
                            [SNew(STextBlock).Text(this, &SDevKitWorkflowWidget::GetStepInput).AutoWrapText(true)]]]
                    + SUniformGridPanel::Slot(1, 0)
                    [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).Padding(10.f)
                        [SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [SNew(STextBlock).Text(FText::FromString(TEXT("完成后得到什么"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
                            [SNew(STextBlock).Text(this, &SDevKitWorkflowWidget::GetStepOutput).AutoWrapText(true)]]]]
                + SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f)
                [SAssignNew(Content, SBox)]
                + SVerticalBox::Slot().AutoHeight().Padding(16.f, 10.f)
                [
                    SNew(SHorizontalBox)
                    .Visibility_Lambda([this](){return WorkflowId == TEXT("scene") ? EVisibility::Collapsed : EVisibility::Visible;})
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                        .ContentPadding(FMargin(14.f, 7.f)).Text(FText::FromString(TEXT("上一步")))
                        .IsEnabled_Lambda([this](){return Workflows.IsValidIndex(ActiveWorkflow) && ActiveStep > 0;})
                        .OnClicked_Lambda([this](){SelectStep(ActiveStep - 1); return FReply::Handled();})]
                    + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f)
                    [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
                        .ContentPadding(FMargin(14.f, 7.f)).Text(FText::FromString(TEXT("下一步")))
                        .IsEnabled_Lambda([this](){return Workflows.IsValidIndex(ActiveWorkflow) && ActiveStep + 1 < Workflows[ActiveWorkflow].Steps.Num();})
                        .OnClicked_Lambda([this](){SelectStep(ActiveStep + 1); return FReply::Handled();})]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                    [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(ActiveWorkflow == INDEX_NONE
                        ? TEXT("只读检查不会提交 P4，也不会发布引擎或 PCB。") : TEXT("仅切换步骤，不自动修改资产。"));})
                        .ColorAndOpacity(FSlateColor::UseSubduedForeground()).AutoWrapText(true)]
                ]
            ]
        ]
        ]
    ];
    RebuildNavigation();
    if (Workflows.IsValidIndex(ActiveWorkflow)) SelectStep(FMath::Clamp(ActiveStep, 0, Workflows[ActiveWorkflow].Steps.Num() - 1));
    else Content->SetContent(SNew(STextBlock).Text(FText::FromString(LoadError)).AutoWrapText(true));
}

SDevKitWorkflowWidget::~SDevKitWorkflowWidget()
{
    const FString ConfigSection = FString::Printf(TEXT("DevKit.Workflow.%s"), *WorkflowId);
    GConfig->SetInt(*ConfigSection, TEXT("Step"), ActiveStep, GEditorPerProjectIni);
}

void SDevKitWorkflowWidget::RebuildNavigation()
{
    Navigation->ClearChildren();
    int32 VisibleSteps = 0;
    for (int32 Index = 0; Index < Workflows.Num(); ++Index)
    {
        const bool bMatchesWorkflow = Workflows[Index].Title.Contains(SearchText);
        bool bAnyMatch = SearchText.IsEmpty() || bMatchesWorkflow;
        for (const FStep& Step : Workflows[Index].Steps)
            bAnyMatch |= Step.Title.Contains(SearchText) || Step.Input.Contains(SearchText) || Step.Output.Contains(SearchText);
        if (!bAnyMatch) continue;
        const bool bHasNamedGroups = Workflows[Index].Steps.ContainsByPredicate([](const FStep& Step) { return !Step.Group.IsEmpty(); });
        if (Workflows.Num() == 1 && bHasNamedGroups)
        {
            Navigation->AddSlot().AutoHeight().Padding(8.f, 8.f, 4.f, 4.f)
            [SNew(STextBlock).Text(FText::FromString(Workflows[Index].Title))
                .ToolTipText(FText::FromString(Workflows[Index].Summary))
                .Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))];
        }
        else if (Workflows.Num() > 1)
        {
            Navigation->AddSlot().AutoHeight().Padding(4.f, 8.f, 4.f, 2.f)
            [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                .ContentPadding(FMargin(10.f, 9.f)).HAlign(HAlign_Left).Text(FText::FromString(Workflows[Index].Title))
                .ToolTipText(FText::FromString(Workflows[Index].Summary))
                .OnClicked_Lambda([this, Index](){SelectWorkflow(Index); return FReply::Handled();})];
            if (ActiveWorkflow != Index && SearchText.IsEmpty()) continue;
        }
        FString LastGroup;
        TSharedPtr<SVerticalBox> GroupBody;
        const auto AddGroup = [this, Index, &LastGroup, &GroupBody]()
        {
            if (!GroupBody) return;
            const FString GroupKey = Workflows[Index].Id + TEXT(":") + LastGroup;
            const bool bCollapsed = SearchText.IsEmpty() && CollapsedNavigationGroups.FindRef(GroupKey);
            const TWeakPtr<SDevKitWorkflowWidget> WeakThis = SharedThis(this);
            Navigation->AddSlot().AutoHeight().Padding(4.f, 8.f, 4.f, 2.f)
            [DevKitWorkbenchUI::MakeSection(FText::FromString(LastGroup), GroupBody.ToSharedRef(), bCollapsed,
                FText::GetEmpty(), FSlateColor::UseSubduedForeground(),
                FOnBooleanValueChanged::CreateLambda([WeakThis, GroupKey](bool bExpanded)
                {
                    if (const TSharedPtr<SDevKitWorkflowWidget> Self = WeakThis.Pin())
                        if (Self->SearchText.IsEmpty()) Self->CollapsedNavigationGroups.FindOrAdd(GroupKey) = !bExpanded;
                }))];
        };
        for (int32 Step = 0; Step < Workflows[Index].Steps.Num(); ++Step)
        {
            const FStep& Entry = Workflows[Index].Steps[Step];
            if (!SearchText.IsEmpty() && !bMatchesWorkflow && !Entry.Title.Contains(SearchText) && !Entry.Input.Contains(SearchText) && !Entry.Output.Contains(SearchText)) continue;
            const FString GroupName = Entry.Group.IsEmpty() ? Workflows[Index].Title : Entry.Group;
            if (!GroupBody || GroupName != LastGroup)
            {
                AddGroup();
                LastGroup = GroupName;
                GroupBody = SNew(SVerticalBox);
            }
            ++VisibleSteps;
            const bool bSelected = ActiveWorkflow == Index && ActiveStep == Step;
            GroupBody->AddSlot().AutoHeight().Padding(2.f)
            [SNew(SBorder).Padding(0.f)
                .BorderImage(FAppStyle::GetBrush(bSelected ? TEXT("Brushes.Primary") : TEXT("Brushes.Recessed")))
                [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                    .ContentPadding(FMargin(12.f, 9.f)).HAlign(HAlign_Left)
                    .ForegroundColor(bSelected ? FStyleColors::White : FStyleColors::Foreground)
                    .Text(FText::FromString(WorkflowId == TEXT("scene") ? Entry.Title : FString::Printf(TEXT("%02d  %s"), Step + 1, *Entry.Title)))
                    .ToolTipText(FText::FromString(TEXT("准备：") + Entry.Input + TEXT("\n产出：") + Entry.Output))
                    .OnClicked_Lambda([this, Index, Step](){ActiveWorkflow = Index; SelectStep(Step); return FReply::Handled();})]];
        }
        AddGroup();
    }
    if (!SearchText.IsEmpty() && VisibleSteps == 0)
        Navigation->AddSlot().AutoHeight().Padding(12.f)[SNew(STextBlock)
            .Text(FText::FromString(TEXT("没有匹配的工具。\n试试材质、贴花或关卡。"))).AutoWrapText(true)];
    Navigation->AddSlot().AutoHeight().Padding(4.f, 16.f)
    [SNew(SBorder).Padding(0.f)
        .BorderImage(FAppStyle::GetBrush(ActiveWorkflow == INDEX_NONE ? TEXT("Brushes.Primary") : TEXT("Brushes.Recessed")))
        [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
        .ForegroundColor(ActiveWorkflow == INDEX_NONE ? FStyleColors::White : FStyleColors::Foreground).HAlign(HAlign_Left)
        .ContentPadding(FMargin(10.f, 10.f)).Text(FText::FromString(TEXT("发布与数据整理")))
        .OnClicked_Lambda([this](){ShowOperations(); return FReply::Handled();})]];
}

void SDevKitWorkflowWidget::SetSearchText(const FText& Text)
{
    SearchText = Text.ToString().TrimStartAndEnd();
    RebuildNavigation();
}

FText SDevKitWorkflowWidget::GetStepTitle() const
{
    if (!Workflows.IsValidIndex(ActiveWorkflow) || !Workflows[ActiveWorkflow].Steps.IsValidIndex(ActiveStep))
        return FText::FromString(TEXT("发布与数据整理  /  检查 → 构建 → 验收 → 发布"));
    return FText::FromString(FString::Printf(TEXT("%s  /  %02d  %s   ·   共 %d 步"),
        *Workflows[ActiveWorkflow].Title, ActiveStep + 1, *Workflows[ActiveWorkflow].Steps[ActiveStep].Title,
        Workflows[ActiveWorkflow].Steps.Num()));
}

FText SDevKitWorkflowWidget::GetStepInput() const
{
    return Workflows.IsValidIndex(ActiveWorkflow) && Workflows[ActiveWorkflow].Steps.IsValidIndex(ActiveStep)
        ? FText::FromString(Workflows[ActiveWorkflow].Steps[ActiveStep].Input) : FText::FromString(TEXT("已确认的本地代码和发布计划"));
}

FText SDevKitWorkflowWidget::GetStepOutput() const
{
    return Workflows.IsValidIndex(ActiveWorkflow) && Workflows[ActiveWorkflow].Steps.IsValidIndex(ActiveStep)
        ? FText::FromString(Workflows[ActiveWorkflow].Steps[ActiveStep].Output) : FText::FromString(TEXT("版本检查、发布阻塞项与数据整理预览"));
}

void SDevKitWorkflowWidget::SelectWorkflow(int32 Index)
{
    if (!Workflows.IsValidIndex(Index)) return;
    ActiveWorkflow = Index;
    SelectStep(0);
}

void SDevKitWorkflowWidget::SelectStep(int32 Index)
{
    if (!Workflows.IsValidIndex(ActiveWorkflow) || !Workflows[ActiveWorkflow].Steps.IsValidIndex(Index)) return;
    ActiveStep = Index;
    const FStep& Step = Workflows[ActiveWorkflow].Steps[Index];
    TSharedPtr<SWidget>& Cached = ToolCache.FindOrAdd(Step.Id);
    if (!Cached.IsValid()) Cached = MakeTool(Step);
    if (Step.Id == TEXT("character_lookdev"))
        StaticCastSharedPtr<SDevKitCharacterLookDevWidget>(Cached)->Activate();
    Content->SetContent(Cached.ToSharedRef());
    RebuildNavigation();
}

bool SDevKitWorkflowWidget::SelectStepById(const FString& StepId)
{
    for (int32 WorkflowIndex = 0; WorkflowIndex < Workflows.Num(); ++WorkflowIndex)
        for (int32 StepIndex = 0; StepIndex < Workflows[WorkflowIndex].Steps.Num(); ++StepIndex)
            if (Workflows[WorkflowIndex].Steps[StepIndex].Id == StepId)
            {
                ActiveWorkflow = WorkflowIndex;
                SelectStep(StepIndex);
                return true;
            }
    return false;
}

FText SDevKitWorkflowWidget::GetStepSummary() const
{
    if (!Workflows.IsValidIndex(ActiveWorkflow) || !Workflows[ActiveWorkflow].Steps.IsValidIndex(ActiveStep))
        return FText::FromString(TEXT("发布流程：预检 → 构建 → 效果验收 → 版本发布 → UGS 验收。"));
    const FStep& Step = Workflows[ActiveWorkflow].Steps[ActiveStep];
    return FText::FromString(Workflows[ActiveWorkflow].Summary + TEXT("\n输入：") + Step.Input + TEXT("    →    产出：") + Step.Output);
}

TSharedRef<SWidget> SDevKitWorkflowWidget::MakeTool(const FStep& Step)
{
    // Lazy construction avoids scanning assets in every tool when the workbench opens.
    if (Step.Id == TEXT("rvt_material")) return SNew(SRVTMaterialManagerWidget);
    if (Step.Id == TEXT("material_rules")) return SNew(SMaterialTextureRulesWidget);
    if (Step.Id == TEXT("texture_audit")) return SNew(STextureVTAuditWidget);
    if (Step.Id == TEXT("texture_collection")) return SNew(SVirtualTextureCollectionManagerWidget);
    if (Step.Id == TEXT("stylized_lighting")) return MakeStylizedLightingSettingsPanel();
    if (Step.Id == TEXT("emissive")) return SNew(SStylizedEmissiveLibraryWidget);
    if (Step.Id == TEXT("character_lookdev")) return SNew(SDevKitCharacterLookDevWidget);
    if (Step.Id == TEXT("graphics_quality")) return SNew(SDevKitGraphicsQualityWorkbenchWidget);
    if (Step.Id == TEXT("surface_library")) return SNew(SDevKitRVTMeshDecalWidget);
    if (Step.Id == TEXT("level_rvt")) return SNew(SDevKitLevelRVTWidget);
    if (Step.Id == TEXT("map_creator")) return SNew(SDevKitMapCreatorWidget);
    if (Step.Id == TEXT("scene_assets")) return SNew(SDevKitSceneWorkbenchWidget);
    if (Step.Id == TEXT("level_data")) return SNew(SLevelDataWorkbenchWidget);
    if (Step.Id == TEXT("model_check")) return SNew(SModelAssetComplianceWidget);
    if (Step.Id == TEXT("env_tags")) return SNew(SEnvBatchTaggerWidget);
    if (Step.Id == TEXT("level_batch")) return SNew(SDevKitLevelBatchProcessorWidget);
    if (Step.Id == TEXT("character")) return SNew(SCharacterBalanceWidget);
    if (Step.Id == TEXT("weapon")) return SNew(SWeaponManagerWidget);
    if (Step.Id == TEXT("enemy")) return SNew(SEnemyManagerWidget);
    if (Step.Id == TEXT("rune")) return SNew(SRuneEditorWidget);
    if (Step.Id == TEXT("rune_balance")) return SNew(SDataEditorWidget);
    if (Step.Id == TEXT("actions")) return SNew(SActionBalanceWidget);
    if (Step.Id == TEXT("progression")) return SNew(SMetaProgressionWorkbenchWidget);
    if (Step.Id == TEXT("story")) return SNew(SStoryEncounterWorkbenchWidget);
    if (Step.Id == TEXT("buff_debug")) return SNew(SBuffFlowDebugWidget);
    if (Step.Id == TEXT("runtime_gm")) return SNew(SRuntimeGMSettingsWidget);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(16.f)
        [SNew(STextBlock).Text(FText::FromString(Step.Output)).AutoWrapText(true)]
        + SVerticalBox::Slot().AutoHeight().Padding(16.f)
        [SNew(SButton).Text(FText::FromString(TEXT("打开操作模式 / 独立工具")))
            .OnClicked_Lambda([this, Step](){return OpenExternal(Step);})];
}

TSharedRef<SWidget> SDevKitWorkflowWidget::MakeStylizedLightingSettingsPanel()
{
    return SNew(SDevKitRenderStyleWorkbenchWidget);
}

bool SDevKitWorkflowWidget::SaveGlobalEnvironmentDefaults(const FString& ConfigPath, FString& Status)
{
    // Compatibility entry for old callers: retired settings must never be written again.
    Status = TEXT("自定义分层与曝光配置已移除；未写入配置文件。请使用 UE 原生灯光、相机和 Post Process 设置。");
    return false;
}

FReply SDevKitWorkflowWidget::OpenExternal(const FStep& Step)
{
    if (Step.Id == TEXT("decal_placement")) OpenDecalPlacement.ExecuteIfBound();
    else if (FGlobalTabmanager::Get()->HasTabSpawner(Step.Tab)) FGlobalTabmanager::Get()->TryInvokeTab(Step.Tab);
    return FReply::Handled();
}

TSharedRef<SWidget> SDevKitWorkflowWidget::MakeOperationsPage()
{
    TSharedRef<SVerticalBox> Page = SNew(SVerticalBox);
    Page->AddSlot().AutoHeight().Padding(8.f)
    [DevKitWorkbenchUI::MakeSection(TEXT("01  版本与工作区检查"), SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
            [SNew(STextBlock).Text(FText::FromString(TEXT("核对 UGS 主工作区的 Engine、项目与插件模块；发布前再检查 P4 opened / pending / resolve。"))).AutoWrapText(true)]
            + SVerticalBox::Slot().AutoHeight()
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
                    .ContentPadding(FMargin(14.f, 8.f)).Text(FText::FromString(TEXT("检查模块匹配")))
                    .IsEnabled_Lambda([this](){return !bOperationBusy;})
                    .OnClicked_Lambda([this](){return RunReadOnlyCheck(TEXT("buildids"));})]
                + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f)
                [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                    .ContentPadding(FMargin(14.f, 8.f)).Text(FText::FromString(TEXT("发布只读预检")))
                    .IsEnabled_Lambda([this](){return !bOperationBusy;})
                    .OnClicked_Lambda([this](){return RunReadOnlyCheck(TEXT("preflight"));})]]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
            [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(OperationSummary);}).AutoWrapText(true)
                .ColorAndOpacity_Lambda([this](){return bOperationBusy || OperationDetails.IsEmpty() ? FStyleColors::Foreground
                    : (bOperationPassed ? FStyleColors::Success : FStyleColors::Warning);})]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
            [SNew(SExpandableArea).InitiallyCollapsed(true)
                .HeaderContent()[SNew(STextBlock).Text(FText::FromString(TEXT("检查详情与输出")))]
                .BodyContent()[SNew(SBox).HeightOverride(240.f)
                    [SNew(SMultiLineEditableTextBox).IsReadOnly(true).AutoWrapText(true)
                        .Text_Lambda([this](){return FText::FromString(OperationDetails);})]]])];

    const auto AddStage = [&Page](const TCHAR* Title, const TCHAR* Description)
    {
        Page->AddSlot().AutoHeight().Padding(8.f, 4.f)
        [DevKitWorkbenchUI::MakeSection(Title,
            SNew(STextBlock).Text(FText::FromString(Description)).AutoWrapText(true))];
    };
    AddStage(TEXT("02  本地开发与效果验收"), TEXT("在 X:/Dev-BuildEngine 修改源码。构建前保存并关闭编辑器，使用独立开发副本的 Binaries / Intermediate。重新打开开发副本，人工确认效果；编译成功不代表效果验收通过。"));
    AddStage(TEXT("03  选择发布路径"), TEXT("只改项目 / 插件：提交已审查代码 → 云端构建 PCB。改引擎 / Shader：生成 source-free Installed Build → 验证 BuildId → 发布 Engine CL → 固定项目 import @CL，再构建 PCB。"));
    AddStage(TEXT("04  云端构建与 UGS 验收"), TEXT("云端只编译项目与插件，不编译完整 UE。核对 ProjectCL / CodeCL / EngineCL 和 BuildId，执行 PCB verify，再用干净 UGS workspace 启动。发布需要单独确认，本面板不会自动创建 CL 或上传。"));
    Page->AddSlot().AutoHeight().Padding(8.f)
    [DevKitWorkbenchUI::MakeSection(TEXT("05  数据整理"), SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
            [SNew(STextBlock).Text(FText::FromString(TEXT("保留资产原路径、引用、存档与自动保存。先扫描重复日志；资产分类只建立本地集合，不移动或删除资产。"))).AutoWrapText(true)]
            + SVerticalBox::Slot().AutoHeight()
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                    .Text(FText::FromString(TEXT("扫描重复日志（不删除）"))).ContentPadding(FMargin(12.f, 8.f))
                    .IsEnabled_Lambda([this](){return !bOperationBusy;})
                    .OnClicked_Lambda([this](){return RunReadOnlyCheck(TEXT("cleanup-preview"));})]
                + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f)
                [SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                    .Text(FText::FromString(TEXT("建立本地资产分类集合"))).ContentPadding(FMargin(12.f, 8.f))
                    .IsEnabled_Lambda([this](){return !bOperationBusy;})
                    .OnClicked_Lambda([](){OrganizeAssets(); return FReply::Handled();})]])];
    return SNew(SScrollBox) + SScrollBox::Slot()[Page];
}

void SDevKitWorkflowWidget::ShowOperations()
{
    ActiveWorkflow = INDEX_NONE;
    RebuildNavigation();
    Content->SetContent(MakeOperationsPage());
}

FString SDevKitWorkflowWidget::GetOperationsRoot() const
{
    FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    FString Json, Host;
    TSharedPtr<FJsonObject> Marker;
    if (FFileHelper::LoadFileToString(Json, *(Root / TEXT("Dev01Development.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Marker) && Marker &&
        Marker->TryGetStringField(TEXT("hostProject"), Host) && FPaths::FileExists(Host / TEXT("DevKit.uproject")))
        Root = Host;
    return Root;
}

FReply SDevKitWorkflowWidget::RunReadOnlyCheck(FString Action)
{
    if (bOperationBusy || (Action != TEXT("buildids") && Action != TEXT("preflight") && Action != TEXT("cleanup-preview")))
        return FReply::Handled();
    const FString Root = GetOperationsRoot();
    FString Python = TEXT("C:/Python313/python.exe");
    FString Json, ConfiguredPython;
    TSharedPtr<FJsonObject> Settings;
    if (FFileHelper::LoadFileToString(Json, *(Root / TEXT("Saved/WorkflowHub/settings.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Settings) && Settings &&
        Settings->TryGetStringField(TEXT("pythonExecutable"), ConfiguredPython)) Python = ConfiguredPython;
    const FString Script = Root / TEXT("BuildScripts/Workflow/workflow_core.py");
    if (!FPaths::FileExists(Python) || !FPaths::FileExists(Script))
    {
        bOperationPassed = false;
        OperationSummary = TEXT("无法开始检查：缺少 Python 3.13+ 或工作流脚本。不会修改任何项目文件。");
        OperationDetails = TEXT("请在 Saved/WorkflowHub/settings.json 设置 pythonExecutable 的完整路径。\n") + Script;
        return FReply::Handled();
    }
    bOperationBusy = true;
    OperationSummary = TEXT("正在只读检查… 可以继续使用其他工具，请勿重复点击。");
    OperationDetails.Empty();
    const TWeakPtr<SDevKitWorkflowWidget> WeakThis = SharedThis(this);
    // Fixed read-only action allowlist; no shell interpolation or publication command.
    Async(EAsyncExecution::ThreadPool, [WeakThis, Python, Script, Root, Action]()
    {
        int32 Code = -1;
        FString Output, Error;
        const FString Args = FString::Printf(TEXT("-X utf8 \"%s\" --project \"%s\" %s"), *Script, *Root, *Action);
        const bool bStarted = FPlatformProcess::ExecProcess(*Python, *Args, &Code, &Output, &Error, *Root, true);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Code, bStarted, Output, Error, Action]()
        {
            const TSharedPtr<SDevKitWorkflowWidget> Self = WeakThis.Pin();
            if (!Self) return;
            Self->bOperationBusy = false;
            Self->bOperationPassed = false;
            Self->OperationDetails = Output + TEXT("\n") + Error;
            TSharedPtr<FJsonObject> Result;
            if (!bStarted || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Output), Result) || !Result)
            {
                Self->OperationSummary = TEXT("检查未完成。展开详情查看原因；不会把启动失败标记为通过。");
                return;
            }
            Self->bOperationPassed = Code == 0;
            if (Action == TEXT("buildids"))
                Self->OperationSummary = Code == 0 ? TEXT("模块 BuildId 一致。还需启动验证；这不代表 PCB 已发布。")
                    : TEXT("模块不匹配或缺失：请恢复对应的完整 PCB，不要仅修改 BuildId，也不要点击 Missing Modules 的 Yes。");
            else if (Action == TEXT("cleanup-preview"))
            {
                const TArray<TSharedPtr<FJsonValue>>* Candidates = nullptr;
                Result->TryGetArrayField(TEXT("candidates"), Candidates);
                Self->OperationSummary = FString::Printf(TEXT("扫描完成：%d 个可归档重复日志。本次未移动或删除文件。"), Candidates ? Candidates->Num() : 0);
            }
            else
            {
                Self->OperationSummary = Code == 0 ? TEXT("只读预检通过。发布仍需人工验收、明确提交范围与单独确认。") : TEXT("发布预检未通过：");
                const TArray<TSharedPtr<FJsonValue>>* Blockers = nullptr;
                if (Result->TryGetArrayField(TEXT("blockers"), Blockers))
                    for (const auto& Blocker : *Blockers) Self->OperationSummary += TEXT("\n• ") + Blocker->AsString();
            }
        });
    });
    return FReply::Handled();
}

void SDevKitWorkflowWidget::OrganizeAssets()
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(TEXT("/Game")));
    for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPlugins())
    {
        if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project && Plugin->CanContainContent())
        {
            FString Mount = Plugin->GetMountedAssetPath();
            Mount.RemoveFromEnd(TEXT("/"));
            Filter.PackagePaths.AddUnique(FName(*Mount));
        }
    }
    Filter.bRecursivePaths = true;
    TArray<FString> Roots;
    for (FName Path : Filter.PackagePaths) Roots.Add(Path.ToString());
    Registry.ScanPathsSynchronous(Roots, false);
    TArray<FAssetData> Assets;
    Registry.GetAssets(Filter, Assets);
    TMap<FName, TArray<FSoftObjectPath>> Groups;
    for (const FAssetData& Asset : Assets)
    {
        const FString Type = Asset.AssetClassPath.GetAssetName().ToString();
        FString Category = TEXT("Other");
        if (Asset.IsRedirector()) Category = TEXT("Redirectors_Review");
        else if (Type.Contains(TEXT("Material"))) Category = TEXT("Materials");
        else if (Type.Contains(TEXT("Texture"))) Category = TEXT("Textures");
        else if (Type == TEXT("StaticMesh") || Type == TEXT("SkeletalMesh")) Category = TEXT("Meshes");
        else if (Type.Contains(TEXT("Niagara")) || Type.Contains(TEXT("Particle"))) Category = TEXT("Effects");
        else if (Type == TEXT("World")) Category = TEXT("Levels");
        else if (Type.Contains(TEXT("Data")) || Type.Contains(TEXT("Ability")) || Type.Contains(TEXT("Flow"))) Category = TEXT("GameplayData");
        Groups.FindOrAdd(FName(*(TEXT("Dev01WF_") + Category))).Add(Asset.GetSoftObjectPath());
    }
    const TSharedRef<ICollectionContainer>& Collections = FCollectionManagerModule::GetModule().Get().GetProjectCollectionContainer();
    FString Report = FString::Printf(TEXT("ScannedAssets=%d\nPolicy=Local collections only; no asset moves or deletions.\n"), Assets.Num());
    int32 Failures = 0;
    for (const auto& Group : Groups)
    {
        FText Error;
        if (!Collections->CollectionExists(Group.Key, ECollectionShareType::CST_Local) &&
            !Collections->CreateCollection(Group.Key, ECollectionShareType::CST_Local, ECollectionStorageMode::Static, &Error))
        {
            Report += Group.Key.ToString() + TEXT(" ERROR: ") + Error.ToString() + TEXT("\n");
            ++Failures;
            continue;
        }
        if (!Collections->AddToCollection(Group.Key, ECollectionShareType::CST_Local, Group.Value, nullptr, &Error))
        {
            Report += Group.Key.ToString() + TEXT(" ERROR: ") + Error.ToString() + TEXT("\n");
            ++Failures;
            continue;
        }
        Report += FString::Printf(TEXT("%s=%d\n"), *Group.Key.ToString(), Group.Value.Num());
    }
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports");
    IFileManager::Get().MakeDirectory(*Directory, true);
    FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("asset-collections.txt")));
    UE_LOG(LogTemp, Display, TEXT("DEV01_WORKFLOW_COLLECTIONS assets=%d groups=%d failures=%d\n%s"), Assets.Num(), Groups.Num(), Failures, *Report);
    if (!FApp::IsUnattended()) FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(
        Failures ? TEXT("部分集合建立失败，请查看 Saved/WorkflowHub/Reports/asset-collections.txt。") :
        TEXT("已在内容浏览器本地集合中按 Dev01WF_* 分类。资产路径和原有集合保持不变。")));
}
