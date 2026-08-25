#include "Tools/EnemyManager/SEnemyManagerWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetThumbnail.h"
#include "AssetToolsModule.h"
#include "Character/EnemyCharacterBase.h"
#include "Data/AbilityData.h"
#include "Data/EnemyWeaponDefinition.h"
#include "Data/GasTemplate.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "IAssetTools.h"
#include "IDetailsView.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "StateTree.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tools/EnemyManager/EnemyDataActorFactory.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "EnemyManager"

const TCHAR* SEnemyManagerWidget::FormalEnemyRoot = TEXT("/Game/Code/Enemy/Definitions");
const TCHAR* SEnemyManagerWidget::TestEnemyRoot = TEXT("/Game/Test/Enemy");
const TCHAR* SEnemyManagerWidget::LegacyEnemyRoot = TEXT("/Game/Docs/Data/Enemy");

const TArray<SEnemyManagerWidget::FEnemyCatalogDefinition>& SEnemyManagerWidget::GetCatalog()
{
	static const TArray<FEnemyCatalogDefinition> Catalog = {
		{TEXT("Shadow"), LOCTEXT("EnemyShadow", "暗影"), EEnemyCombatTier::Normal, false, true,
			{TEXT("Shadow"), TEXT("DA_Shadow")}},
		{TEXT("CorruptedRat"), LOCTEXT("EnemyCorruptedRat", "腐鼠"), EEnemyCombatTier::Normal, false, true,
			{TEXT("CorruptedRat"), TEXT("Rat"), TEXT("DA_Rat")}},
		{TEXT("RatNest"), LOCTEXT("EnemyRatNest", "鼠巢"), EEnemyCombatTier::Normal, false, true,
			{TEXT("RatNest")}},
		{TEXT("RottenGuard"), LOCTEXT("EnemyRottenGuard", "腐化看守"), EEnemyCombatTier::Normal, false, true,
			{TEXT("RottenGuard"), TEXT("CorruptedGuardA"), TEXT("DA_RottenGuard")}},
		{TEXT("AlarmBellJailer"), LOCTEXT("EnemyAlarmBellJailer", "哨铃狱卒"), EEnemyCombatTier::Normal, false, true,
			{TEXT("AlarmBellJailer")}},
		{TEXT("RatPunishedPrisoner"), LOCTEXT("EnemyRatPunishedPrisoner", "鼠刑囚犯"), EEnemyCombatTier::Normal, false, true,
			{TEXT("RatPunishedPrisoner")}},
		{TEXT("SpineMusketeer"), LOCTEXT("EnemySpineMusketeer", "脊骨猎枪看守"), EEnemyCombatTier::Normal, false, true,
			{TEXT("SpineMusketeer")}},
		{TEXT("TyrantPrisoner"), LOCTEXT("EnemyTyrantPrisoner", "霸道囚犯"), EEnemyCombatTier::Normal, false, true,
			{TEXT("TyrantPrisoner")}},
		{TEXT("IronCagePrisoner"), LOCTEXT("EnemyIronCagePrisoner", "铁笼囚徒"), EEnemyCombatTier::Elite, false, true,
			{TEXT("IronCagePrisoner")}},
		{TEXT("GuardCaptain"), LOCTEXT("EnemyGuardCaptain", "看守队长"), EEnemyCombatTier::Elite, false, true,
			{TEXT("GuardCaptain")}},
		{TEXT("WitheredBloomAlchemist"), LOCTEXT("EnemyWitheredBloomAlchemist", "枯荣炼金士"), EEnemyCombatTier::Elite, false, true,
			{TEXT("WitheredBloomAlchemist")}},
		{TEXT("CorruptedOccultist"), LOCTEXT("EnemyCorruptedOccultist", "腐化神秘学家"), EEnemyCombatTier::Elite, false, true,
			{TEXT("CorruptedOccultist")}},
		{TEXT("InquisitorDalsoHermann"), LOCTEXT("EnemyInquisitorDalsoHermann", "审判官 达尔索·赫尔曼"), EEnemyCombatTier::Boss, false, true,
			{TEXT("InquisitorDalsoHermann"), TEXT("DalsoHermann")}},
		{TEXT("Dummy"), LOCTEXT("EnemyDummy", "程序测试 Dummy"), EEnemyCombatTier::Normal, true, false,
			{TEXT("Dummy"), TEXT("DA_Dummy")}},
		{TEXT("TrainingDummy"), LOCTEXT("EnemyTrainingDummy", "训练木头人"), EEnemyCombatTier::Normal, true, false,
			{TEXT("TrainingDummy"), TEXT("WoodenDummy"), TEXT("TargetDummy")}},
	};
	return Catalog;
}

bool SEnemyManagerWidget::IsUnderRoot(const FString& PackageName, const FString& Root)
{
	return PackageName.Equals(Root, ESearchCase::IgnoreCase)
		|| PackageName.StartsWith(Root + TEXT("/"), ESearchCase::IgnoreCase);
}

bool SEnemyManagerWidget::IsOfficialEnemyPackage(const FString& PackageName)
{
	return IsUnderRoot(PackageName, FormalEnemyRoot);
}

bool SEnemyManagerWidget::IsTestEnemyPackage(const FString& PackageName)
{
	return IsUnderRoot(PackageName, TestEnemyRoot);
}

FString SEnemyManagerWidget::NormalizeEnemyIdentity(const FString& Value)
{
	FString Result;
	Result.Reserve(Value.Len());
	for (const TCHAR Character : Value)
	{
		if (FChar::IsAlnum(Character))
		{
			Result.AppendChar(FChar::ToLower(Character));
		}
	}
	Result.RemoveFromStart(TEXT("da"));
	Result.RemoveFromStart(TEXT("enemy"));
	Result.RemoveFromStart(TEXT("en"));
	return Result;
}

FText SEnemyManagerWidget::GetTierText(const EEnemyCombatTier Tier)
{
	switch (Tier)
	{
	case EEnemyCombatTier::Elite:
		return LOCTEXT("TierElite", "精英");
	case EEnemyCombatTier::Boss:
		return LOCTEXT("TierBoss", "Boss");
	default:
		return LOCTEXT("TierNormal", "普通");
	}
}

FString SEnemyManagerWidget::GetTierFolder(const EEnemyCombatTier Tier)
{
	switch (Tier)
	{
	case EEnemyCombatTier::Elite:
		return TEXT("Elite");
	case EEnemyCombatTier::Boss:
		return TEXT("Boss");
	default:
		return TEXT("Normal");
	}
}

FText SEnemyManagerWidget::GetLibraryCategoryText(const EEnemyLibraryCategory Category)
{
	switch (Category)
	{
	case EEnemyLibraryCategory::Official:
		return LOCTEXT("CategoryOfficial", "正式资产");
	case EEnemyLibraryCategory::Placeholder:
		return LOCTEXT("CategoryPlaceholder", "占位计划");
	case EEnemyLibraryCategory::Legacy:
		return LOCTEXT("CategoryLegacy", "待迁移");
	case EEnemyLibraryCategory::Test:
		return LOCTEXT("CategoryTest", "测试库");
	default:
		return FText::GetEmpty();
	}
}

namespace
{
	TSharedRef<SWidget> BuildEnemyPageHeader(const FText& Title, const FText& Description)
	{
		return SNew(SBorder)
			.Padding(FMargin(10.f, 8.f))
			.BorderBackgroundColor(FLinearColor(0.07f, 0.07f, 0.07f, 1.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(Title)
						.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(Description)
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
	}
}

class SEnemyManagerDragCard final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnemyManagerDragCard) {}
		SLATE_ARGUMENT(FEnemyManagerItemPtr, Item)
		SLATE_ARGUMENT(TWeakPtr<SEnemyManagerWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;

		ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox)
					.WidthOverride(48.f)
					.HeightOverride(48.f)
				[
					Item.IsValid() && Item->Thumbnail.IsValid()
						? Item->Thumbnail->MakeThumbnailWidget()
						: SNullWidget::NullWidget
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(Item.IsValid() ? Item->DisplayName : FText::GetEmpty())
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(Item.IsValid()
							? FText::Format(
								LOCTEXT("EnemyDragRowMeta", "[{0}] {1} · {2}"),
								SEnemyManagerWidget::GetTierText(Item->CombatTier),
								FText::FromString(Item->EnemyId),
								SEnemyManagerWidget::GetLibraryCategoryText(Item->LibraryCategory))
							: FText::GetEmpty())
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(Item.IsValid() ? Item->ValidationText : FText::GetEmpty())
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
						.ColorAndOpacity(Item.IsValid()
							? Item->ValidationColor
							: FSlateColor::UseForeground())
						.AutoWrapText(true)
				]
			]
		];
	}

	virtual FReply OnMouseButtonDown(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Item.IsValid())
		{
			if (const TSharedPtr<SEnemyManagerWidget> Owner = OwnerWidget.Pin())
			{
				Owner->SelectEnemyForInteraction(Item);
			}
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override
	{
		if (Item.IsValid())
		{
			if (const TSharedPtr<SEnemyManagerWidget> Owner = OwnerWidget.Pin())
			{
				return Owner->BeginEnemyDrag(Item);
			}
		}
		return FReply::Unhandled();
	}

private:
	FEnemyManagerItemPtr Item;
	TWeakPtr<SEnemyManagerWidget> OwnerWidget;
};

