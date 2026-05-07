#pragma once

#include "CoreMinimal.h"
#include "Data/RuneDataAsset.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class URuneDataAsset;

struct FDataEditorRuneRow
{
	explicit FDataEditorRuneRow(URuneDataAsset* InAsset)
		: Asset(InAsset)
	{
	}

	TWeakObjectPtr<URuneDataAsset> Asset;
};

class SDataEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDataEditorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenRuneAsset(TSharedPtr<FDataEditorRuneRow> Row) const;
	void CopyRuneIdTag(TSharedPtr<FDataEditorRuneRow> Row) const;

private:
	using FRuneRowPtr = TSharedPtr<FDataEditorRuneRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	TSharedRef<ITableRow> GenerateRuneRow(FRuneRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	FText GetStatsText() const;
	FText GetSelectedRarityText() const;
	FText GetSelectedTriggerTypeText() const;
	TOptional<int32> GetPendingGoldCost() const;
	TOptional<float> GetPendingScalarValue() const;

	FReply OnRefreshClicked();
	FReply OnValidateAllClicked();
	FReply OnExportCsvClicked();
	FReply OnMigrationPrepareClicked();
	FReply OnMigrationApplyClicked();
	FReply OnBatchGoldCostClicked();
	FReply OnBatchRarityClicked();
	FReply OnBatchTriggerTypeClicked();
	FReply OnSetTuningScalarClicked();

	void OnGoldCostChanged(int32 NewValue);
	void OnGoldCostCommitted(int32 NewValue, ETextCommit::Type CommitType);
	void OnScalarValueChanged(float NewValue);
	void OnScalarValueCommitted(float NewValue, ETextCommit::Type CommitType);
	void OnRaritySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnTriggerTypeSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	bool HasSelectedRunes() const;
	TArray<URuneDataAsset*> GetSelectedRuneAssets() const;
	ERuneRarity GetSelectedRarity() const;
	ERuneTriggerType GetSelectedTriggerType() const;

	TArray<FRuneRowPtr> RuneRows;
	TSharedPtr<SListView<FRuneRowPtr>> RuneListView;
	TArray<TSharedPtr<FString>> RarityOptions;
	TSharedPtr<FString> SelectedRarityOption;
	TArray<TSharedPtr<FString>> TriggerTypeOptions;
	TSharedPtr<FString> SelectedTriggerTypeOption;
	TSharedPtr<class SEditableTextBox> ScalarKeyTextBox;
	FText StatusText;
	int32 EffectCount = 0;
	int32 PendingGoldCost = 0;
	float PendingScalarValue = 0.f;
};
