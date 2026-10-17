#include "Tools/ActionWorkbench/SDevKitActionWorkbenchWidget.h"
#include "Tools/ActionWorkbench/SDevKitActionRuleSourcePanel.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformFileManager.h"
#include "IDetailsView.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitActionWorkbench"

namespace ActionWorkbenchUI
{
	const FName TabId(TEXT("DevKitActionWorkbench"));
	FText Text(const TCHAR* Value) { return FText::FromString(Value); }
	TSharedRef<SWidget> Label(const FText& Value, int32 Size = 10, FSlateColor Color = FStyleColors::Foreground)
	{
		return SNew(STextBlock).Text(Value).Font(FCoreStyle::GetDefaultFontStyle(Size >= 14 ? "Bold" : "Regular", Size))
			.ColorAndOpacity(Color).AutoWrapText(true);
	}
	TSharedRef<SWidget> Section(const FText& Title, const FText& Description, const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Panel).Padding(18)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Label(Title, 14)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 14)[Label(Description, 10, FStyleColors::ForegroundHeader)]
			+ SVerticalBox::Slot().FillHeight(1)[Content]
		];
	}
	bool IsPlaySession()
	{
		if (!GEditor) return false;
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
			if (Context.WorldType == EWorldType::PIE) return true;
		return GEditor->PlayWorld != nullptr || GEditor->bIsSimulatingInEditor;
	}
	FText CellText(const FDevKitActionWorkbenchCell& Cell)
	{
		if (Cell.bAmbiguous) return Text(TEXT("规则冲突"));
		if (Cell.bBlocked && Cell.bCancels) return Text(TEXT("阻止 + 取消"));
		if (Cell.bBlocked) return Text(TEXT("阻止激活"));
		if (Cell.bCancels) return Text(TEXT("进入后取消"));
		return Text(TEXT("本表未限制"));
	}
	FSlateColor CellColor(const FDevKitActionWorkbenchCell& Cell)
	{
		if (Cell.bAmbiguous || (Cell.bBlocked && Cell.bCancels)) return FStyleColors::Warning;
		if (Cell.bBlocked) return FStyleColors::AccentRed;
		if (Cell.bCancels) return FStyleColors::AccentBlue;
		return FStyleColors::ForegroundHeader;
	}
	FString TagLines(const FGameplayTagContainer& Tags)
	{
		TArray<FString> Names;
		for (const FGameplayTag& Tag : Tags) Names.Add(Tag.ToString());
		Names.Sort();
		return Names.IsEmpty() ? TEXT("  无\n") : TEXT("  ") + FString::Join(Names, TEXT("\n  ")) + TEXT("\n");
	}
	FAutoConsoleCommand OpenCommand(TEXT("DevKit.ActionWorkbench.Open"),
		TEXT("Open the native Dev01 action/state conflict workbench without changing any rule."),
		FConsoleCommandDelegate::CreateStatic(&SDevKitActionWorkbenchWidget::OpenTab));
}

using namespace ActionWorkbenchUI;

void SDevKitActionWorkbenchWidget::RegisterTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateStatic(&SDevKitActionWorkbenchWidget::SpawnTab))
		.SetDisplayName(LOCTEXT("Title", "动作工作台"))
		.SetTooltipText(LOCTEXT("Tooltip", "集中编辑状态阻断规则，检查动作关系与 PIE 实时状态。"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.GameplayAbility"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void SDevKitActionWorkbenchWidget::UnregisterTab() { FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId); }
void SDevKitActionWorkbenchWidget::OpenTab() { FGlobalTabmanager::Get()->TryInvokeTab(TabId); }

TSharedRef<SDockTab> SDevKitActionWorkbenchWidget::SpawnTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDevKitActionWorkbenchWidget> Workbench = SNew(SDevKitActionWorkbenchWidget);
	TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab)[Workbench];
	Tab->SetCanCloseTab(SDockTab::FCanCloseTab::CreateSP(Workbench, &SDevKitActionWorkbenchWidget::CanClose));
	return Tab;
}

