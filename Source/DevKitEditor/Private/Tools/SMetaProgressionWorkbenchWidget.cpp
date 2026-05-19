#include "Tools/SMetaProgressionWorkbenchWidget.h"

#include "Tools/UMetaNodeEditProxy.h"
#include "MetaProgression/MetaProgressionSettings.h"
#include "DesktopPlatformModule.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "IDetailsView.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PropertyEditorModule.h"
#include "SaveGame/YogSaveGame.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SMetaProgressionWorkbenchWidget"

namespace
{
	TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = {})
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(ToolTip.IsEmpty() ? FText::GetEmpty() : FText::FromString(ToolTip));
	}

	FString EffectTypeLabel(EMetaUpgradeEffectType Type)
	{
		switch (Type)
		{
		case EMetaUpgradeEffectType::StatBoost: return TEXT("Stat");
		case EMetaUpgradeEffectType::FeatureUnlock: return TEXT("Unlock");
		case EMetaUpgradeEffectType::SlotUnlock: return TEXT("Slot");
		default: return TEXT("-");
		}
	}

	FString SideLabel(EMetaSide Side)
	{
		return Side == EMetaSide::Mystic ? TEXT("Mystic") : TEXT("Flesh");
	}

	FString CostSummary(const TArray<FMetaCurrencyCost>& Costs)
	{
		if (Costs.IsEmpty())
		{
			return TEXT("-");
		}

		FString Out;
		for (const FMetaCurrencyCost& Cost : Costs)
		{
			if (!Out.IsEmpty())
			{
				Out += TEXT(", ");
			}
			Out += FString::Printf(TEXT("%s x%d"), *Cost.CurrencyTag.ToString(), Cost.Amount);
		}
		return Out;
	}
}

