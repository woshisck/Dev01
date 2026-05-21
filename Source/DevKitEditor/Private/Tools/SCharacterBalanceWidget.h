#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UCharacterData;
class UDataTable;

struct FCharacterBalanceRow
{
	explicit FCharacterBalanceRow(UCharacterData* InAsset)
		: Asset(InAsset)
	{
	}

	TWeakObjectPtr<UCharacterData> Asset;
};

class SCharacterBalanceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterBalanceWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenAsset(TSharedPtr<FCharacterBalanceRow> Row) const;
	void OpenBaseTable(TSharedPtr<FCharacterBalanceRow> Row) const;
	void OpenMovementTable(TSharedPtr<FCharacterBalanceRow> Row) const;
	void CommitFloat(TSharedPtr<FCharacterBalanceRow> Row, FName ColumnName, float NewValue);

private:
	using FRowPtr = TSharedPtr<FCharacterBalanceRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	TSharedRef<ITableRow> GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	FText GetStatsText() const;
	FReply OnRefreshClicked();
	FReply OnValidateClicked();
	FReply OnExportCsvClicked();

	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> ListView;
	FText StatusText;
};