void SDevKitActionWorkbenchWidget::Construct(const FArguments& InArgs)
{
	Page = 4; // External sources are the lightweight default; legacy pages remain available.
	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddSPLambda(this,
		[this](const FString& Filename, UPackage* Package, FObjectPostSaveContext Context)
		{
			if (Model.HasPendingApply() && Model.GetSource() && Package == Model.GetSource()->GetOutermost()
				&& !Context.IsFromAutoSave() && !Context.IsCooking() && !Context.IsProceduralSave()
				&& Context.IsUpdatingLoadedPath() && Context.SaveSucceeded())
				bObservedPackageSave = true; // Confirm on Tick, after the complete save and dirty-state update.
		});
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DraftDetails = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsArgs);
	DraftDetails->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &SDevKitActionWorkbenchWidget::CanEdit));
	DraftDetails->OnFinishedChangingProperties().AddSPLambda(this, [this](const FPropertyChangedEvent&) { Refresh(); });
	Status = LOCTEXT("InitialStatus", "只编辑草稿；应用并保存后，重新进入 PIE 才会读取新规则。");

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Recessed).Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[BuildHeader()]
			+ SVerticalBox::Slot().FillHeight(1).Padding(10, 8)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value_Lambda([this] { return Page == 4 ? 0.f : 0.20f; }).MinSize_Lambda([this] { return Page == 4 ? 0.f : 200.f; })
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Panel).Padding(14)
					.Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Label(LOCTEXT("CatalogTitle", "动作目录"), 14)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 12)[Label(LOCTEXT("CatalogHint", "既有 Tag 映射 · 不自动注册新名称"), 9, FStyleColors::ForegroundHeader)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
						[
							SNew(SSearchBox).HintText(LOCTEXT("Search", "搜索动作 / Tag"))
							.OnTextChanged_Lambda([this](const FText& Value) { SearchText = Value.ToString(); CatalogBox->SetContent(BuildCatalog()); })
						]
						+ SVerticalBox::Slot().FillHeight(1)[SAssignNew(CatalogBox, SBox)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
						[
							Label(LOCTEXT("CatalogBoundary", "目录是状态标签投影，不会自动创建 GA、武器或动画。普攻槽位保留 Combo1–4。"), 9, FStyleColors::ForegroundHeader)
						]
					]
				]
				+ SSplitter::Slot().Value(0.80f).MinSize(560)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 0, 8)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(12, 10)).Text(LOCTEXT("RuleSourcesTab", "表格 / Lua 规则源"))
							.ForegroundColor_Lambda([this] { return Page == 4 ? FStyleColors::AccentBlue : FStyleColors::Foreground; })
							.OnClicked_Lambda([this] { Page = 4; Pages->SetActiveWidgetIndex(Page); return FReply::Handled(); })]
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(12, 10)).Text(LOCTEXT("MatrixTab", "旧资产矩阵"))
							.ForegroundColor_Lambda([this] { return Page == 0 ? FStyleColors::AccentBlue : FStyleColors::Foreground; })
							.OnClicked_Lambda([this] { Page = 0; Pages->SetActiveWidgetIndex(Page); Refresh(); return FReply::Handled(); })]
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(18, 10)).Text(LOCTEXT("SourceTab", "状态与资源"))
							.ForegroundColor_Lambda([this] { return Page == 1 ? FStyleColors::AccentBlue : FStyleColors::Foreground; })
							.OnClicked_Lambda([this] { Page = 1; Pages->SetActiveWidgetIndex(Page); return FReply::Handled(); })]
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(18, 10)).Text(LOCTEXT("DiagTab", "运行诊断"))
							.ForegroundColor_Lambda([this] { return Page == 2 ? FStyleColors::AccentBlue : FStyleColors::Foreground; })
							.OnClicked_Lambda([this] { Page = 2; Pages->SetActiveWidgetIndex(Page); RefreshActors(); return FReply::Handled(); })]
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(18, 10)).Text(LOCTEXT("ValidateTab", "校验与范围"))
							.ForegroundColor_Lambda([this] { return Page == 3 ? FStyleColors::AccentBlue : FStyleColors::Foreground; })
							.OnClicked_Lambda([this] { Page = 3; Pages->SetActiveWidgetIndex(Page); Refresh(); return FReply::Handled(); })]
					]
					+ SVerticalBox::Slot().FillHeight(1).Padding(8, 0, 0, 0)
					[
						SAssignNew(Pages, SWidgetSwitcher)
						.WidgetIndex(4)
						+ SWidgetSwitcher::Slot()
						[
							SNew(SSplitter)
							+ SSplitter::Slot().Value(0.66f).MinSize(320)[SAssignNew(MatrixBox, SBox)]
							+ SSplitter::Slot().Value(0.34f).MinSize(250)[SAssignNew(InspectorBox, SBox)]
						]
						+ SWidgetSwitcher::Slot()[BuildSourcePage()]
						+ SWidgetSwitcher::Slot()[BuildDiagnostics()]
						+ SWidgetSwitcher::Slot()[SAssignNew(ValidationBox, SBox)]
						+ SWidgetSwitcher::Slot()[SNew(SDevKitActionRuleSourcePanel)]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Panel).Padding(FMargin(20, 10))
				[SNew(STextBlock).Text_Lambda([this] { return Page == 4 ? LOCTEXT("SourceFooter", "外部规则源为显式启用的扩展层；旧资产 / GAS / 冷却限制保留。打开或校验不会改变运行规则。") : Status; }).AutoWrapText(true).ColorAndOpacity(FStyleColors::ForegroundHeader)]
			]
		]
	];

	UStateConflictDataAsset* Initial = InArgs._InitialAsset;
	if (!Initial)
	{
		FString ConfiguredPath;
		GConfig->GetString(TEXT("/Script/DevKit.DevAssetManager"), TEXT("StateConflictData"), ConfiguredPath, GGameIni);
		Initial = Cast<UStateConflictDataAsset>(FSoftObjectPath(ConfiguredPath).TryLoad());
	}
	LoadAsset(Initial);
}