class SMetaNodeTableRow : public SMultiColumnTableRow<TSharedPtr<FMetaNodeListRow>>
{
public:
	SLATE_BEGIN_ARGS(SMetaNodeTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FMetaNodeListRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FMetaNodeListRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FMetaNodeListRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		const FMetaUpgradeNodeRow& Data = Item->Data;
		if (ColumnName == TEXT("RowName")) return MakeTextCell(Item->RowName.ToString());
		if (ColumnName == TEXT("Display")) return MakeTextCell(Data.DisplayName.ToString());
		if (ColumnName == TEXT("Side")) return MakeTextCell(SideLabel(Data.Side));
		if (ColumnName == TEXT("MaxLevel")) return MakeTextCell(FString::FromInt(Data.MaxLevel));
		if (ColumnName == TEXT("Effect")) return MakeTextCell(EffectTypeLabel(Data.EffectType));
		if (ColumnName == TEXT("Cost")) return MakeTextCell(CostSummary(Data.CostsPerLevel));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FMetaNodeListRow> Item;
};

class SMetaCurrencyTableRow : public SMultiColumnTableRow<TSharedPtr<FMetaCurrencyListRow>>
{
public:
	SLATE_BEGIN_ARGS(SMetaCurrencyTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FMetaCurrencyListRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FMetaCurrencyListRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FMetaCurrencyListRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		const FMetaCurrencyRow& Data = Item->Data;
		if (ColumnName == TEXT("RowName")) return MakeTextCell(Item->RowName.ToString());
		if (ColumnName == TEXT("Tag")) return MakeTextCell(Data.CurrencyTag.ToString());
		if (ColumnName == TEXT("Display")) return MakeTextCell(Data.DisplayName.ToString());
		if (ColumnName == TEXT("Short")) return MakeTextCell(Data.ShortName.ToString());
		if (ColumnName == TEXT("Cap")) return MakeTextCell(FString::FromInt(Data.MaxCapacity));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FMetaCurrencyListRow> Item;
};

class SYogSaveSlotTableRow : public SMultiColumnTableRow<TSharedPtr<FYogSaveSlotListRow>>
{
public:
	SLATE_BEGIN_ARGS(SYogSaveSlotTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FYogSaveSlotListRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FYogSaveSlotListRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FYogSaveSlotListRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Slot")) return MakeTextCell(FString::Printf(TEXT("Slot %d"), Item->SlotIndex + 1));
		if (ColumnName == TEXT("Data")) return MakeTextCell(Item->bHasData ? TEXT("Yes") : TEXT("No"));
		if (ColumnName == TEXT("Run")) return MakeTextCell(Item->bHasPendingRun ? TEXT("Continue") : TEXT("-"));
		if (ColumnName == TEXT("Floor")) return MakeTextCell(FString::FromInt(Item->HighestFloor));
		if (ColumnName == TEXT("Stats"))
		{
			return MakeTextCell(FString::Printf(TEXT("Runs %d / Kills %d / Gold %d"),
				Item->TotalRuns,
				Item->TotalKills,
				Item->TotalGoldEarned));
		}
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FYogSaveSlotListRow> Item;
};

void SMetaProgressionWorkbenchWidget::Construct(const FArguments& InArgs)
{
	EditProxy = TStrongObjectPtr<UMetaNodeEditProxy>(
		NewObject<UMetaNodeEditProxy>(GetTransientPackage(), NAME_None, RF_Transient));
	CurrencyEditProxy = TStrongObjectPtr<UMetaCurrencyEditProxy>(
		NewObject<UMetaCurrencyEditProxy>(GetTransientPackage(), NAME_None, RF_Transient));
	SaveSlotEditProxy = TStrongObjectPtr<UYogSaveSlotEditProxy>(
		NewObject<UYogSaveSlotEditProxy>(GetTransientPackage(), NAME_None, RF_Transient));

	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;
	NodeDetailsView = PropModule.CreateDetailView(DetailsArgs);
	NodeDetailsView->SetObject(nullptr);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveDataTables", "Save DataTables"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSaveDataTablesClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ExportNodeCSV", "Export Nodes CSV"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnExportNodeCsvClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportNodeCSV", "Import Nodes CSV"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnImportNodeCsvClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ExportCurrencyCSV", "Export Currency CSV"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnExportCurrencyCsvClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportCurrencyCSV", "Import Currency CSV"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnImportCurrencyCsvClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Validate", "Validate"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnValidateClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveSlot", "Save Slot"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSaveSelectedSlotClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ExportSlot", "Export Slot"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnExportSelectedSlotClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("DeleteSlot", "Delete Slot"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnDeleteSelectedSlotClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock).Text_Lambda([this]() { return StatusText; })
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.42f)
			[
				SNew(SBorder).Padding(4.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(0.55f).Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SExpandableArea)
						.InitiallyCollapsed(false)
						.HeaderContent()
						[
							SNew(STextBlock).Text(LOCTEXT("NodesHeader", "Meta Upgrade Nodes"))
						]
						.BodyContent()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton)
									.Text(LOCTEXT("AllSide", "All"))
									.ButtonColorAndOpacity(this, &SMetaProgressionWorkbenchWidget::GetSideFilterColor, -1)
									.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSideFilterClicked, -1)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton)
									.Text(LOCTEXT("FleshSide", "Flesh"))
									.ButtonColorAndOpacity(this, &SMetaProgressionWorkbenchWidget::GetSideFilterColor, 0)
									.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSideFilterClicked, 0)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SButton)
									.Text(LOCTEXT("MysticSide", "Mystic"))
									.ButtonColorAndOpacity(this, &SMetaProgressionWorkbenchWidget::GetSideFilterColor, 1)
									.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSideFilterClicked, 1)
								]
							]
							+ SVerticalBox::Slot().FillHeight(1.f)
							[
								SAssignNew(NodeListView, SListView<FNodeRowPtr>)
								.ListItemsSource(&FilteredRows)
								.SelectionMode(ESelectionMode::Single)
								.OnGenerateRow(this, &SMetaProgressionWorkbenchWidget::GenerateNodeRow)
								.OnSelectionChanged(this, &SMetaProgressionWorkbenchWidget::OnNodeSelectionChanged)
								.HeaderRow(
									SNew(SHeaderRow)
									+ SHeaderRow::Column(TEXT("RowName")).DefaultLabel(LOCTEXT("RowNameCol", "Row")).FillWidth(0.23f)
									+ SHeaderRow::Column(TEXT("Display")).DefaultLabel(LOCTEXT("DisplayCol", "Display")).FillWidth(0.24f)
									+ SHeaderRow::Column(TEXT("Side")).DefaultLabel(LOCTEXT("SideCol", "Side")).FixedWidth(54.f)
									+ SHeaderRow::Column(TEXT("MaxLevel")).DefaultLabel(LOCTEXT("MaxLvCol", "Max")).FixedWidth(44.f)
									+ SHeaderRow::Column(TEXT("Effect")).DefaultLabel(LOCTEXT("EffectCol", "Effect")).FixedWidth(58.f)
									+ SHeaderRow::Column(TEXT("Cost")).DefaultLabel(LOCTEXT("CostCol", "Cost")).FillWidth(0.28f)
								)
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(0.24f).Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SExpandableArea)
						.InitiallyCollapsed(false)
						.HeaderContent()
						[
							SNew(STextBlock).Text(LOCTEXT("CurrencyHeader", "Meta Currency Rules"))
						]
						.BodyContent()
						[
							SAssignNew(CurrencyListView, SListView<FCurrencyRowPtr>)
							.ListItemsSource(&CurrencyRows)
							.SelectionMode(ESelectionMode::Single)
							.OnGenerateRow(this, &SMetaProgressionWorkbenchWidget::GenerateCurrencyRow)
							.OnSelectionChanged(this, &SMetaProgressionWorkbenchWidget::OnCurrencySelectionChanged)
							.HeaderRow(
								SNew(SHeaderRow)
								+ SHeaderRow::Column(TEXT("RowName")).DefaultLabel(LOCTEXT("CurrencyRowNameCol", "Row")).FillWidth(0.22f)
								+ SHeaderRow::Column(TEXT("Tag")).DefaultLabel(LOCTEXT("CurrencyTagCol", "Tag")).FillWidth(0.28f)
								+ SHeaderRow::Column(TEXT("Display")).DefaultLabel(LOCTEXT("CurrencyDisplayCol", "Display")).FillWidth(0.24f)
								+ SHeaderRow::Column(TEXT("Short")).DefaultLabel(LOCTEXT("CurrencyShortCol", "Short")).FixedWidth(54.f)
								+ SHeaderRow::Column(TEXT("Cap")).DefaultLabel(LOCTEXT("CurrencyCapCol", "Cap")).FixedWidth(52.f)
							)
						]
					]
					+ SVerticalBox::Slot().FillHeight(0.21f)
					[
						SNew(SExpandableArea)
						.InitiallyCollapsed(false)
						.HeaderContent()
						[
							SNew(STextBlock).Text(LOCTEXT("SaveSlotsHeader", "Save Slots"))
						]
						.BodyContent()
						[
							SAssignNew(SaveSlotListView, SListView<FSaveSlotRowPtr>)
							.ListItemsSource(&SaveSlotRows)
							.SelectionMode(ESelectionMode::Single)
							.OnGenerateRow(this, &SMetaProgressionWorkbenchWidget::GenerateSaveSlotRow)
							.OnSelectionChanged(this, &SMetaProgressionWorkbenchWidget::OnSaveSlotSelectionChanged)
							.HeaderRow(
								SNew(SHeaderRow)
								+ SHeaderRow::Column(TEXT("Slot")).DefaultLabel(LOCTEXT("SlotCol", "Slot")).FixedWidth(58.f)
								+ SHeaderRow::Column(TEXT("Data")).DefaultLabel(LOCTEXT("DataCol", "Data")).FixedWidth(48.f)
								+ SHeaderRow::Column(TEXT("Run")).DefaultLabel(LOCTEXT("RunCol", "Run")).FixedWidth(70.f)
								+ SHeaderRow::Column(TEXT("Floor")).DefaultLabel(LOCTEXT("FloorCol", "Floor")).FixedWidth(52.f)
								+ SHeaderRow::Column(TEXT("Stats")).DefaultLabel(LOCTEXT("StatsCol", "Stats")).FillWidth(1.f)
							)
						]
					]
				]
			]
			+ SSplitter::Slot().Value(0.58f)
			[
				SNew(SBorder).Padding(4.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(this, &SMetaProgressionWorkbenchWidget::GetDetailsTitleText)
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						NodeDetailsView.ToSharedRef()
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 2.f)
		[
			SNew(STextBlock)
			.Text(this, &SMetaProgressionWorkbenchWidget::GetStatsSummaryText)
		]
	];

	RefreshData();
}