void SEnemyManagerWidget::Construct(const FArguments& InArgs)
{
	ThumbnailPool = MakeShared<FAssetThumbnailPool>(128);
	StatusText = LOCTEXT("InitialStatus", "敌人编辑器已就绪。");
	EnabledLibraryFilters.Add(EEnemyLibraryCategory::Official);
	EnabledLibraryFilters.Add(EEnemyLibraryCategory::Placeholder);
	EnabledLibraryFilters.Add(EEnemyLibraryCategory::Legacy);

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsArgs.NotifyHook = this;

	FPropertyEditorModule& PropertyEditor =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
	DetailsView->SetIsPropertyVisibleDelegate(
		FIsPropertyVisible::CreateSP(this, &SEnemyManagerWidget::IsPropertyVisible));

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.f)
			+ SSplitter::Slot().Value(0.23f).MinSize(235.f)
			[
				BuildLeftPanel()
			]
			+ SSplitter::Slot().Value(0.47f).MinSize(430.f)
			[
				SAssignNew(CenterPanelBox, SBox)
				[
					BuildCenterPanel()
				]
			]
			+ SSplitter::Slot().Value(0.30f).MinSize(315.f)
			[
				SAssignNew(RightPanelHost, SBox)
				[
					BuildRightPanel()
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.Padding(FMargin(10.f, 6.f))
			[
				SNew(STextBlock)
				.Text(this, &SEnemyManagerWidget::GetStatusText)
				.ColorAndOpacity(this, &SEnemyManagerWidget::GetStatusColor)
			]
		]
	];

	if (!InArgs._SkipInitialAssetScan)
	{
		RefreshEntries(false);
	}
	RebuildCenterPage();
	RefreshInspector();
	RefreshRightPanel();
}

#if WITH_DEV_AUTOMATION_TESTS
bool SEnemyManagerWidget::HasCorePanelsForTesting() const
{
	return EnemyListView.IsValid()
		&& CenterPanelBox.IsValid()
		&& PageControlsHost.IsValid()
		&& InspectorHost.IsValid()
		&& RightPanelHost.IsValid()
		&& DetailsView.IsValid();
}

int32 SEnemyManagerWidget::GetRequiredFormalPlaceholderCountForTesting()
{
	int32 Count = 0;
	for (const FEnemyCatalogDefinition& Definition : GetCatalog())
	{
		Count += Definition.bRequired && !Definition.bTestOnly ? 1 : 0;
	}
	return Count;
}

int32 SEnemyManagerWidget::GetOptionalTestPlaceholderCountForTesting()
{
	int32 Count = 0;
	for (const FEnemyCatalogDefinition& Definition : GetCatalog())
	{
		Count += Definition.bTestOnly && !Definition.bRequired ? 1 : 0;
	}
	return Count;
}

TArray<FString> SEnemyManagerWidget::GetRequiredFormalEnemyIdsForTesting()
{
	TArray<FString> Result;
	for (const FEnemyCatalogDefinition& Definition : GetCatalog())
	{
		if (Definition.bRequired && !Definition.bTestOnly)
		{
			Result.Add(Definition.EnemyId);
		}
	}
	return Result;
}

TArray<FString> SEnemyManagerWidget::GetOptionalTestEnemyIdsForTesting()
{
	TArray<FString> Result;
	for (const FEnemyCatalogDefinition& Definition : GetCatalog())
	{
		if (!Definition.bRequired && Definition.bTestOnly)
		{
			Result.Add(Definition.EnemyId);
		}
	}
	return Result;
}

bool SEnemyManagerWidget::IsOfficialEnemyPathForTesting(const FString& PackageName)
{
	return IsOfficialEnemyPackage(PackageName);
}

bool SEnemyManagerWidget::IsTestEnemyPathForTesting(const FString& PackageName)
{
	return IsTestEnemyPackage(PackageName);
}
#endif