SDevKitActionWorkbenchWidget::~SDevKitActionWorkbenchWidget()
{
	UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildHeader()
{
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Panel).Padding(FMargin(20, 16))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Label(LOCTEXT("Brand", "DESIGNER  /  ACTION SYSTEM"), 9, FStyleColors::AccentBlue)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 4)[Label(LOCTEXT("Title", "动作工作台"), 22)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(this, &SDevKitActionWorkbenchWidget::SessionText).ColorAndOpacity(FStyleColors::ForegroundHeader)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)[SNew(SButton).Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; }).Text(LOCTEXT("Reload", "重新载入")).OnClicked(this, &SDevKitActionWorkbenchWidget::ReloadDraft)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)[SNew(SButton).Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; }).Text(LOCTEXT("Export", "导出草稿")).OnClicked(this, &SDevKitActionWorkbenchWidget::ExportDraft)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)[SNew(SButton).Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; }).Text(LOCTEXT("Validate", "校验")).OnClicked(this, &SDevKitActionWorkbenchWidget::ValidateDraft)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "PrimaryButton").Text(LOCTEXT("Apply", "应用到资产"))
				.Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; })
				.ToolTipText(LOCTEXT("ApplyHint", "仅写回所选规则资产并标记未保存；不保存磁盘、不提交 P4、不刷新运行角色。"))
				.IsEnabled(this, &SDevKitActionWorkbenchWidget::CanApply).OnClicked(this, &SDevKitActionWorkbenchWidget::ApplyDraft)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
			[
				SNew(SButton).Text(LOCTEXT("Save", "保存资产"))
				.Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; })
				.IsEnabled_Lambda([this] { return bAppliedUnsaved && !IsPlaySession() && !bSourceStale && !Model.IsDirty(); })
				.OnClicked(this, &SDevKitActionWorkbenchWidget::SaveSource)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
		[
			SNew(SHorizontalBox).Visibility_Lambda([this] { return Page == 4 ? EVisibility::Collapsed : EVisibility::Visible; })
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
			[SNew(STextBlock).Text(LOCTEXT("Authority", "旧资产来源" )).ColorAndOpacity(FStyleColors::ForegroundHeader)]
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(SObjectPropertyEntryBox).AllowedClass(UStateConflictDataAsset::StaticClass()).AllowClear(false)
				.ObjectPath_Lambda([this] { return Model.GetSource() ? Model.GetSource()->GetPathName() : FString(); })
				.OnObjectChanged(this, &SDevKitActionWorkbenchWidget::OnAssetPicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)[SNew(SButton).Text(LOCTEXT("Locate", "定位资产")).OnClicked(this, &SDevKitActionWorkbenchWidget::LocateSource)]
		]
	];
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildCatalog()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (const FDevKitActionWorkbenchAction& Action : Actions)
	{
		if (!SearchText.IsEmpty() && !Action.DisplayName.ToString().Contains(SearchText) && !Action.Id.ToString().Contains(SearchText)) continue;
		List->AddSlot().AutoHeight().Padding(0, 0, 0, 5)
		[
			SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(10, 10))
			.ToolTipText(FText::FromString(Action.Id.ToString()))
			.OnClicked_Lambda([this, Id = Action.Id] { SelectPair(Id, CurrentTag); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Action.DisplayName).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity_Lambda([this, Id = Action.Id] { return Id == RequestTag ? FStyleColors::AccentBlue : FStyleColors::Foreground; }).AutoWrapText(true)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[Label(Action.bBuiltin ? LOCTEXT("CoreMapping", "核心动作 · 既有映射") : LOCTEXT("AssetMapping", "规则资产 · 状态映射"), 9, FStyleColors::ForegroundHeader)]
			]
		];
	}
	return SNew(SScrollBox) + SScrollBox::Slot()[List];
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildMatrix()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	TSharedRef<SHorizontalBox> Heading = SNew(SHorizontalBox);
	Heading->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(132).Padding(10)[Label(LOCTEXT("Direction", "↓ 新请求  /  当前 →"), 9, FStyleColors::ForegroundHeader)]];
	for (const auto& Action : Actions)
		Heading->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(128).Padding(8)[Label(Action.DisplayName, 10)]];
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Heading];
	for (const auto& Request : Actions)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(132).Padding(10)[Label(Request.DisplayName, 10)]];
		for (const auto& Current : Actions)
		{
			const FDevKitActionWorkbenchCell Cell = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Request.Id, Current.Id);
			Row->AddSlot().AutoWidth().Padding(2)
			[
				SNew(SBox).WidthOverride(124).HeightOverride(58)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(1)
					.BorderBackgroundColor_Lambda([this, R = Request.Id, C = Current.Id] { return R == RequestTag && C == CurrentTag ? FStyleColors::Primary : FStyleColors::InputOutline; })
					[
						SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(6).HAlign(HAlign_Center)
						.ToolTipText(Cell.Explanation)
						.OnClicked_Lambda([this, R = Request.Id, C = Current.Id] { SelectPair(R, C); return FReply::Handled(); })
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[SNew(STextBlock).Text(CellText(Cell)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 10)).ColorAndOpacity(CellColor(Cell))]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 0)
							[Label(Cell.bInheritedBlock || Cell.bInheritedCancel ? LOCTEXT("ParentTarget", "含上级目标匹配") : LOCTEXT("InspectCell", "查看规则"), 8, FStyleColors::ForegroundHeader)]
						]
					]
				]
			];
		}
		Rows->AddSlot().AutoHeight()[Row];
	}
	return Section(LOCTEXT("MatrixTitle", "有向动作关系"),
		LOCTEXT("MatrixSubtitle", "行是新请求，列是当前状态。点击一个格子，在右侧编辑两个独立关系。"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox) + SScrollBox::Slot()[SNew(SScrollBox).Orientation(Orient_Horizontal) + SScrollBox::Slot()[Rows]]]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
		[Label(LOCTEXT("MatrixLegend", "红色：阻止激活   蓝色：进入后取消   灰色：仅本表未限制\n无表内限制 ≠ GAS 最终允许；取消并不保证成功，也不会跳过成本、冷却或输入锁。"), 9, FStyleColors::ForegroundHeader)]);
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildInspector()
{
	const FDevKitActionWorkbenchCell Cell = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), RequestTag, CurrentTag);
	const bool bDash = RequestTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash"), false))
		|| CurrentTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash"), false));
	return Section(LOCTEXT("InspectorTitle", "选中关系"), LOCTEXT("InspectorSubtitle", "只修改这一方向，不联动反向格。"),
		SNew(SScrollBox) + SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Label(LOCTEXT("IncomingLabel", "新请求"), 9, FStyleColors::ForegroundHeader)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 14)[Label(ActionName(RequestTag), 18, FStyleColors::AccentBlue)]
			+ SVerticalBox::Slot().AutoHeight()[Label(LOCTEXT("CurrentLabel", "当前状态"), 9, FStyleColors::ForegroundHeader)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 18)[Label(ActionName(CurrentTag), 15)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
			[
				SNew(SCheckBox).IsEnabled_Lambda([this, Cell] { return CanEdit() && !Cell.bAmbiguous; })
				.IsChecked(Cell.bBlocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState Value) { SetCell(false, Value == ECheckBoxState::Checked); })
				[Label(LOCTEXT("BlockCheck", "阻止新请求激活"), 11)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
			[
				SNew(SCheckBox).IsEnabled_Lambda([this, Cell] { return CanEdit() && !Cell.bAmbiguous; })
				.IsChecked(Cell.bCancels ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState Value) { SetCell(true, Value == ECheckBoxState::Checked); })
				[Label(LOCTEXT("CancelCheck", "进入后取消当前动作"), 11)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[Label(Cell.Explanation, 10, FStyleColors::ForegroundHeader)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
			[
				Label(bDash ? LOCTEXT("DashDeferred", "冲刺需求此前暂缓。工作台没有预置新优先级；这里的手工修改也只进入草稿，应用前请单独验收。")
					: LOCTEXT("NoFinalAdmission", "表内没有限制时，旧 GA、蓝图、蒙太奇窗口和输入层仍可拒绝动作。"), 10, FStyleColors::Warning)
			]
			+ SVerticalBox::Slot().AutoHeight()[Label(LOCTEXT("MappingTitle", "底层映射 · 只读"), 11)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 7)[Label(FText::FromString(RequestTag.ToString() + TEXT("\n→\n") + CurrentTag.ToString()), 9, FStyleColors::ForegroundHeader)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12)[Label(LOCTEXT("FutureFeatures", "排队 / 共存 / 动画窗口：未接入\n当前不会把演示规则写成真实运行逻辑。"), 9, FStyleColors::ForegroundHeader)]
		]
	);
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildSourcePage()
{
	return Section(LOCTEXT("SourceTitle", "权威草稿 · 状态与资源限制"),
		LOCTEXT("SourceDescription", "直接编辑现有 StateConflict 结构，不创建第二套运行时策略。Rules 是能力限制；BlockCategoryMap 是系统资源触发状态。"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
		[
			Label(LOCTEXT("RawContract", "ActiveTag 精确触发；BlockTags / CancelTags 匹配 GA 身份标签。Priority 当前不参与运行时判定。\n资源类别只接通 Block.Movement 与 Block.AI；删除表项不会解除死亡、受击、对话等其他系统限制。"), 10, FStyleColors::Warning)
		]
		+ SVerticalBox::Slot().FillHeight(1)[DraftDetails.ToSharedRef()]);
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildDiagnostics()
{
	return Section(LOCTEXT("DiagnosticTitle", "PIE 实时观察"),
		LOCTEXT("DiagnosticDescription", "只读快照，不发送输入、不激活或取消能力、不清除任何锁，也不读取或写入玩家存档。"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SAssignNew(ActorPicker, SComboBox<FActorOption>).OptionsSource(&ActorOptions)
				.OnComboBoxOpening_Lambda([this] { RefreshActors(); })
				.OnGenerateWidget_Lambda([](FActorOption Option)
				{
					AActor* Actor = Option.IsValid() ? Option->Get() : nullptr;
					return Label(FText::FromString(Actor && Actor->GetWorld() ? Actor->GetActorLabel() + TEXT(" · ") + Actor->GetWorld()->GetName() : TEXT("已销毁")));
				})
				.OnSelectionChanged_Lambda([this](FActorOption Option, ESelectInfo::Type) { SelectedActor = Option; RefreshDiagnosticText(); })
				[SNew(STextBlock).Text_Lambda([this]
				{
					AActor* Actor = SelectedActor.IsValid() ? SelectedActor->Get() : nullptr;
					return Actor && Actor->GetWorld() ? FText::FromString(Actor->GetActorLabel() + TEXT(" · ") + Actor->GetWorld()->GetName()) : LOCTEXT("PickActor", "选择 PIE 中带 ASC 的角色…");
				})]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
			[SNew(SButton).Text(LOCTEXT("CopyDiagnostic", "复制诊断报告")).OnClicked_Lambda([this] { RefreshDiagnosticText(); FPlatformApplicationMisc::ClipboardCopy(*Diagnostics.ToString()); Status = LOCTEXT("Copied", "已复制实时观察报告；这是快照，不是完整的动作裁决日志。"); return FReply::Handled(); })]
		]
		+ SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox) + SScrollBox::Slot()[SNew(STextBlock).Text_Lambda([this] { return Diagnostics; }).AutoWrapText(true)]]);
}

TSharedRef<SWidget> SDevKitActionWorkbenchWidget::BuildValidation()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (const auto& Issue : Model.Validate())
		List->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[Label(FText::Format(Issue.bError ? LOCTEXT("ErrorPrefix", "错误  ·  {0}") : LOCTEXT("WarnPrefix", "提示  ·  {0}"), Issue.Message), 11, Issue.bError ? FStyleColors::Error : FStyleColors::Warning)];
	List->AddSlot().AutoHeight().Padding(0, 14, 0, 12)[Label(LOCTEXT("VersionScope", "本版已接通"), 14)];
	List->AddSlot().AutoHeight()[Label(LOCTEXT("ImplementedScope", "既有状态目录、有向阻止/取消矩阵、原生 Tag 选择器、移动/AI 分类、独立草稿、校验、冲突检测、应用事务、显式保存与 PIE 只读快照。"))];
	List->AddSlot().AutoHeight().Padding(0, 18, 0, 12)[Label(LOCTEXT("DeferredScope", "尚未接通，不作保证"), 14)];
	List->AddSlot().AutoHeight()[Label(LOCTEXT("NotImplementedScope", "按帧拖拽的动画窗口、队列/共存策略、锁所有者账本、原子取消/激活事务、规则预设继承、表格批量导入。当前不提供全局白名单，也不承诺解决所有旧系统残锁。\n\n运行验证需重新进入 PIE；成本/冷却、蒙太奇和输入层是独立检查。保存不等于已发布，工具不会提交 P4 或 GitHub。"), 10, FStyleColors::ForegroundHeader)];
	return Section(LOCTEXT("ValidationTitle", "校验结果与交付边界"), LOCTEXT("ValidationDescription", "结构错误阻止应用；告警保留真实运行时语义，不把未知解释成允许。"), SNew(SScrollBox) + SScrollBox::Slot()[List]);
}

void SDevKitActionWorkbenchWidget::Refresh()
{
	Actions = FDevKitActionWorkbenchModel::GetActions(Model.GetDraft());
	if (!Actions.ContainsByPredicate([this](const auto& A) { return A.Id == RequestTag; })) RequestTag = Actions.IsEmpty() ? FGameplayTag() : Actions[0].Id;
	if (!Actions.ContainsByPredicate([this](const auto& A) { return A.Id == CurrentTag; })) CurrentTag = Actions.IsEmpty() ? FGameplayTag() : Actions[0].Id;
	LastDraftSnapshot = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	bSourceStale = Model.HasSourceChanged();
	CatalogBox->SetContent(BuildCatalog());
	// The lightweight source page must not eagerly instantiate an N x N legacy grid.
	if (Page == 0) MatrixBox->SetContent(BuildMatrix());
	InspectorBox->SetContent(BuildInspector());
	ValidationBox->SetContent(BuildValidation());
}

void SDevKitActionWorkbenchWidget::LoadAsset(UStateConflictDataAsset* Asset)
{
	FText Message;
	if (Model.Load(Asset, Message))
	{
		DraftDetails->SetObject(Model.GetDraft());
		bAppliedUnsaved = false;
		bObservedPackageSave = false;
		Status = Message;
	}
	else Status = Message;
	Refresh();
}

void SDevKitActionWorkbenchWidget::OnAssetPicked(const FAssetData& Asset)
{
	if (Asset.GetSoftObjectPath().ToString() == SourcePathText().ToString()) return;
	if (ConfirmDiscard(LOCTEXT("SwitchAsset", "切换规则资产"))) LoadAsset(Cast<UStateConflictDataAsset>(Asset.GetAsset()));
}

void SDevKitActionWorkbenchWidget::SelectPair(FGameplayTag Request, FGameplayTag Current)
{
	RequestTag = Request; CurrentTag = Current;
	InspectorBox->SetContent(BuildInspector());
	Page = 0; Pages->SetActiveWidgetIndex(0);
	MatrixBox->SetContent(BuildMatrix());
}

FText SDevKitActionWorkbenchWidget::ActionName(FGameplayTag ActionTag) const
{
	for (const auto& Action : Actions) if (Action.Id == ActionTag) return Action.DisplayName;
	return FText::FromString(ActionTag.IsValid() ? ActionTag.ToString() : TEXT("未选择"));
}

FText SDevKitActionWorkbenchWidget::SourcePathText() const { return FText::FromString(Model.GetSource() ? Model.GetSource()->GetPathName() : FString()); }
FText SDevKitActionWorkbenchWidget::SessionText() const
{
	if (Page == 4) return LOCTEXT("ExternalSourceMode", "表格 / Lua 规则源  ·  只读检查与显式挂载");
	if (IsPlaySession()) return LOCTEXT("PIEReadOnly", "PIE 观察模式  ·  规则编辑与保存已锁定");
	if (bSourceStale) return LOCTEXT("StaleSource", "来源已变化  ·  应用被阻止，请先处理源资产冲突");
	if (Model.IsDirty()) return LOCTEXT("DraftDirty", "草稿有修改  ·  尚未影响正式规则");
	if (bAppliedUnsaved) return LOCTEXT("AppliedUnsaved", "已应用到资产  ·  尚未保存到磁盘");
	return LOCTEXT("CleanDraft", "独立草稿  ·  未改变运行规则");
}

bool SDevKitActionWorkbenchWidget::CanEdit() const { return Model.GetDraft() && !IsPlaySession() && !bSourceStale && !Model.HasPendingApply(); }
bool SDevKitActionWorkbenchWidget::CanApply() const { return CanEdit() && Model.IsDirty() && !bAppliedUnsaved; }
bool SDevKitActionWorkbenchWidget::ConfirmDiscard(const FText& Purpose) const
{
	if (!Model.IsDirty() && !bAppliedUnsaved) return true;
	return FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(LOCTEXT("DiscardPrompt", "{0}？\n\n未应用草稿将丢弃；已经应用但未保存的资产仍会保留在编辑器中，需另行保存。可以先导出草稿再继续。"), Purpose)) == EAppReturnType::Yes;
}
bool SDevKitActionWorkbenchWidget::CanClose() { return ConfirmDiscard(LOCTEXT("ClosePurpose", "关闭动作工作台")); }