SMetaProgressionWorkbenchWidget::~SMetaProgressionWorkbenchWidget()
{
	EditProxy.Reset();
	CurrencyEditProxy.Reset();
	SaveSlotEditProxy.Reset();
}

UDataTable* SMetaProgressionWorkbenchWidget::GetUpgradeTable() const
{
	const UMetaProgressionSettings* Settings = GetDefault<UMetaProgressionSettings>();
	return Settings ? Settings->MetaUpgradeNodeTable.LoadSynchronous() : nullptr;
}

UDataTable* SMetaProgressionWorkbenchWidget::GetCurrencyTable() const
{
	const UMetaProgressionSettings* Settings = GetDefault<UMetaProgressionSettings>();
	return Settings ? Settings->MetaCurrencyRuleTable.LoadSynchronous() : nullptr;
}

void SMetaProgressionWorkbenchWidget::RefreshData(const FText& NewStatus)
{
	AllRows.Reset();
	UDataTable* Table = GetUpgradeTable();
	if (Table)
	{
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			const FMetaUpgradeNodeRow* Row = reinterpret_cast<const FMetaUpgradeNodeRow*>(Pair.Value);
			if (Row)
			{
				AllRows.Add(MakeShared<FMetaNodeListRow>(Pair.Key, *Row));
			}
		}
		AllRows.Sort([](const FNodeRowPtr& A, const FNodeRowPtr& B)
		{
			return A->RowName.LexicalLess(B->RowName);
		});
	}

	RefreshCurrencyData();
	RefreshSaveSlotData();
	RebuildFilteredRows();

	StatusText = !NewStatus.IsEmpty()
		? NewStatus
		: FText::Format(LOCTEXT("LoadedStatus", "Loaded {0} nodes, {1} currency rows, {2} save slots."),
			FText::AsNumber(AllRows.Num()),
			FText::AsNumber(CurrencyRows.Num()),
			FText::AsNumber(SaveSlotRows.Num()));
}