TSharedRef<SWidget> SEnemyManagerWidget::BuildToolbar()
{
	return SNew(SBorder)
		.Padding(FMargin(10.f, 8.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "刷新"))
				.OnClicked_Lambda([this]()
				{
					RefreshEntries(true);
					RebuildCenterPage();
					RefreshInspector();
					RefreshRightPanel();
					SetStatus(LOCTEXT("Refreshed", "已刷新正式、占位、待迁移和测试敌人库。"));
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreatePlaceholder", "创建选中占位资产"))
				.ToolTipText(LOCTEXT("CreatePlaceholderTip", "在隔离后的正式或测试目录中创建 UEnemyData 占位资产。"))
				.IsEnabled_Lambda([this]() { return SelectedItem.IsValid() && SelectedItem->bPlaceholder; })
				.OnClicked(this, &SEnemyManagerWidget::CreateSelectedPlaceholder)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("LocateAsset", "定位（Ctrl+B）"))
				.IsEnabled_Lambda([this]() { return SelectedItem.IsValid() && SelectedItem->AssetData.IsValid(); })
				.OnClicked(this, &SEnemyManagerWidget::SyncSelectedEnemy)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenAsset", "打开资产"))
				.IsEnabled_Lambda([this]() { return SelectedItem.IsValid() && SelectedItem->EnemyData.IsValid(); })
				.OnClicked(this, &SEnemyManagerWidget::OpenSelectedEnemy)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveAsset", "签出并保存"))
				.IsEnabled_Lambda([this]() { return SelectedItem.IsValid() && SelectedItem->EnemyData.IsValid(); })
				.OnClicked(this, &SEnemyManagerWidget::SaveSelectedEnemy)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(14.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EditorTitle", "敌人配置编辑器"))
				.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
				.Justification(ETextJustify::Right)
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildLeftPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EnemyLibrary", "敌人库（可拖入关卡）"))
				.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(FText::Format(
					LOCTEXT("PathRule", "正式：{0}\n测试：{1}\n旧 {2} 目录仅作为待迁移来源。"),
					FText::FromString(FormalEnemyRoot),
					FText::FromString(TestEnemyRoot),
					FText::FromString(LegacyEnemyRoot)))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(LOCTEXT("Search", "搜索名称、ID、路径、级别或状态…"))
				.OnTextChanged(this, &SEnemyManagerWidget::HandleSearchChanged)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildLibraryFilters()
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(EnemyListView, SListView<FEnemyManagerItemPtr>)
				.ListItemsSource(&FilteredEntries)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SEnemyManagerWidget::GenerateEnemyRow)
				.OnSelectionChanged(this, &SEnemyManagerWidget::HandleSelectionChanged)
				.OnKeyDownHandler(this, &SEnemyManagerWidget::HandleEnemyListKeyDown)
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildCenterPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildPageTabs()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 8.f)
			[
				SAssignNew(PageControlsHost, SBox)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(InspectorHost, SBox)
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildPageTabs()
{
	auto MakeTab = [this](const EEnemyManagerPage Page, const FText& Label)
	{
		return SNew(SButton)
			.Text(Label)
			.ButtonColorAndOpacity(this, &SEnemyManagerWidget::GetPageButtonColor, Page)
			.OnClicked(this, &SEnemyManagerWidget::SetPage, Page);
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeTab(EEnemyManagerPage::Overview, LOCTEXT("OverviewTab", "身份与概览"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeTab(EEnemyManagerPage::CharacterData, LOCTEXT("CharacterDataTab", "角色基础数据"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				MakeTab(EEnemyManagerPage::AbilityData, LOCTEXT("AbilityDataTab", "Ability 数据"))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeTab(EEnemyManagerPage::Blueprint, LOCTEXT("BlueprintTab", "敌人 BP"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeTab(EEnemyManagerPage::AIAndCombat, LOCTEXT("AICombatTab", "AI 与战斗"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				MakeTab(EEnemyManagerPage::SpawnAndRewards, LOCTEXT("SpawnRewardsTab", "生成与奖励"))
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildPageControls()
{
	if (!SelectedItem.IsValid())
	{
		return BuildEnemyPageHeader(
			LOCTEXT("SelectEnemyPageTitle", "请选择敌人"),
			LOCTEXT("SelectEnemyPageDescription", "从左侧敌人库选择一个条目；配置完整的正式资产可以直接拖入关卡视口。"));
	}
	if (SelectedItem->bPlaceholder)
	{
		return BuildEnemyPageHeader(
			LOCTEXT("PlaceholderPageTitle", "占位条目"),
			LOCTEXT("PlaceholderPageDescription", "先创建 EnemyData；占位条目本身不能配置或拖入场景。"));
	}

	switch (CurrentPage)
	{
	case EEnemyManagerPage::Overview:
		return BuildEnemyPageHeader(
			LOCTEXT("OverviewPageTitle", "身份、分类与生产状态"),
			LOCTEXT("OverviewPageDescription", "直接编辑敌人 ID、中文/英文名、级别、阵营、区域、生产状态和难度分值。"));
	case EEnemyManagerPage::CharacterData:
		return BuildEnemyPageHeader(
			LOCTEXT("CharacterDataPageTitle", "角色基础数据"),
			LOCTEXT("CharacterDataPageDescription", "直接选择移动数据行、基础属性数据行和默认动画层；无需离开敌人编辑器。"));
	case EEnemyManagerPage::AbilityData:
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildEnemyPageHeader(
					LOCTEXT("AbilityPageTitle", "Ability 与 GAS 配置"),
					LOCTEXT("AbilityPageDescription", "指定 AbilityData / GASTemplate 后，可在本页面下方直接编辑对应 DA。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("AbilityDataPickerLabel", "AbilityData"))
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(UAbilityData::StaticClass())
						.ObjectPath(this, &SEnemyManagerWidget::GetAbilityDataObjectPath)
						.OnObjectChanged(this, &SEnemyManagerWidget::HandleAbilityDataChanged)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("GasTemplatePickerLabel", "GASTemplate"))
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(UGASTemplate::StaticClass())
						.ObjectPath(this, &SEnemyManagerWidget::GetGasTemplateObjectPath)
						.OnObjectChanged(this, &SEnemyManagerWidget::HandleGasTemplateChanged)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
						.Text(LOCTEXT("InspectAbilityData", "编辑 AbilityData"))
						.ButtonColorAndOpacity_Lambda([this]()
						{
							return CurrentAbilityInspector == EEnemyAbilityInspector::AbilityData
								? FSlateColor(FLinearColor(0.08f, 0.38f, 0.62f, 1.f))
								: FSlateColor(FLinearColor(0.16f, 0.16f, 0.16f, 1.f));
						})
						.OnClicked(this, &SEnemyManagerWidget::SetAbilityInspector, EEnemyAbilityInspector::AbilityData)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SButton)
						.Text(LOCTEXT("InspectGasTemplate", "编辑 GASTemplate"))
						.ButtonColorAndOpacity_Lambda([this]()
						{
							return CurrentAbilityInspector == EEnemyAbilityInspector::GasTemplate
								? FSlateColor(FLinearColor(0.08f, 0.38f, 0.62f, 1.f))
								: FSlateColor(FLinearColor(0.16f, 0.16f, 0.16f, 1.f));
						})
						.OnClicked(this, &SEnemyManagerWidget::SetAbilityInspector, EEnemyAbilityInspector::GasTemplate)
				]
			];
	case EEnemyManagerPage::Blueprint:
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildEnemyPageHeader(
					LOCTEXT("BlueprintPageTitle", "敌人 Blueprint 默认配置"),
					LOCTEXT(
						"BlueprintPageDescription",
						"选择 EnemyClass 后，下方直接显示该 BP 的 Class Default Object。模型、动画、组件引用和默认参数可在这里编辑，不必另开 BP。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("EnemyClassPickerLabel", "EnemyClass"))
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SClassPropertyEntryBox)
						.MetaClass(AEnemyCharacterBase::StaticClass())
						.AllowAbstract(false)
						.AllowNone(true)
						.ShowDisplayNames(true)
						.SelectedClass(this, &SEnemyManagerWidget::GetSelectedEnemyClass)
						.OnSetClass(this, &SEnemyManagerWidget::HandleEnemyClassChanged)
				]
			];
	case EEnemyManagerPage::AIAndCombat:
		return BuildEnemyPageHeader(
			LOCTEXT("AICombatPageTitle", "AI、武器、Buff 与韧性"),
			LOCTEXT("AICombatPageDescription", "集中编辑行为树、移动/感知/攻击调参、敌人武器、专属 Buff 和霸体参数。"));
	case EEnemyManagerPage::SpawnAndRewards:
		return BuildEnemyPageHeader(
			LOCTEXT("SpawnRewardsPageTitle", "生成表现与敌人击杀奖励"),
			LOCTEXT(
				"SpawnRewardsPageDescription",
				"敌人只管理自己的击杀掉落；RoomData 的关卡掉落保持独立。DropExp 是未接入结算的遗留 GAS 属性，本页不使用它。"));
	default:
		return BuildEnemyPageHeader(FText::GetEmpty(), FText::GetEmpty());
	}
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildRightPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
					.Text(this, &SEnemyManagerWidget::GetSelectedTitle)
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
					.Text(this, &SEnemyManagerWidget::GetSelectedSubtitle)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					BuildValidationPanel()
				]
				+ SScrollBox::Slot()
				.Padding(0.f, 8.f, 0.f, 0.f)
				[
					BuildDependencyPanel()
				]
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildValidationMessage(
	const FText& Message,
	const FLinearColor& Color,
	const bool bStrong) const
{
	const FLinearColor Background(
		Color.R * 0.22f,
		Color.G * 0.22f,
		Color.B * 0.22f,
		1.f);
	return SNew(SBorder)
		.Padding(FMargin(9.f, 7.f))
		.BorderBackgroundColor(Background)
		[
			SNew(STextBlock)
				.Text(Message)
				.Font(FAppStyle::GetFontStyle(bStrong ? TEXT("HeadingExtraSmall") : TEXT("NormalFontBold")))
				.ColorAndOpacity(FSlateColor(Color))
				.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildValidationPanel()
{
	const FLinearColor ErrorColor(1.f, 0.24f, 0.18f, 1.f);
	const FLinearColor WarningColor(1.f, 0.62f, 0.12f, 1.f);
	const FLinearColor RequirementColor(0.26f, 0.66f, 1.f, 1.f);
	const FLinearColor ReadyColor(0.24f, 0.9f, 0.42f, 1.f);

	TSharedRef<SVerticalBox> Messages = SNew(SVerticalBox);
	auto AddMessage = [&Messages, this](const FText& Text, const FLinearColor& Color, const bool bStrong)
	{
		Messages->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
		[
			BuildValidationMessage(Text, Color, bStrong)
		];
	};

	if (!SelectedItem.IsValid())
	{
		AddMessage(LOCTEXT("ValidationSelectEnemy", "需求：从左侧选择一个敌人。"), RequirementColor, true);
	}
	else if (SelectedItem->bPlaceholder)
	{
		AddMessage(
			SelectedItem->bRequired
				? LOCTEXT("ValidationRequiredPlaceholder", "需求：这是正式敌人占位，必须先创建 EnemyData 才能进入配置与发布流程。")
				: LOCTEXT("ValidationOptionalPlaceholder", "提示：这是可选测试占位，不参与正式发布门禁。"),
			SelectedItem->bRequired ? WarningColor : RequirementColor,
			true);
	}
	else
	{
		const UEnemyData* Enemy = SelectedItem->EnemyData.Get();
		if (!Enemy)
		{
			AddMessage(LOCTEXT("ValidationEnemyLoadFailed", "错误：敌人资产加载失败，请刷新敌人库。"), ErrorColor, true);
		}
		else
		{
			const FString PackageName = SelectedItem->AssetData.PackageName.ToString();
			if (SelectedItem->LibraryCategory == EEnemyLibraryCategory::Legacy)
			{
				AddMessage(
					LOCTEXT("ValidationLegacyPath", "警告：此资产仍在旧目录，只作为待迁移来源；正式发布前必须迁入正式敌人目录。"),
					WarningColor,
					true);
			}
			if (IsOfficialEnemyPackage(PackageName) && Enemy->bTestOnly)
			{
				AddMessage(LOCTEXT("ValidationOfficialMarkedTest", "错误：正式目录中的敌人不能勾选“仅测试”。"), ErrorColor, true);
			}
			if (IsTestEnemyPackage(PackageName) && !Enemy->bTestOnly)
			{
				AddMessage(LOCTEXT("ValidationTestNotMarked", "警告：测试目录资产应勾选“仅测试”，避免误进入正式发布。"), WarningColor, false);
			}
			if (Enemy->EnemyId.IsNone())
			{
				AddMessage(LOCTEXT("ValidationMissingEnemyId", "需求：配置稳定的 EnemyId；不要用显示名代替存档和统计身份。"), RequirementColor, false);
			}
			if (Enemy->DisplayName.IsEmpty())
			{
				AddMessage(LOCTEXT("ValidationMissingDisplayName", "需求：配置面向策划和美术的中文显示名。"), RequirementColor, false);
			}
			if (!Enemy->EnemyClass)
			{
				AddMessage(
					LOCTEXT("ValidationMissingEnemyClass", "错误：未配置 EnemyClass。没有敌人 BP 时不能拖入关卡，也不能运行时生成。"),
					ErrorColor,
					true);
			}
			if (!Enemy->GasTemplate && !Enemy->bTestOnly)
			{
				AddMessage(LOCTEXT("ValidationMissingGas", "错误：正式敌人必须配置 GASTemplate。"), ErrorColor, true);
			}
			if (!Enemy->AbilityData && !Enemy->bTestOnly)
			{
				AddMessage(LOCTEXT("ValidationMissingAbility", "警告：未配置 AbilityData，敌人攻击蒙太奇与动作映射可能缺失。"), WarningColor, false);
			}
			if (!Enemy->StateTree && !Enemy->bTestOnly)
			{
				AddMessage(LOCTEXT("ValidationMissingStateTree", "警告：未配置 StateTree，敌人可能不会执行战斗逻辑。"), WarningColor, false);
			}
			if (!Enemy->DefaultWeaponDefinition && !Enemy->bTestOnly)
			{
				AddMessage(LOCTEXT("ValidationMissingWeapon", "警告：未配置默认敌人武器；确认该敌人是否确实徒手。"), WarningColor, false);
			}
			if (Enemy->bEnableKillRewards && Enemy->KillRewards.IsEmpty())
			{
				AddMessage(LOCTEXT("ValidationEmptyRewards", "错误：已启用敌人击杀奖励，但奖励列表为空。"), ErrorColor, true);
			}

			int32 Errors = 0;
			int32 Warnings = 0;
			CountValidationIssues(SelectedItem, Errors, Warnings);
			if (Errors == 0 && Warnings == 0)
			{
				AddMessage(LOCTEXT("ValidationAllReady", "配置完整：当前敌人没有发现错误或警告。"), ReadyColor, true);
			}
		}
	}

	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 7.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ValidationPanelTitle", "配置需求与警告"))
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Messages
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildLibraryFilters()
{
	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(4.f, 4.f));
	WrapBox->AddSlot()[BuildLibraryFilterButton(EEnemyLibraryCategory::Official, LOCTEXT("FilterOfficial", "正式"))];
	WrapBox->AddSlot()[BuildLibraryFilterButton(EEnemyLibraryCategory::Placeholder, LOCTEXT("FilterPlaceholder", "占位"))];
	WrapBox->AddSlot()[BuildLibraryFilterButton(EEnemyLibraryCategory::Legacy, LOCTEXT("FilterLegacy", "待迁移"))];
	WrapBox->AddSlot()[BuildLibraryFilterButton(EEnemyLibraryCategory::Test, LOCTEXT("FilterTest", "测试"))];
	return WrapBox;
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildDependencyPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(LOCTEXT("DependencyTitle", "关联配置状态"))
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 6.f)
			[
				SNew(STextBlock)
					.Text(this, &SEnemyManagerWidget::GetDependencySummary)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT(
						"DependencyInlineHint",
						"所有主要配置都在中间页面直接编辑。下方按钮只切换页面，不会另开 BP 或 DA 窗口。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.26f, 0.66f, 1.f, 1.f)))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SWrapBox)
				.UseAllottedSize(true)
				.InnerSlotPadding(FVector2D(4.f, 4.f))
				+ SWrapBox::Slot()
				[
					SNew(SButton)
						.Text(LOCTEXT("GoEnemyClass", "敌人 BP"))
						.OnClicked(this, &SEnemyManagerWidget::SetPage, EEnemyManagerPage::Blueprint)
				]
				+ SWrapBox::Slot()
				[
					SNew(SButton)
						.Text(LOCTEXT("GoAbilityData", "Ability 数据"))
						.OnClicked(this, &SEnemyManagerWidget::SetPage, EEnemyManagerPage::AbilityData)
				]
				+ SWrapBox::Slot()
				[
					SNew(SButton)
						.Text(LOCTEXT("GoAICombat", "AI 与战斗"))
						.OnClicked(this, &SEnemyManagerWidget::SetPage, EEnemyManagerPage::AIAndCombat)
				]
				+ SWrapBox::Slot()
				[
					SNew(SButton)
						.Text(LOCTEXT("GoSpawnRewards", "生成与奖励"))
						.OnClicked(this, &SEnemyManagerWidget::SetPage, EEnemyManagerPage::SpawnAndRewards)
				]
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildLibraryFilterButton(
	const EEnemyLibraryCategory Category,
	const FText& Label)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SEnemyManagerWidget::IsLibraryFilterChecked, Category)
		.OnCheckStateChanged(this, &SEnemyManagerWidget::HandleLibraryFilterChanged, Category)
		[
			SNew(STextBlock).Text(Label)
		];
}

TSharedRef<ITableRow> SEnemyManagerWidget::GenerateEnemyRow(
	FEnemyManagerItemPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FEnemyManagerItemPtr>, OwnerTable)
		.Padding(2.f)
		[
			SNew(SBorder)
			.Padding(FMargin(7.f, 6.f))
			[
				SNew(SEnemyManagerDragCard)
					.Item(Item)
					.OwnerWidget(SharedThis(this))
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildPlaceholderInspector(const FEnemyManagerItemPtr& Item)
{
	if (!Item.IsValid())
	{
		return BuildEmptyInspector(LOCTEXT("NoPlaceholder", "占位信息不可用。"));
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
			[
				SNew(STextBlock)
					.Text(FText::Format(
						LOCTEXT(
							"PlaceholderDescription",
							"这是 {0} 的{1}占位条目。\n\n目标 ID：{2}\n目标目录：{3}\n生产状态：占位\n\n"
							"创建后会生成 UEnemyData，并在统一 Details 面板中配置身份、阵营、区域、属性、AI、武器、技能、"
							"受击、生成和敌人击杀奖励。"),
						Item->DisplayName,
						Item->bRequired ? LOCTEXT("RequiredPlaceholder", "正式") : LOCTEXT("OptionalPlaceholder", "可选测试"),
						FText::FromString(Item->EnemyId),
						FText::FromString(BuildPlaceholderPackageName(Item))))
					.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f)
			[
				SNew(SButton)
					.Text(LOCTEXT("CreateFromPlaceholder", "从占位条目创建 EnemyData"))
					.OnClicked(this, &SEnemyManagerWidget::CreateSelectedPlaceholder)
			]
		];
}

TSharedRef<SWidget> SEnemyManagerWidget::BuildEmptyInspector(const FText& Message) const
{
	return SNew(SBorder)
		.Padding(24.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(Message)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
}

bool SEnemyManagerWidget::CatalogMatchesEnemy(
	const FEnemyCatalogDefinition& Definition,
	const UEnemyData* Enemy,
	const FAssetData& AssetData) const
{
	TArray<FString> Candidates = Definition.Aliases;
	Candidates.Add(Definition.EnemyId);

	TArray<FString> ActualValues;
	ActualValues.Add(AssetData.AssetName.ToString());
	if (Enemy)
	{
		ActualValues.Add(Enemy->EnemyId.ToString());
		ActualValues.Add(Enemy->EnglishName);
	}

	for (const FString& Candidate : Candidates)
	{
		const FString NormalizedCandidate = NormalizeEnemyIdentity(Candidate);
		for (const FString& Actual : ActualValues)
		{
			if (!NormalizedCandidate.IsEmpty() && NormalizedCandidate == NormalizeEnemyIdentity(Actual))
			{
				return true;
			}
		}
	}
	return false;
}

void SEnemyManagerWidget::RefreshEntries(const bool bKeepSelection)
{
	const FName PreviousPackage = bKeepSelection && SelectedItem.IsValid()
		? SelectedItem->AssetData.PackageName
		: NAME_None;
	const FString PreviousPlaceholderId = bKeepSelection && SelectedItem.IsValid() && SelectedItem->bPlaceholder
		? SelectedItem->EnemyId
		: FString();

	AllEntries.Reset();
	SelectedItem.Reset();

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous(
		{FormalEnemyRoot, TestEnemyRoot, LegacyEnemyRoot},
		true);

	FARFilter Filter;
	Filter.ClassPaths.Add(UEnemyData::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.LexicalLess(B.PackageName);
	});

	TSet<FString> FulfilledCatalogIds;
	for (const FAssetData& AssetData : Assets)
	{
		UEnemyData* Enemy = Cast<UEnemyData>(AssetData.FastGetAsset(false));

		const FString PackageName = AssetData.PackageName.ToString();
		FEnemyManagerItemPtr Item = MakeShared<FEnemyManagerItem>();
		Item->AssetData = AssetData;
		Item->EnemyData = Enemy;
		Item->Thumbnail = MakeShared<FAssetThumbnail>(AssetData, 48, 48, ThumbnailPool);
		Item->EnemyId = Enemy && !Enemy->EnemyId.IsNone()
			? Enemy->EnemyId.ToString()
			: AssetData.AssetName.ToString();
		Item->EnglishName = Enemy ? Enemy->EnglishName : FString();
		Item->DisplayName = Enemy && !Enemy->DisplayName.IsEmpty()
			? Enemy->DisplayName
			: FText::FromName(AssetData.AssetName);
		Item->CombatTier = Enemy ? Enemy->CombatTier : EEnemyCombatTier::Normal;
		Item->ProductionStatus = Enemy
			? Enemy->ProductionStatus
			: EEnemyProductionStatus::InProduction;
		Item->bTestOnly = Enemy && Enemy->bTestOnly;

		if (IsOfficialEnemyPackage(PackageName))
		{
			Item->LibraryCategory = EEnemyLibraryCategory::Official;
		}
		else if (IsTestEnemyPackage(PackageName))
		{
			Item->LibraryCategory = EEnemyLibraryCategory::Test;
		}
		else
		{
			Item->LibraryCategory = EEnemyLibraryCategory::Legacy;
		}

		for (const FEnemyCatalogDefinition& Definition : GetCatalog())
		{
			if (!CatalogMatchesEnemy(Definition, Enemy, AssetData))
			{
				continue;
			}

			Item->EnemyId = Definition.EnemyId;
			if (!Enemy || Enemy->DisplayName.IsEmpty())
			{
				Item->DisplayName = Definition.DisplayName;
			}
			if (Item->EnglishName.IsEmpty())
			{
				Item->EnglishName = Definition.EnemyId;
			}
			if ((Definition.bTestOnly && Item->LibraryCategory == EEnemyLibraryCategory::Test)
				|| (!Definition.bTestOnly && Item->LibraryCategory == EEnemyLibraryCategory::Official))
			{
				FulfilledCatalogIds.Add(Definition.EnemyId);
			}
			break;
		}

		Item->ValidationText = BuildValidationText(Item);
		Item->ValidationColor = BuildValidationColor(Item);
		Item->SearchText = FString::Printf(
			TEXT("%s %s %s %s %s"),
			*Item->DisplayName.ToString(),
			*Item->EnemyId,
			*Item->EnglishName,
			*PackageName,
			*GetLibraryCategoryText(Item->LibraryCategory).ToString());
		AllEntries.Add(Item);
	}

	for (const FEnemyCatalogDefinition& Definition : GetCatalog())
	{
		if (FulfilledCatalogIds.Contains(Definition.EnemyId))
		{
			continue;
		}

		FEnemyManagerItemPtr Item = MakeShared<FEnemyManagerItem>();
		Item->EnemyId = Definition.EnemyId;
		Item->EnglishName = Definition.EnemyId;
		Item->DisplayName = Definition.DisplayName;
		Item->CombatTier = Definition.CombatTier;
		Item->ProductionStatus = EEnemyProductionStatus::Placeholder;
		Item->LibraryCategory = Definition.bTestOnly
			? EEnemyLibraryCategory::Test
			: EEnemyLibraryCategory::Placeholder;
		Item->bPlaceholder = true;
		Item->bRequired = Definition.bRequired;
		Item->bTestOnly = Definition.bTestOnly;
		Item->ValidationText = BuildValidationText(Item);
		Item->ValidationColor = BuildValidationColor(Item);
		Item->SearchText = FString::Printf(
			TEXT("%s %s %s %s"),
			*Item->DisplayName.ToString(),
			*Item->EnemyId,
			*GetTierText(Item->CombatTier).ToString(),
			*GetLibraryCategoryText(Item->LibraryCategory).ToString());
		AllEntries.Add(Item);
	}

	AllEntries.Sort([](const FEnemyManagerItemPtr& A, const FEnemyManagerItemPtr& B)
	{
		if (!A.IsValid() || !B.IsValid())
		{
			return A.IsValid();
		}
		if (A->LibraryCategory != B->LibraryCategory)
		{
			return static_cast<uint8>(A->LibraryCategory) < static_cast<uint8>(B->LibraryCategory);
		}
		if (A->CombatTier != B->CombatTier)
		{
			return static_cast<uint8>(A->CombatTier) < static_cast<uint8>(B->CombatTier);
		}
		return A->EnemyId < B->EnemyId;
	});

	RebuildFilteredEntries();

	if (bKeepSelection)
	{
		for (const FEnemyManagerItemPtr& Item : AllEntries)
		{
			const bool bSameAsset = !PreviousPackage.IsNone()
				&& Item.IsValid()
				&& Item->AssetData.PackageName == PreviousPackage;
			const bool bSamePlaceholder = !PreviousPlaceholderId.IsEmpty()
				&& Item.IsValid()
				&& Item->bPlaceholder
				&& Item->EnemyId == PreviousPlaceholderId;
			if (bSameAsset || bSamePlaceholder)
			{
				SelectedItem = Item;
				break;
			}
		}
	}

	if (bKeepSelection && !SelectedItem.IsValid() && !FilteredEntries.IsEmpty())
	{
		SelectedItem = FilteredEntries[0];
	}
	if (SelectedItem.IsValid() && EnemyListView.IsValid())
	{
		EnemyListView->SetSelection(SelectedItem);
	}
}

void SEnemyManagerWidget::RebuildFilteredEntries()
{
	FilteredEntries.Reset();
	for (const FEnemyManagerItemPtr& Item : AllEntries)
	{
		if (!Item.IsValid() || !EnabledLibraryFilters.Contains(Item->LibraryCategory))
		{
			continue;
		}
		if (!SearchFilter.IsEmpty()
			&& !Item->SearchText.Contains(SearchFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}
		FilteredEntries.Add(Item);
	}
	if (EnemyListView.IsValid())
	{
		EnemyListView->RequestListRefresh();
	}
}

void SEnemyManagerWidget::RebuildCenterPage()
{
	if (PageControlsHost.IsValid())
	{
		PageControlsHost->SetContent(BuildPageControls());
	}
}

void SEnemyManagerWidget::RefreshRightPanel()
{
	if (RightPanelHost.IsValid())
	{
		RightPanelHost->SetContent(BuildRightPanel());
	}
}

FReply SEnemyManagerWidget::SetPage(const EEnemyManagerPage NewPage)
{
	CurrentPage = NewPage;
	RebuildCenterPage();
	RefreshInspector();
	RefreshRightPanel();
	return FReply::Handled();
}

FReply SEnemyManagerWidget::SetAbilityInspector(const EEnemyAbilityInspector NewInspector)
{
	CurrentAbilityInspector = NewInspector;
	RebuildCenterPage();
	RefreshInspector();
	return FReply::Handled();
}

FSlateColor SEnemyManagerWidget::GetPageButtonColor(const EEnemyManagerPage Page) const
{
	return CurrentPage == Page
		? FSlateColor(FLinearColor(0.08f, 0.38f, 0.62f, 1.f))
		: FSlateColor(FLinearColor(0.16f, 0.16f, 0.16f, 1.f));
}

bool SEnemyManagerWidget::IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	if (CurrentPage == EEnemyManagerPage::AbilityData
		|| CurrentPage == EEnemyManagerPage::Blueprint)
	{
		return true;
	}

	auto MatchesAny = [&PropertyAndParent](const TArray<FName>& VisibleProperties)
	{
		if (VisibleProperties.Contains(PropertyAndParent.Property.GetFName()))
		{
			return true;
		}
		for (const FProperty* Parent : PropertyAndParent.ParentProperties)
		{
			if (Parent && VisibleProperties.Contains(Parent->GetFName()))
			{
				return true;
			}
		}
		return false;
	};

	switch (CurrentPage)
	{
	case EEnemyManagerPage::Overview:
		return MatchesAny({
			GET_MEMBER_NAME_CHECKED(UEnemyData, EnemyId),
			GET_MEMBER_NAME_CHECKED(UEnemyData, DisplayName),
			GET_MEMBER_NAME_CHECKED(UEnemyData, EnglishName),
			GET_MEMBER_NAME_CHECKED(UEnemyData, CombatTier),
			GET_MEMBER_NAME_CHECKED(UEnemyData, ProductionStatus),
			GET_MEMBER_NAME_CHECKED(UEnemyData, FactionTag),
			GET_MEMBER_NAME_CHECKED(UEnemyData, RegionTags),
			GET_MEMBER_NAME_CHECKED(UEnemyData, bTestOnly),
			GET_MEMBER_NAME_CHECKED(UEnemyData, DifficultyScore),
		});
	case EEnemyManagerPage::CharacterData:
		return MatchesAny({
			GET_MEMBER_NAME_CHECKED(UCharacterData, MovementDataRow),
			GET_MEMBER_NAME_CHECKED(UCharacterData, YogBaseAttributeDataRow),
			GET_MEMBER_NAME_CHECKED(UCharacterData, DefaultAnimeLayers),
		});
	case EEnemyManagerPage::AIAndCombat:
		return MatchesAny({
			GET_MEMBER_NAME_CHECKED(UEnemyData, StateTree),
			GET_MEMBER_NAME_CHECKED(UEnemyData, StateTreeBlackboard),
			GET_MEMBER_NAME_CHECKED(UEnemyData, DefaultWeaponDefinition),
			GET_MEMBER_NAME_CHECKED(UEnemyData, AllowedWeaponDefinitions),
			GET_MEMBER_NAME_CHECKED(UEnemyData, EnemyBuffPool),
			GET_MEMBER_NAME_CHECKED(UEnemyData, MovementTuning),
			GET_MEMBER_NAME_CHECKED(UEnemyData, AwarenessTuning),
			GET_MEMBER_NAME_CHECKED(UEnemyData, AttackProfile),
			GET_MEMBER_NAME_CHECKED(UEnemyData, SuperArmorThreshold),
			GET_MEMBER_NAME_CHECKED(UEnemyData, SuperArmorDuration),
			GET_MEMBER_NAME_CHECKED(UEnemyData, RecentlyDamagedStateDuration),
		});
	case EEnemyManagerPage::SpawnAndRewards:
		return MatchesAny({
			GET_MEMBER_NAME_CHECKED(UEnemyData, PreSpawnFX),
			GET_MEMBER_NAME_CHECKED(UEnemyData, PreSpawnFXDuration),
			GET_MEMBER_NAME_CHECKED(UEnemyData, SpawnLifecycleFlow),
			GET_MEMBER_NAME_CHECKED(UEnemyData, bEnableKillRewards),
			GET_MEMBER_NAME_CHECKED(UEnemyData, KillRewards),
		});
	default:
		return true;
	}
}

void SEnemyManagerWidget::RefreshInspector()
{
	if (!InspectorHost.IsValid() || !DetailsView.IsValid())
	{
		return;
	}

	if (!SelectedItem.IsValid())
	{
		DetailsView->SetObject(nullptr);
		InspectorHost->SetContent(BuildEmptyInspector(LOCTEXT("SelectEnemy", "从左侧选择敌人或占位条目。")));
		return;
	}

	if (SelectedItem->bPlaceholder)
	{
		DetailsView->SetObject(nullptr);
		InspectorHost->SetContent(BuildPlaceholderInspector(SelectedItem));
		return;
	}

	UEnemyData* Enemy = SelectedItem->EnemyData.Get();
	if (!Enemy)
	{
		DetailsView->SetObject(nullptr);
		InspectorHost->SetContent(BuildEmptyInspector(LOCTEXT("EnemyLoadFailed", "敌人资产加载失败，请刷新资产注册表。")));
		return;
	}

	UObject* ObjectToInspect = Enemy;
	FText EmptyMessage;
	switch (CurrentPage)
	{
	case EEnemyManagerPage::AbilityData:
		if (CurrentAbilityInspector == EEnemyAbilityInspector::AbilityData)
		{
			ObjectToInspect = Enemy->AbilityData.Get();
			EmptyMessage = LOCTEXT(
				"MissingAbilityInspector",
				"尚未配置 AbilityData。请先在本页上方选择对应 DA。");
		}
		else
		{
			ObjectToInspect = Enemy->GasTemplate.Get();
			EmptyMessage = LOCTEXT(
				"MissingGasInspector",
				"尚未配置 GASTemplate。请先在本页上方选择对应 DA。");
		}
		break;
	case EEnemyManagerPage::Blueprint:
		ObjectToInspect = Enemy->EnemyClass
			? Enemy->EnemyClass->GetDefaultObject()
			: nullptr;
		EmptyMessage = LOCTEXT(
			"MissingBlueprintInspector",
			"尚未配置 EnemyClass。请先在本页上方选择敌人 Blueprint。");
		break;
	default:
		break;
	}

	if (!ObjectToInspect)
	{
		DetailsView->SetObject(nullptr);
		InspectorHost->SetContent(BuildEmptyInspector(EmptyMessage));
		return;
	}

	DetailsView->SetObject(ObjectToInspect);
	DetailsView->ForceRefresh();
	InspectorHost->SetContent(DetailsView.ToSharedRef());
}

void SEnemyManagerWidget::RefreshSelectedValidation()
{
	if (!SelectedItem.IsValid())
	{
		return;
	}
	SelectedItem->ValidationText = BuildValidationText(SelectedItem);
	SelectedItem->ValidationColor = BuildValidationColor(SelectedItem);
	if (EnemyListView.IsValid())
	{
		EnemyListView->RequestListRefresh();
	}
}

void SEnemyManagerWidget::LoadEnemyForItem(const FEnemyManagerItemPtr& Item)
{
	if (!Item.IsValid() || Item->bPlaceholder || Item->EnemyData.IsValid() || !Item->AssetData.IsValid())
	{
		return;
	}

	UEnemyData* Enemy = Cast<UEnemyData>(Item->AssetData.GetAsset());
	if (!Enemy)
	{
		return;
	}

	Item->EnemyData = Enemy;
	Item->EnemyId = Enemy->EnemyId.IsNone() ? Item->AssetData.AssetName.ToString() : Enemy->EnemyId.ToString();
	Item->EnglishName = Enemy->EnglishName;
	Item->DisplayName = Enemy->DisplayName.IsEmpty() ? FText::FromName(Item->AssetData.AssetName) : Enemy->DisplayName;
	Item->CombatTier = Enemy->CombatTier;
	Item->ProductionStatus = Enemy->ProductionStatus;
	Item->bTestOnly = Enemy->bTestOnly;

	for (const FEnemyCatalogDefinition& Definition : GetCatalog())
	{
		if (!CatalogMatchesEnemy(Definition, Enemy, Item->AssetData))
		{
			continue;
		}

		Item->EnemyId = Definition.EnemyId;
		if (Enemy->DisplayName.IsEmpty())
		{
			Item->DisplayName = Definition.DisplayName;
		}
		if (Item->EnglishName.IsEmpty())
		{
			Item->EnglishName = Definition.EnemyId;
		}
		break;
	}

	Item->SearchText = FString::Printf(
		TEXT("%s %s %s %s %s"),
		*Item->DisplayName.ToString(),
		*Item->EnemyId,
		*Item->EnglishName,
		*Item->AssetData.PackageName.ToString(),
		*GetLibraryCategoryText(Item->LibraryCategory).ToString());
}

void SEnemyManagerWidget::HandleSearchChanged(const FText& NewText)
{
	SearchFilter = NewText.ToString();
	RebuildFilteredEntries();
}

void SEnemyManagerWidget::HandleSelectionChanged(
	FEnemyManagerItemPtr Item,
	const ESelectInfo::Type SelectInfo)
{
	SelectedItem = Item;
	LoadEnemyForItem(SelectedItem);
	RebuildCenterPage();
	RefreshInspector();
	RefreshSelectedValidation();
	RefreshRightPanel();
}

void SEnemyManagerWidget::SelectEnemyForInteraction(FEnemyManagerItemPtr Item)
{
	if (!Item.IsValid())
	{
		return;
	}

	if (EnemyListView.IsValid())
	{
		EnemyListView->SetSelection(Item, ESelectInfo::OnMouseClick);
		FSlateApplication::Get().SetKeyboardFocus(
			EnemyListView,
			EFocusCause::Mouse);
	}
	HandleSelectionChanged(Item, ESelectInfo::OnMouseClick);
}

FReply SEnemyManagerWidget::BeginEnemyDrag(FEnemyManagerItemPtr Item)
{
	SelectEnemyForInteraction(Item);
	if (!Item.IsValid() || Item->bPlaceholder)
	{
		SetStatus(LOCTEXT("DragPlaceholderEnemy", "占位敌人尚未生成正式资产，不能拖入关卡。"), true);
		return FReply::Unhandled();
	}

	UEnemyData* Enemy = Item->EnemyData.Get();
	if (!Enemy)
	{
		SetStatus(LOCTEXT("DragInvalidEnemy", "无法将无效的敌人定义拖入关卡。"), true);
		return FReply::Unhandled();
	}
	if (!Enemy->EnemyClass)
	{
		SetStatus(
			LOCTEXT("DragEnemyMissingClass", "拖入关卡前，请先在“敌人 BP”页面配置 EnemyClass。"),
			true);
		return FReply::Unhandled();
	}

	UEnemyDataActorFactory* Factory =
		NewObject<UEnemyDataActorFactory>(GetTransientPackage());
	Factory->Configure(Enemy);
	ActiveDragFactoryRoot = TStrongObjectPtr<UObject>(Factory);
	SetStatus(FText::Format(
		LOCTEXT("DraggingEnemyStatus", "正在拖拽 {0}：在关卡视口中松开鼠标即可放置配置好的敌人 BP。"),
		Item->DisplayName));
	return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(Item->AssetData, Factory));
}

void SEnemyManagerWidget::HandleLibraryFilterChanged(
	const ECheckBoxState NewState,
	const EEnemyLibraryCategory Category)
{
	if (NewState == ECheckBoxState::Checked)
	{
		EnabledLibraryFilters.Add(Category);
	}
	else
	{
		EnabledLibraryFilters.Remove(Category);
	}
	RebuildFilteredEntries();
}

ECheckBoxState SEnemyManagerWidget::IsLibraryFilterChecked(const EEnemyLibraryCategory Category) const
{
	return EnabledLibraryFilters.Contains(Category)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

FReply SEnemyManagerWidget::HandleEnemyListKeyDown(
	const FGeometry& Geometry,
	const FKeyEvent& KeyEvent)
{
	if (KeyEvent.IsControlDown()
		&& !KeyEvent.IsAltDown()
		&& KeyEvent.GetKey() == EKeys::B)
	{
		return SyncSelectedEnemy();
	}
	return FReply::Unhandled();
}

const UClass* SEnemyManagerWidget::GetSelectedEnemyClass() const
{
	const UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	return Enemy && Enemy->EnemyClass ? Enemy->EnemyClass.Get() : nullptr;
}

void SEnemyManagerWidget::HandleEnemyClassChanged(const UClass* NewClass)
{
	UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (!Enemy)
	{
		return;
	}
	if (NewClass && !NewClass->IsChildOf(AEnemyCharacterBase::StaticClass()))
	{
		SetStatus(LOCTEXT("InvalidEnemyClass", "EnemyClass 必须继承 AEnemyCharacterBase。"), true);
		return;
	}

	Enemy->Modify();
	Enemy->EnemyClass = const_cast<UClass*>(NewClass);
	Enemy->MarkPackageDirty();
	RefreshInspector();
	RefreshSelectedValidation();
	RefreshRightPanel();
	SetStatus(NewClass
		? LOCTEXT("EnemyClassAssigned", "已关联敌人 BP；可在下方直接编辑该 BP 的默认配置。")
		: LOCTEXT("EnemyClassCleared", "已清空 EnemyClass；该敌人现在不能拖入关卡。"),
		NewClass == nullptr);
}

FString SEnemyManagerWidget::GetAbilityDataObjectPath() const
{
	const UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	return Enemy && Enemy->AbilityData
		? Enemy->AbilityData->GetPathName()
		: FString();
}

FString SEnemyManagerWidget::GetGasTemplateObjectPath() const
{
	const UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	return Enemy && Enemy->GasTemplate
		? Enemy->GasTemplate->GetPathName()
		: FString();
}

void SEnemyManagerWidget::HandleAbilityDataChanged(const FAssetData& AssetData)
{
	UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (!Enemy)
	{
		return;
	}

	Enemy->Modify();
	Enemy->AbilityData = Cast<UAbilityData>(AssetData.GetAsset());
	Enemy->MarkPackageDirty();
	CurrentAbilityInspector = EEnemyAbilityInspector::AbilityData;
	RebuildCenterPage();
	RefreshInspector();
	RefreshSelectedValidation();
	RefreshRightPanel();
	SetStatus(Enemy->AbilityData
		? LOCTEXT("AbilityAssigned", "已关联 AbilityData；下方正在直接编辑该 DA。")
		: LOCTEXT("AbilityCleared", "已清空 AbilityData。"),
		Enemy->AbilityData == nullptr);
}

void SEnemyManagerWidget::HandleGasTemplateChanged(const FAssetData& AssetData)
{
	UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (!Enemy)
	{
		return;
	}

	Enemy->Modify();
	Enemy->GasTemplate = Cast<UGASTemplate>(AssetData.GetAsset());
	Enemy->MarkPackageDirty();
	CurrentAbilityInspector = EEnemyAbilityInspector::GasTemplate;
	RebuildCenterPage();
	RefreshInspector();
	RefreshSelectedValidation();
	RefreshRightPanel();
	SetStatus(Enemy->GasTemplate
		? LOCTEXT("GasAssigned", "已关联 GASTemplate；下方正在直接编辑该 DA。")
		: LOCTEXT("GasCleared", "已清空 GASTemplate。"),
		Enemy->GasTemplate == nullptr);
}

void SEnemyManagerWidget::NotifyPostChange(
	const FPropertyChangedEvent& PropertyChangedEvent,
	FProperty* PropertyThatChanged)
{
	UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (Enemy)
	{
		Enemy->MarkPackageDirty();
		if (CurrentPage == EEnemyManagerPage::AbilityData)
		{
			UObject* EditedAsset = CurrentAbilityInspector == EEnemyAbilityInspector::AbilityData
				? static_cast<UObject*>(Enemy->AbilityData.Get())
				: static_cast<UObject*>(Enemy->GasTemplate.Get());
			if (EditedAsset)
			{
				EditedAsset->MarkPackageDirty();
			}
		}
		else if (CurrentPage == EEnemyManagerPage::Blueprint && Enemy->EnemyClass)
		{
			if (UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(Enemy->EnemyClass.Get()))
			{
				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			}
		}
	}

	RefreshSelectedValidation();
	RefreshRightPanel();
	SetStatus(LOCTEXT("InlineConfigChanged", "配置已修改；完成后请点击“签出并保存”。"));
}

FString SEnemyManagerWidget::BuildPlaceholderPackageName(const FEnemyManagerItemPtr& Item) const
{
	if (!Item.IsValid())
	{
		return FString();
	}
	const FString Root = Item->bTestOnly ? TestEnemyRoot : FormalEnemyRoot;
	const FString TierFolder = Item->bTestOnly ? TEXT("Utility") : GetTierFolder(Item->CombatTier);
	const FString Prefix = Item->bTestOnly ? TEXT("DA_TEST_EN_") : TEXT("DA_EN_");
	return FString::Printf(TEXT("%s/%s/%s/%s%s"), *Root, *TierFolder, *Item->EnemyId, *Prefix, *Item->EnemyId);
}

FReply SEnemyManagerWidget::CreateSelectedPlaceholder()
{
	if (!SelectedItem.IsValid() || !SelectedItem->bPlaceholder)
	{
		return FReply::Handled();
	}

	const FEnemyManagerItemPtr Placeholder = SelectedItem;
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	const FString RequestedPackageName = BuildPlaceholderPackageName(Placeholder);
	FString PackageName;
	FString AssetName;
	AssetTools.CreateUniqueAssetName(RequestedPackageName, TEXT(""), PackageName, AssetName);

	if (!FPackageName::IsValidLongPackageName(PackageName))
	{
		SetStatus(LOCTEXT("InvalidPlaceholderPath", "占位资产目标目录无效。"), true);
		return FReply::Handled();
	}

	UPackage* Package = CreatePackage(*PackageName);
	UEnemyData* Enemy = Package
		? NewObject<UEnemyData>(
			Package,
			UEnemyData::StaticClass(),
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional)
		: nullptr;
	if (!Enemy)
	{
		SetStatus(LOCTEXT("CreatePlaceholderFailed", "无法创建 EnemyData 占位资产。"), true);
		return FReply::Handled();
	}

	Enemy->EnemyId = FName(*Placeholder->EnemyId);
	Enemy->DisplayName = Placeholder->DisplayName;
	Enemy->EnglishName = Placeholder->EnglishName;
	Enemy->CombatTier = Placeholder->CombatTier;
	Enemy->ProductionStatus = EEnemyProductionStatus::Placeholder;
	Enemy->bTestOnly = Placeholder->bTestOnly;
	Enemy->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Enemy);

	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages({Package}, false);
	if (!bSaved)
	{
		SetStatus(LOCTEXT("CreatePlaceholderUnsaved", "EnemyData 已创建，但保存失败或被取消。"), true);
		return FReply::Handled();
	}

	RefreshEntries(false);
	for (const FEnemyManagerItemPtr& Item : AllEntries)
	{
		if (Item.IsValid() && Item->AssetData.PackageName.ToString() == PackageName)
		{
			SelectedItem = Item;
			break;
		}
	}
	if (SelectedItem.IsValid() && EnemyListView.IsValid())
	{
		EnemyListView->SetSelection(SelectedItem);
	}
	RebuildCenterPage();
	RefreshInspector();
	RefreshRightPanel();
	SetStatus(FText::Format(
		LOCTEXT("PlaceholderCreated", "已创建占位资产：{0}"),
		FText::FromString(PackageName)));
	return FReply::Handled();
}

FReply SEnemyManagerWidget::SaveSelectedEnemy()
{
	UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (!Enemy)
	{
		return FReply::Handled();
	}

	TArray<UPackage*> Packages;
	Packages.AddUnique(Enemy->GetOutermost());
	if (Enemy->AbilityData)
	{
		Packages.AddUnique(Enemy->AbilityData->GetOutermost());
	}
	if (Enemy->GasTemplate)
	{
		Packages.AddUnique(Enemy->GasTemplate->GetOutermost());
	}
	if (Enemy->EnemyClass)
	{
		if (UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(Enemy->EnemyClass.Get()))
		{
			Packages.AddUnique(Blueprint->GetOutermost());
		}
	}
	const bool bSaved = FEditorFileUtils::PromptForCheckoutAndSave(Packages, false, false)
		== FEditorFileUtils::PR_Success;
	SetStatus(
		bSaved ? LOCTEXT("Saved", "已签出并保存 EnemyData 及本页内嵌编辑的关联资产。")
			: LOCTEXT("SaveFailed", "签出/保存失败或被取消。"),
		!bSaved);
	RefreshEntries(true);
	RebuildCenterPage();
	RefreshInspector();
	RefreshRightPanel();
	return FReply::Handled();
}

FReply SEnemyManagerWidget::OpenSelectedEnemy() const
{
	UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (GEditor && Enemy)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Enemy);
		}
	}
	return FReply::Handled();
}

FReply SEnemyManagerWidget::OpenReferencedAsset(UObject* Asset) const
{
	if (GEditor && Asset)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Asset);
		}
	}
	return FReply::Handled();
}

FReply SEnemyManagerWidget::SyncSelectedEnemy()
{
	if (!SelectedItem.IsValid() || !SelectedItem->AssetData.IsValid() || !GEditor)
	{
		SetStatus(LOCTEXT("PlaceholderCannotSync", "占位条目还没有资产，无法在内容浏览器中定位。"), true);
		return FReply::Handled();
	}

	GEditor->SyncBrowserToObjects({SelectedItem->AssetData}, true);
	SetStatus(FText::Format(
		LOCTEXT("Synced", "已在内容浏览器中定位 {0}。"),
		SelectedItem->DisplayName));
	return FReply::Handled();
}

void SEnemyManagerWidget::CountValidationIssues(
	const FEnemyManagerItemPtr& Item,
	int32& OutErrors,
	int32& OutWarnings) const
{
	OutErrors = 0;
	OutWarnings = 0;
	if (!Item.IsValid())
	{
		++OutErrors;
		return;
	}

	if (Item->bPlaceholder)
	{
		if (Item->bRequired)
		{
			++OutWarnings;
		}
		return;
	}

	const UEnemyData* Enemy = Item->EnemyData.Get();
	if (!Enemy)
	{
		++OutErrors;
		return;
	}

	const FString PackageName = Item->AssetData.PackageName.ToString();
	if (Item->LibraryCategory == EEnemyLibraryCategory::Legacy)
	{
		++OutWarnings;
	}
	if (IsOfficialEnemyPackage(PackageName) && Enemy->bTestOnly)
	{
		++OutErrors;
	}
	if (Enemy->bTestOnly && !IsTestEnemyPackage(PackageName))
	{
		++OutWarnings;
	}
	if (IsTestEnemyPackage(PackageName) && !Enemy->bTestOnly)
	{
		++OutWarnings;
	}
	if (Enemy->EnemyId.IsNone())
	{
		++OutWarnings;
	}
	if (Enemy->DisplayName.IsEmpty())
	{
		++OutWarnings;
	}

	const bool bStrictGameplayValidation = !Enemy->bTestOnly;
	if (bStrictGameplayValidation && !Enemy->EnemyClass)
	{
		++OutErrors;
	}
	if (bStrictGameplayValidation && !Enemy->GasTemplate)
	{
		++OutErrors;
	}
	if (bStrictGameplayValidation && !Enemy->AbilityData)
	{
		++OutWarnings;
	}
	if (bStrictGameplayValidation && !Enemy->StateTree)
	{
		++OutWarnings;
	}
	if (bStrictGameplayValidation && !Enemy->DefaultWeaponDefinition)
	{
		++OutWarnings;
	}

	if (Enemy->bEnableKillRewards)
	{
		if (Enemy->KillRewards.IsEmpty())
		{
			++OutErrors;
		}
		for (const FEnemyKillRewardEntry& Entry : Enemy->KillRewards)
		{
			if (Entry.DropChance < 0.f || Entry.DropChance > 1.f)
			{
				++OutErrors;
			}
			else if (Entry.DropChance == 0.f)
			{
				++OutWarnings;
			}
			if (Entry.MinQuantityMultiplier < 1
				|| Entry.MaxQuantityMultiplier < 1
				|| Entry.MinQuantityMultiplier > Entry.MaxQuantityMultiplier)
			{
				++OutErrors;
			}
			if (Entry.Loot.LootType == ELootType::Rune && !Entry.Loot.RuneAsset)
			{
				++OutErrors;
			}
			if (Entry.Loot.LootType != ELootType::Rune && Entry.Loot.Amount <= 0)
			{
				++OutErrors;
			}
			if (Entry.Loot.LootType == ELootType::Material && !Entry.Loot.MetaCurrencyTag.IsValid())
			{
				++OutWarnings;
			}
		}
	}
}

FText SEnemyManagerWidget::BuildValidationText(const FEnemyManagerItemPtr& Item) const
{
	if (Item.IsValid() && Item->bPlaceholder)
	{
		return Item->bRequired
			? LOCTEXT("RequiredPlaceholderValidation", "正式占位：尚未创建资产")
			: LOCTEXT("OptionalPlaceholderValidation", "可选测试占位：不参与正式发布门禁");
	}
	if (Item.IsValid() && Item->AssetData.IsValid() && !Item->EnemyData.IsValid())
	{
		return LOCTEXT("ValidationOnSelection", "选择后加载并校验");
	}

	int32 Errors = 0;
	int32 Warnings = 0;
	CountValidationIssues(Item, Errors, Warnings);
	if (Errors > 0)
	{
		return FText::Format(LOCTEXT("ValidationErrors", "{0} 个错误，{1} 个警告"), Errors, Warnings);
	}
	if (Warnings > 0)
	{
		return FText::Format(LOCTEXT("ValidationWarnings", "{0} 个警告"), Warnings);
	}
	return LOCTEXT("ValidationReady", "配置完整");
}

FSlateColor SEnemyManagerWidget::BuildValidationColor(const FEnemyManagerItemPtr& Item) const
{
	if (Item.IsValid() && Item->AssetData.IsValid() && !Item->EnemyData.IsValid())
	{
		return FSlateColor::UseSubduedForeground();
	}
	int32 Errors = 0;
	int32 Warnings = 0;
	CountValidationIssues(Item, Errors, Warnings);
	if (Errors > 0)
	{
		return FSlateColor(FLinearColor(0.95f, 0.28f, 0.22f));
	}
	if (Warnings > 0 || (Item.IsValid() && Item->bPlaceholder))
	{
		return FSlateColor(FLinearColor(0.85f, 0.52f, 0.2f));
	}
	return FSlateColor(FLinearColor(0.32f, 0.85f, 0.42f));
}

void SEnemyManagerWidget::SetStatus(const FText& InStatus, const bool bError)
{
	StatusText = InStatus;
	bStatusError = bError;
}

FText SEnemyManagerWidget::GetSelectedTitle() const
{
	if (!SelectedItem.IsValid())
	{
		return LOCTEXT("NoSelectionTitle", "未选择敌人");
	}
	return FText::Format(
		LOCTEXT("SelectedTitle", "{0} · {1}"),
		SelectedItem->DisplayName,
		GetLibraryCategoryText(SelectedItem->LibraryCategory));
}

FText SEnemyManagerWidget::GetSelectedSubtitle() const
{
	if (!SelectedItem.IsValid())
	{
		return LOCTEXT("NoSelectionSubtitle", "从左侧敌人库选择条目。");
	}
	if (SelectedItem->bPlaceholder)
	{
		return FText::Format(
			LOCTEXT("PlaceholderSubtitle", "[{0}] {1} → {2}"),
			GetTierText(SelectedItem->CombatTier),
			FText::FromString(SelectedItem->EnemyId),
			FText::FromString(BuildPlaceholderPackageName(SelectedItem)));
	}
	return FText::Format(
		LOCTEXT("AssetSubtitle", "[{0}] {1}\n{2}"),
		GetTierText(SelectedItem->CombatTier),
		FText::FromString(SelectedItem->EnemyId),
		FText::FromName(SelectedItem->AssetData.PackageName));
}

FText SEnemyManagerWidget::GetDependencySummary() const
{
	const UEnemyData* Enemy = SelectedItem.IsValid() ? SelectedItem->EnemyData.Get() : nullptr;
	if (!Enemy)
	{
		return LOCTEXT(
			"PlaceholderDependencySummary",
			"创建 EnemyData 后，这里会集中显示 BP、属性表、AbilityData、GASTemplate、敌人武器、行为树和击杀奖励。");
	}

	const FString EnemyClassName = Enemy->EnemyClass ? Enemy->EnemyClass->GetName() : TEXT("未配置");
	const FString AbilityDataName = Enemy->AbilityData ? Enemy->AbilityData->GetName() : TEXT("未配置");
	const FString GasTemplateName = Enemy->GasTemplate ? Enemy->GasTemplate->GetName() : TEXT("未配置");
	const FString WeaponName = Enemy->DefaultWeaponDefinition ? Enemy->DefaultWeaponDefinition->GetName() : TEXT("未配置");
	const FString StateTreeName = Enemy->StateTree ? Enemy->StateTree->GetName() : TEXT("未配置");
	const FString AttributeRow = Enemy->YogBaseAttributeDataRow.RowName.IsNone()
		? TEXT("未配置")
		: Enemy->YogBaseAttributeDataRow.RowName.ToString();
	const FString MovementRow = Enemy->MovementDataRow.RowName.IsNone()
		? TEXT("未配置")
		: Enemy->MovementDataRow.RowName.ToString();

	return FText::Format(
		LOCTEXT(
			"DependencySummary",
			"BP：{0}　属性行：{1}　移动行：{2}\nAbilityData：{3}　GASTemplate：{4}\n武器：{5}　StateTree：{6}　击杀奖励条目：{7}"),
		FText::FromString(EnemyClassName),
		FText::FromString(AttributeRow),
		FText::FromString(MovementRow),
		FText::FromString(AbilityDataName),
		FText::FromString(GasTemplateName),
		FText::FromString(WeaponName),
		FText::FromString(StateTreeName),
		FText::AsNumber(Enemy->KillRewards.Num()));
}

FText SEnemyManagerWidget::GetSelectedValidationText() const
{
	return SelectedItem.IsValid()
		? SelectedItem->ValidationText
		: LOCTEXT("NoValidation", "未选择需要验证的敌人。");
}

FSlateColor SEnemyManagerWidget::GetSelectedValidationColor() const
{
	return SelectedItem.IsValid() ? SelectedItem->ValidationColor : FSlateColor::UseSubduedForeground();
}

FText SEnemyManagerWidget::GetStatusText() const
{
	return StatusText;
}

FSlateColor SEnemyManagerWidget::GetStatusColor() const
{
	return bStatusError
		? FSlateColor(FLinearColor(0.95f, 0.28f, 0.22f))
		: FSlateColor::UseForeground();
}

#undef LOCTEXT_NAMESPACE