FReply SDevKitActionWorkbenchWidget::SetCell(bool bCancel, bool bEnabled)
{
	if (!CanEdit()) return FReply::Handled();
	FText Message;
	if (bCancel) Model.SetCancel(RequestTag, CurrentTag, bEnabled, Message);
	else Model.SetBlocked(RequestTag, CurrentTag, bEnabled, Message);
	Status = Message;
	DraftDetails->ForceRefresh();
	Refresh();
	return FReply::Handled();
}

FReply SDevKitActionWorkbenchWidget::ValidateDraft()
{
	int32 Errors = 0;
	const auto Issues = Model.Validate();
	for (const auto& Issue : Issues) Errors += Issue.bError ? 1 : 0;
	Status = FText::Format(LOCTEXT("ValidationSummary", "校验完成：{0} 个错误，{1} 个提示。只验证规则结构，不代表游戏行为已验收。"), Errors, Issues.Num() - Errors);
	Refresh(); Page = 3; Pages->SetActiveWidgetIndex(Page);
	return FReply::Handled();
}

FReply SDevKitActionWorkbenchWidget::ApplyDraft()
{
	if (!CanApply()) return FReply::Handled();
	if (FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(LOCTEXT("ApplyConfirm", "将草稿应用到下列资产？\n{0}\n\n这是唯一受影响的资产。当前不会保存磁盘、热刷新角色或提交版本控制。应用可使用编辑器 Undo 撤销。"), SourcePathText())) != EAppReturnType::Yes) return FReply::Handled();
	FText Message;
	if (Model.ApplyDraft(Message)) bAppliedUnsaved = true;
	Status = Message;
	Refresh();
	return FReply::Handled();
}