void SMetaProgressionWorkbenchWidget::RebuildFilteredRows()
{
	FilteredRows.Reset();
	for (const FNodeRowPtr& Row : AllRows)
	{
		if (!Row.IsValid())
		{
			continue;
		}

		const bool bPass =
			SideFilter == -1 ||
			(SideFilter == 0 && Row->Data.Side == EMetaSide::Flesh) ||
			(SideFilter == 1 && Row->Data.Side == EMetaSide::Mystic);
		if (bPass)
		{
			FilteredRows.Add(Row);
		}
	}
	if (NodeListView)
	{
		NodeListView->RequestListRefresh();
	}
}

void SMetaProgressionWorkbenchWidget::RefreshCurrencyData()
{
	CurrencyRows.Reset();
	UDataTable* Table = GetCurrencyTable();
	if (Table)
	{
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			const FMetaCurrencyRow* Row = reinterpret_cast<const FMetaCurrencyRow*>(Pair.Value);
			if (Row)
			{
				CurrencyRows.Add(MakeShared<FMetaCurrencyListRow>(Pair.Key, *Row));
			}
		}
		CurrencyRows.Sort([](const FCurrencyRowPtr& A, const FCurrencyRowPtr& B)
		{
			return A->RowName.LexicalLess(B->RowName);
		});
	}

	if (CurrencyListView)
	{
		CurrencyListView->RequestListRefresh();
	}
}

