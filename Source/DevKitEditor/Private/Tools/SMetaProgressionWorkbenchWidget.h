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

class SMetaProgressionWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMetaProgressionWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMetaProgressionWorkbenchWidget() override;

private:
	using FNodeRowPtr = TSharedPtr<FMetaNodeListRow>;

	UDataTable* GetUpgradeTable() const;
	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RebuildFilteredRows();

	TSharedRef<ITableRow> GenerateNodeRow(FNodeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnNodeSelectionChanged(FNodeRowPtr Row, ESelectInfo::Type SelectInfo);

	FReply OnRefreshClicked();
	FReply OnExportCsvClicked();
	FReply OnImportCsvClicked();
	FReply OnValidateClicked();
	FReply OnSideFilterClicked(int32 NewFilter);

	FText GetStatsSummaryText() const;
	FSlateColor GetSideFilterColor(int32 Filter) const;

	// -1=All  0=Flesh  1=Mystic
	int32 SideFilter = -1;

	TArray<FNodeRowPtr> AllRows;
	TArray<FNodeRowPtr> FilteredRows;
	FNodeRowPtr SelectedRow;

	TSharedPtr<SListView<FNodeRowPtr>> NodeListView;
	TSharedPtr<IDetailsView> NodeDetailsView;
	FText StatusText;

	TStrongObjectPtr<UMetaNodeEditProxy> EditProxy;
};
