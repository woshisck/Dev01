#pragma once

#include "CoreMinimal.h"
#include "MetaProgression/MetaTypes.h"
#include "Tools/UMetaNodeEditProxy.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class UDataTable;

struct FMetaNodeListRow
{
	FName RowName;
	FMetaUpgradeNodeRow Data;

	explicit FMetaNodeListRow(FName InRowName, const FMetaUpgradeNodeRow& InData)
		: RowName(InRowName), Data(InData) {}
};

struct FMetaCurrencyListRow
{
	FName RowName;
	FMetaCurrencyRow Data;

	explicit FMetaCurrencyListRow(FName InRowName, const FMetaCurrencyRow& InData)
		: RowName(InRowName), Data(InData) {}
};

struct FYogSaveSlotListRow
{
	int32 SlotIndex = 0;
	bool bHasData = false;
	bool bHasPendingRun = false;
	int32 HighestFloor = 0;
	int32 TotalRuns = 0;
	int32 TotalKills = 0;
	int32 TotalGoldEarned = 0;

	explicit FYogSaveSlotListRow(int32 InSlotIndex)
		: SlotIndex(InSlotIndex) {}
};

class SMetaProgressionWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMetaProgressionWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMetaProgressionWorkbenchWidget() override;

private:
	using FNodeRowPtr = TSharedPtr<FMetaNodeListRow>;
	using FCurrencyRowPtr = TSharedPtr<FMetaCurrencyListRow>;
	using FSaveSlotRowPtr = TSharedPtr<FYogSaveSlotListRow>;

	UDataTable* GetUpgradeTable() const;
	UDataTable* GetCurrencyTable() const;
	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RebuildFilteredRows();
	void RefreshCurrencyData();
	void RefreshSaveSlotData();

	TSharedRef<ITableRow> GenerateNodeRow(FNodeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnNodeSelectionChanged(FNodeRowPtr Row, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateCurrencyRow(FCurrencyRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCurrencySelectionChanged(FCurrencyRowPtr Row, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateSaveSlotRow(FSaveSlotRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSaveSlotSelectionChanged(FSaveSlotRowPtr Row, ESelectInfo::Type SelectInfo);

	FReply OnRefreshClicked();
	FReply OnExportNodeCsvClicked();
	FReply OnImportNodeCsvClicked();
	FReply OnExportCurrencyCsvClicked();
	FReply OnImportCurrencyCsvClicked();
	FReply OnSaveDataTablesClicked();
	FReply OnSaveSelectedSlotClicked();
	FReply OnExportSelectedSlotClicked();
	FReply OnDeleteSelectedSlotClicked();
	FReply OnValidateClicked();
	FReply OnSideFilterClicked(int32 NewFilter);

	FText GetStatsSummaryText() const;
	FText GetDetailsTitleText() const;
	FSlateColor GetSideFilterColor(int32 Filter) const;
	FString GetSlotName(int32 SlotIndex) const;
	class UYogSaveGame* LoadSaveSlot(int32 SlotIndex) const;
	bool SaveDirtyDataTables();
	bool ExportTableCsv(UDataTable* Table, const FString& FilePrefix, FText& OutStatus) const;
	bool ImportTableCsv(UDataTable* Table, const FText& TransactionText, const FString& DialogTitle, FText& OutStatus);

	// -1=All  0=Flesh  1=Mystic
	int32 SideFilter = -1;

	TArray<FNodeRowPtr> AllRows;
	TArray<FNodeRowPtr> FilteredRows;
	TArray<FCurrencyRowPtr> CurrencyRows;
	TArray<FSaveSlotRowPtr> SaveSlotRows;
	FNodeRowPtr SelectedRow;
	FCurrencyRowPtr SelectedCurrencyRow;
	FSaveSlotRowPtr SelectedSaveSlotRow;

	TSharedPtr<SListView<FNodeRowPtr>> NodeListView;
	TSharedPtr<SListView<FCurrencyRowPtr>> CurrencyListView;
	TSharedPtr<SListView<FSaveSlotRowPtr>> SaveSlotListView;
	TSharedPtr<IDetailsView> NodeDetailsView;
	FText StatusText;
	FText DetailsTitleText;

	TStrongObjectPtr<UMetaNodeEditProxy> EditProxy;
	TStrongObjectPtr<UMetaCurrencyEditProxy> CurrencyEditProxy;
	TStrongObjectPtr<UYogSaveSlotEditProxy> SaveSlotEditProxy;
};