void SMetaProgressionWorkbenchWidget::RefreshSaveSlotData()
{
	SaveSlotRows.Reset();
	for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
	{
		FSaveSlotRowPtr Row = MakeShared<FYogSaveSlotListRow>(SlotIndex);
		if (UYogSaveGame* Save = LoadSaveSlot(SlotIndex))
		{
			Row->bHasData = true;
			Row->bHasPendingRun = Save->RunCheckpoint.bIsValid;
			Row->HighestFloor = Save->Statistics.HighestFloor;
			if (Save->RunCheckpoint.CheckpointFloor > Row->HighestFloor)
			{
				Row->HighestFloor = Save->RunCheckpoint.CheckpointFloor;
			}
			Row->TotalRuns = Save->Statistics.TotalRuns;
			Row->TotalKills = Save->Statistics.TotalKills;
			Row->TotalGoldEarned = Save->Statistics.TotalGoldEarned;
		}
		SaveSlotRows.Add(Row);
	}

	if (SaveSlotListView)
	{
		SaveSlotListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SMetaProgressionWorkbenchWidget::GenerateNodeRow(FNodeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMetaNodeTableRow, OwnerTable).Item(Row);
}

void SMetaProgressionWorkbenchWidget::OnNodeSelectionChanged(FNodeRowPtr Row, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedRow = Row;
	if (Row.IsValid() && EditProxy)
	{
		EditProxy->LoadFromRow(GetUpgradeTable(), Row->RowName);
		DetailsTitleText = FText::Format(LOCTEXT("NodeDetailsTitle", "Node: {0}"), FText::FromName(Row->RowName));
		NodeDetailsView->SetObject(EditProxy.Get());
	}
	else
	{
		DetailsTitleText = FText::GetEmpty();
		NodeDetailsView->SetObject(nullptr);
	}
}

TSharedRef<ITableRow> SMetaProgressionWorkbenchWidget::GenerateCurrencyRow(FCurrencyRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMetaCurrencyTableRow, OwnerTable).Item(Row);
}

void SMetaProgressionWorkbenchWidget::OnCurrencySelectionChanged(FCurrencyRowPtr Row, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedCurrencyRow = Row;
	if (Row.IsValid() && CurrencyEditProxy)
	{
		CurrencyEditProxy->LoadFromRow(GetCurrencyTable(), Row->RowName);
		DetailsTitleText = FText::Format(LOCTEXT("CurrencyDetailsTitle", "Currency: {0}"), FText::FromName(Row->RowName));
		NodeDetailsView->SetObject(CurrencyEditProxy.Get());
	}
	else
	{
		DetailsTitleText = FText::GetEmpty();
		NodeDetailsView->SetObject(nullptr);
	}
}

TSharedRef<ITableRow> SMetaProgressionWorkbenchWidget::GenerateSaveSlotRow(FSaveSlotRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SYogSaveSlotTableRow, OwnerTable).Item(Row);
}

void SMetaProgressionWorkbenchWidget::OnSaveSlotSelectionChanged(FSaveSlotRowPtr Row, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedSaveSlotRow = Row;
	if (Row.IsValid() && SaveSlotEditProxy)
	{
		SaveSlotEditProxy->LoadFromSave(LoadSaveSlot(Row->SlotIndex), Row->SlotIndex);
		DetailsTitleText = FText::Format(LOCTEXT("SaveDetailsTitle", "Save Slot: {0}"), FText::AsNumber(Row->SlotIndex + 1));
		NodeDetailsView->SetObject(SaveSlotEditProxy.Get());
	}
	else
	{
		DetailsTitleText = FText::GetEmpty();
		NodeDetailsView->SetObject(nullptr);
	}
}