FReply SDevKitActionWorkbenchWidget::SaveSource()
{
	if (!bAppliedUnsaved || IsPlaySession() || Model.IsDirty() || Model.HasSourceChanged() || !Model.GetSource())
	{
		Status = LOCTEXT("SaveRefused", "保存被阻止：请先应用本工作台草稿，处理来源变化并退出 PIE。"); return FReply::Handled();
	}
	UPackage* Package = Model.GetSource()->GetOutermost();
	TArray<UPackage*> Packages { Package };
	const FEditorFileUtils::EPromptReturnCode Result = FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, true);
	if (Result == FEditorFileUtils::PR_Success && !Package->IsDirty())
	{
		FText Message;
		if (Model.AcknowledgeSavedSource(Message))
		{
			bAppliedUnsaved = false;
			Status = LOCTEXT("Saved", "规则资产已保存。重新进入 PIE / 重新初始化角色后生效；未提交 P4、未发布 PCB。");
		}
		else Status = Message;
		Refresh();
	}
	else Status = LOCTEXT("SaveCancelled", "资产未保存（取消、只读或保存失败）。已应用内容仍留在编辑器中，请处理后重试。");
	return FReply::Handled();
}

FReply SDevKitActionWorkbenchWidget::ReloadDraft()
{
	if (IsPlaySession() || !ConfirmDiscard(LOCTEXT("ReloadPurpose", "重新载入草稿"))) return FReply::Handled();
	FText Message;
	if (Model.Reset(Message)) { DraftDetails->SetObject(Model.GetDraft()); bAppliedUnsaved = false; }
	Status = Message; Refresh();
	return FReply::Handled();
}