FReply SMetaProgressionWorkbenchWidget::OnRefreshClicked()
{
	RefreshData();
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnExportNodeCsvClicked()
{
	ExportTableCsv(GetUpgradeTable(), TEXT("MetaNodes"), StatusText);
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnImportNodeCsvClicked()
{
	ImportTableCsv(
		GetUpgradeTable(),
		LOCTEXT("ImportNodeCSVTx", "Import Meta Node CSV"),
		TEXT("Choose node CSV file"),
		StatusText);
	RefreshData(StatusText);
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnExportCurrencyCsvClicked()
{
	ExportTableCsv(GetCurrencyTable(), TEXT("MetaCurrencies"), StatusText);
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnImportCurrencyCsvClicked()
{
	ImportTableCsv(
		GetCurrencyTable(),
		LOCTEXT("ImportCurrencyCSVTx", "Import Meta Currency CSV"),
		TEXT("Choose currency CSV file"),
		StatusText);
	RefreshData(StatusText);
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnSaveDataTablesClicked()
{
	StatusText = SaveDirtyDataTables()
		? LOCTEXT("SaveDataTablesOK", "DataTables saved.")
		: LOCTEXT("SaveDataTablesFail", "No dirty DataTables were saved, or save failed.");
	RefreshData(StatusText);
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnSaveSelectedSlotClicked()
{
	if (!SelectedSaveSlotRow.IsValid() || !SaveSlotEditProxy)
	{
		StatusText = LOCTEXT("NoSaveSlotSelected", "Select a save slot first.");
		return FReply::Handled();
	}

	const int32 SlotIndex = SelectedSaveSlotRow->SlotIndex;
	UYogSaveGame* Save = LoadSaveSlot(SlotIndex);
	if (!Save)
	{
		Save = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		if (!Save)
		{
			StatusText = LOCTEXT("CreateSaveFail", "Failed to create save object.");
			return FReply::Handled();
		}
		Save->SlotCreatedTime = FDateTime::Now();
	}

	SaveSlotEditProxy->ApplyToSave(Save);
	const FString SlotName = GetSlotName(SlotIndex);
	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	StatusText = bSaved
		? FText::Format(LOCTEXT("SaveSlotOK", "Saved {0}."), FText::FromString(SlotName))
		: FText::Format(LOCTEXT("SaveSlotFail", "Failed to save {0}."), FText::FromString(SlotName));

	RefreshSaveSlotData();
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnExportSelectedSlotClicked()
{
	if (!SelectedSaveSlotRow.IsValid() || !SaveSlotEditProxy)
	{
		StatusText = LOCTEXT("NoExportSaveSlotSelected", "Select a save slot first.");
		return FReply::Handled();
	}

	const FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString OutPath = FPaths::ProjectSavedDir() / TEXT("Balance") /
		FString::Printf(TEXT("%s_%s.txt"), *GetSlotName(SelectedSaveSlotRow->SlotIndex), *TimeStamp);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutPath), true);

	if (FFileHelper::SaveStringToFile(SaveSlotEditProxy->BuildExportText(), *OutPath))
	{
		StatusText = FText::Format(LOCTEXT("ExportSlotOK", "Exported save slot: {0}"), FText::FromString(OutPath));
	}
	else
	{
		StatusText = LOCTEXT("ExportSlotFail", "Failed to export save slot.");
	}
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnDeleteSelectedSlotClicked()
{
	if (!SelectedSaveSlotRow.IsValid())
	{
		StatusText = LOCTEXT("NoDeleteSaveSlotSelected", "Select a save slot first.");
		return FReply::Handled();
	}

	const FString SlotName = GetSlotName(SelectedSaveSlotRow->SlotIndex);
	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	StatusText = bDeleted
		? FText::Format(LOCTEXT("DeleteSlotOK", "Deleted {0}."), FText::FromString(SlotName))
		: FText::Format(LOCTEXT("DeleteSlotFail", "No save data deleted for {0}."), FText::FromString(SlotName));

	RefreshSaveSlotData();
	if (SaveSlotEditProxy)
	{
		SaveSlotEditProxy->LoadFromSave(nullptr, SelectedSaveSlotRow->SlotIndex);
		NodeDetailsView->SetObject(SaveSlotEditProxy.Get());
	}
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnValidateClicked()
{
	UDataTable* NodeTable = GetUpgradeTable();
	UDataTable* CurrencyTable = GetCurrencyTable();
	if (!NodeTable)
	{
		StatusText = LOCTEXT("NoNodeTableValidate", "MetaUpgradeNodeTable is not configured.");
		return FReply::Handled();
	}

	TSet<FGameplayTag> CurrencyTags;
	if (CurrencyTable)
	{
		for (const TPair<FName, uint8*>& Pair : CurrencyTable->GetRowMap())
		{
			const FMetaCurrencyRow* Row = reinterpret_cast<const FMetaCurrencyRow*>(Pair.Value);
			if (!Row)
			{
				continue;
			}
			if (!Row->CurrencyTag.IsValid())
			{
				StatusText = FText::Format(LOCTEXT("CurrencyInvalidTag", "Currency row {0} has an invalid tag."), FText::FromName(Pair.Key));
				return FReply::Handled();
			}
			CurrencyTags.Add(Row->CurrencyTag);
		}
	}

	TArray<FString> Errors;
	const TMap<FName, uint8*>& RowMap = NodeTable->GetRowMap();
	for (const TPair<FName, uint8*>& Pair : RowMap)
	{
		const FName RowName = Pair.Key;
		const FMetaUpgradeNodeRow* Row = reinterpret_cast<const FMetaUpgradeNodeRow*>(Pair.Value);
		if (!Row)
		{
			continue;
		}

		if (Row->MaxLevel <= 0)
		{
			Errors.Add(FString::Printf(TEXT("[%s] MaxLevel must be > 0"), *RowName.ToString()));
		}
		for (const FMetaCurrencyCost& Cost : Row->CostsPerLevel)
		{
			if (!Cost.CurrencyTag.IsValid())
			{
				Errors.Add(FString::Printf(TEXT("[%s] Cost currency tag is invalid"), *RowName.ToString()));
			}
			else if (CurrencyTable && !CurrencyTags.Contains(Cost.CurrencyTag))
			{
				Errors.Add(FString::Printf(TEXT("[%s] Cost currency tag '%s' is missing from currency table"),
					*RowName.ToString(),
					*Cost.CurrencyTag.ToString()));
			}
			if (Cost.Amount < 0)
			{
				Errors.Add(FString::Printf(TEXT("[%s] Cost amount cannot be negative"), *RowName.ToString()));
			}
		}
		for (const FName& Prereq : Row->Prerequisites)
		{
			if (!RowMap.Contains(Prereq))
			{
				Errors.Add(FString::Printf(TEXT("[%s] Missing prerequisite node '%s'"), *RowName.ToString(), *Prereq.ToString()));
			}
		}
	}

	if (Errors.IsEmpty())
	{
		StatusText = FText::Format(LOCTEXT("ValidateOK", "Validation passed: {0} nodes, {1} currency rows."),
			FText::AsNumber(RowMap.Num()),
			FText::AsNumber(CurrencyTags.Num()));
	}
	else
	{
		FString ErrorText;
		for (const FString& Error : Errors)
		{
			ErrorText += TEXT("- ") + Error + TEXT("\n");
		}
		StatusText = FText::Format(LOCTEXT("ValidateFail", "Validation failed: {0} issue(s).\n{1}"),
			FText::AsNumber(Errors.Num()),
			FText::FromString(ErrorText));
	}
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnSideFilterClicked(int32 NewFilter)
{
	SideFilter = NewFilter;
	RebuildFilteredRows();
	return FReply::Handled();
}

FText SMetaProgressionWorkbenchWidget::GetStatsSummaryText() const
{
	int32 Mystic = 0;
	int32 Flesh = 0;
	for (const FNodeRowPtr& Row : AllRows)
	{
		if (Row.IsValid() && Row->Data.Side == EMetaSide::Mystic)
		{
			++Mystic;
		}
		else if (Row.IsValid())
		{
			++Flesh;
		}
	}

	int32 ExistingSlots = 0;
	for (const FSaveSlotRowPtr& Row : SaveSlotRows)
	{
		if (Row.IsValid() && Row->bHasData)
		{
			++ExistingSlots;
		}
	}

	return FText::Format(
		LOCTEXT("StatsSummary", "Nodes: {0} (Flesh {1} / Mystic {2}) | Currency rows: {3} | Save slots with data: {4}/3"),
		FText::AsNumber(AllRows.Num()),
		FText::AsNumber(Flesh),
		FText::AsNumber(Mystic),
		FText::AsNumber(CurrencyRows.Num()),
		FText::AsNumber(ExistingSlots));
}

FText SMetaProgressionWorkbenchWidget::GetDetailsTitleText() const
{
	return DetailsTitleText.IsEmpty()
		? LOCTEXT("NoDetailsSelection", "Select a node, currency row, or save slot.")
		: DetailsTitleText;
}

FSlateColor SMetaProgressionWorkbenchWidget::GetSideFilterColor(int32 Filter) const
{
	return SideFilter == Filter
		? FSlateColor(FLinearColor(0.2f, 0.5f, 0.9f))
		: FSlateColor(FLinearColor(0.28f, 0.28f, 0.28f));
}

FString SMetaProgressionWorkbenchWidget::GetSlotName(int32 SlotIndex) const
{
	return FString::Printf(TEXT("SaveSlot_%d"), FMath::Clamp(SlotIndex, 0, 2));
}

UYogSaveGame* SMetaProgressionWorkbenchWidget::LoadSaveSlot(int32 SlotIndex) const
{
	const FString SlotName = GetSlotName(SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return nullptr;
	}
	return Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
}

bool SMetaProgressionWorkbenchWidget::SaveDirtyDataTables()
{
	TArray<UPackage*> PackagesToSave;
	for (UDataTable* Table : { GetUpgradeTable(), GetCurrencyTable() })
	{
		if (Table && Table->GetOutermost() && Table->GetOutermost()->IsDirty())
		{
			PackagesToSave.AddUnique(Table->GetOutermost());
		}
	}

	if (PackagesToSave.IsEmpty())
	{
		return false;
	}

	return UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
}

bool SMetaProgressionWorkbenchWidget::ExportTableCsv(UDataTable* Table, const FString& FilePrefix, FText& OutStatus) const
{
	if (!Table)
	{
		OutStatus = FText::Format(LOCTEXT("NoTableExport", "{0} table is not configured."), FText::FromString(FilePrefix));
		return false;
	}

	const FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString OutPath = FPaths::ProjectSavedDir() / TEXT("Balance") /
		FString::Printf(TEXT("%s_%s.csv"), *FilePrefix, *TimeStamp);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutPath), true);

	if (!FFileHelper::SaveStringToFile(Table->GetTableAsCSV(), *OutPath))
	{
		OutStatus = FText::Format(LOCTEXT("ExportFail", "Failed to export {0}."), FText::FromString(FilePrefix));
		return false;
	}

	OutStatus = FText::Format(LOCTEXT("ExportOK", "Exported {0}: {1}"), FText::FromString(FilePrefix), FText::FromString(OutPath));
	return true;
}

bool SMetaProgressionWorkbenchWidget::ImportTableCsv(UDataTable* Table, const FText& TransactionText, const FString& DialogTitle, FText& OutStatus)
{
	if (!Table)
	{
		OutStatus = LOCTEXT("NoTableImport", "DataTable is not configured.");
		return false;
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		OutStatus = LOCTEXT("NoDesktopPlatform", "Desktop platform module is unavailable.");
		return false;
	}

	TArray<FString> OutFiles;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		DialogTitle,
		FPaths::ProjectSavedDir() / TEXT("Balance"),
		TEXT(""),
		TEXT("CSV files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFiles);
	if (!bOpened || OutFiles.IsEmpty())
	{
		OutStatus = LOCTEXT("ImportCancelled", "Import cancelled.");
		return false;
	}

	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *OutFiles[0]))
	{
		OutStatus = LOCTEXT("ImportReadFail", "Failed to read CSV file.");
		return false;
	}

	UDataTable* TempTable = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	TempTable->RowStruct = Table->RowStruct;
	const TArray<FString> Problems = TempTable->CreateTableFromCSVString(CSVContent);
	if (!Problems.IsEmpty())
	{
		FString ProblemText;
		for (const FString& Problem : Problems)
		{
			ProblemText += TEXT("- ") + Problem + TEXT("\n");
		}
		OutStatus = FText::Format(LOCTEXT("ImportValidateFail", "CSV validation failed:\n{0}"), FText::FromString(ProblemText));
		return false;
	}

	{
		FScopedTransaction Transaction(TransactionText);
		Table->Modify();
		Table->CreateTableFromCSVString(CSVContent);
		Table->MarkPackageDirty();
	}

	SaveDirtyDataTables();
	OutStatus = FText::Format(LOCTEXT("ImportOK", "Imported CSV: {0}"), FText::FromString(OutFiles[0]));
	return true;
}

#undef LOCTEXT_NAMESPACE