FReply SDevKitActionWorkbenchWidget::ExportDraft()
{
	if (!Model.GetDraft()) return FReply::Handled();
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("ActionWorkbench/Exports");
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString File = Directory / (TEXT("Draft_") + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S_")) + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8) + TEXT(".json"));
	if (FFileHelper::SaveStringToFile(FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		Status = FText::FromString(TEXT("草稿快照已导出：") + FPaths::ConvertRelativePathToFull(File) + TEXT("（评审用途，不会自动导入或启用）"));
	else Status = LOCTEXT("ExportFailed", "导出失败；源规则与草稿均未改变。");
	return FReply::Handled();
}

FReply SDevKitActionWorkbenchWidget::LocateSource()
{
	if (GEditor && Model.GetSource()) { TArray<UObject*> Assets { Model.GetSource() }; GEditor->SyncBrowserToObjects(Assets); }
	return FReply::Handled();
}

void SDevKitActionWorkbenchWidget::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
	if (CurrentTime < NextRefresh) return;
	NextRefresh = CurrentTime + 0.75;
	if (bObservedPackageSave)
	{
		bObservedPackageSave = false;
		if (Model.HasPendingApply())
		{
			FText Message;
			if (Model.AcknowledgeSavedSource(Message)) bAppliedUnsaved = false;
			Status = Message;
		}
	}
	bSourceStale = Model.HasSourceChanged();
	if (FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()) != LastDraftSnapshot) Refresh();
	if (Page == 2) RefreshDiagnosticText();
}

void SDevKitActionWorkbenchWidget::RefreshActors()
{
	AActor* PreviouslySelected = SelectedActor.IsValid() ? SelectedActor->Get() : nullptr;
	ActorOptions.Reset(); SelectedActor.Reset();
	if (GEditor)
	{
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if (Context.WorldType != EWorldType::PIE || !Context.World()) continue;
			for (TActorIterator<AActor> It(Context.World()); It; ++It)
			{
				if (!It->FindComponentByClass<UAbilitySystemComponent>()) continue;
				FActorOption Option = MakeShared<TWeakObjectPtr<AActor>>(*It);
				ActorOptions.Add(Option);
				if (*It == PreviouslySelected) SelectedActor = Option;
			}
		}
	}
	if (ActorPicker.IsValid())
	{
		ActorPicker->RefreshOptions();
		ActorPicker->SetSelectedItem(SelectedActor);
	}
	RefreshDiagnosticText();
}

void SDevKitActionWorkbenchWidget::RefreshDiagnosticText()
{
	AActor* Actor = SelectedActor.IsValid() ? SelectedActor->Get() : nullptr;
	UAbilitySystemComponent* ASC = Actor && Actor->GetWorld() && Actor->GetWorld()->WorldType == EWorldType::PIE ? Actor->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
	if (!ASC)
	{
		Diagnostics = LOCTEXT("NoRuntime", "等待 PIE 角色\n\n启动隔离测试 PIE 后，展开上方角色选择器。当前没有选中可观察的 ASC。\n\n工作台不会启动正式地图、加载玩家存档、发送测试输入或清空锁。这里只显示真实快照；不把草稿模拟结果当作已经执行。");
		return;
	}
	FString Report = FString::Printf(TEXT("实时快照  ·  %s\n角色：%s\n世界：%s\n\n"), *FDateTime::Now().ToString(), *Actor->GetActorLabel(), *Actor->GetWorld()->GetName());
	const FGameplayTagContainer Owned = ASC->GetOwnedGameplayTags();
	Report += TEXT("当前显式状态 / 效果标签\n") + TagLines(Owned);
	Report += TEXT("\nGAS 当前阻止的能力标签（真实计数投影）\n") + TagLines(ASC->GetBlockedAbilityTags());
	Report += TEXT("\n活跃能力（只读，不调用 CanActivateAbility）\n");
	int32 ActiveCount = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive() || !Spec.Ability) continue;
		++ActiveCount;
		Report += TEXT("  ") + Spec.Ability->GetName() + TEXT("\n") + TagLines(Spec.Ability->GetAssetTags());
	}
	if (!ActiveCount) Report += TEXT("  无\n");
	const UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
	if (const auto* Rules = Actor->GetWorld()->GetSubsystem<UDevKitActionRuleSubsystem>())
	{
		Report += TEXT("\n表格 / Lua 规则源\n  ") + Rules->GetStatus() + TEXT("\n");
		if (const auto Snapshot = Rules->GetSnapshot(); Snapshot && YogASC)
			for (const auto& Action : Snapshot->Actions)
			{
				const auto Decision = YogASC->QueryActionRules(Action.AbilityTags);
				Report += TEXT("  ") + Action.Id + (Decision.bBlocked ? TEXT(" : 阻止") : TEXT(" : 本规则源未阻止"))
					+ TEXT(" | ") + FString::Join(Decision.MatchedRules, TEXT(", ")) + TEXT("\n    ")
					+ FString::Join(Decision.Reasons, TEXT(" / ")) + TEXT("\n");
			}
	}
	const UStateConflictDataAsset* RuntimeTable = YogASC ? YogASC->ConflictTable.Get() : nullptr;
	Report += TEXT("\n角色实际规则来源\n  ") + (RuntimeTable ? RuntimeTable->GetPathName() : TEXT("无 Dev01 ConflictTable / 非 Yog ASC")) + TEXT("\n");
	if (RuntimeTable != Model.GetSource()) Report += TEXT("  注意：角色使用的表与工作台所选资产不同。\n");
	Report += TEXT("\n实际来源表中的状态匹配（不是缓存/历史锁账本）\n");
	if (RuntimeTable)
	{
		for (const FStateConflictRule& Rule : RuntimeTable->Rules)
		{
			if (!Owned.HasTagExact(Rule.ActiveTag)) continue;
			Report += TEXT("  状态：") + Rule.ActiveTag.ToString() + TEXT("\n  配置 Block：") + Rule.BlockTags.ToStringSimple() + TEXT("\n  配置 Cancel：") + Rule.CancelTags.ToStringSimple() + TEXT("\n");
		}
		for (const auto& Pair : RuntimeTable->BlockCategoryMap)
		{
			for (const FGameplayTag& State : Pair.Value)
				if (Owned.HasTagExact(State)) Report += TEXT("  资源配置：") + Pair.Key.ToString() + TEXT(" ← ") + State.ToString() + TEXT("\n");
		}
	}
	Report += TEXT("\n解释边界\n  表内匹配不是最终准入结果；角色缓存、GA/蓝图、不可取消状态、成本、冷却、输入/对话/死亡限制仍然独立。\n  此处没有 owner/获取时间/最后失败请求历史，不能由当前 Tag 推断锁已释放或取消已成功。\n  草稿未应用到运行角色；停止并重新进入 PIE 才读取已保存的新规则。");
	Diagnostics = FText::FromString(Report);
}

#undef LOCTEXT_NAMESPACE
