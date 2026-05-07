#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UBlueprint;
class UGA_MusketBase;
class UMontageAttackDataAsset;
class UMontageConfigDA;
class UMontageNotifyEntry;

enum class EActionBalanceRowType : uint8
{
	MeleeAttackData,
	MeleeHitWindow,
	MeleeComboWindow,
	MeleeEarlyExit,
	MusketLight,
	MusketHeavy,
	MusketSprint
};

struct FActionBalanceRow
{
	EActionBalanceRowType Type = EActionBalanceRowType::MeleeAttackData;
	TWeakObjectPtr<UObject> PrimaryObject;
	TWeakObjectPtr<UMontageConfigDA> MontageConfig;
	TWeakObjectPtr<UMontageNotifyEntry> MontageEntry;
	TWeakObjectPtr<UBlueprint> BlueprintAsset;
	TWeakObjectPtr<UGA_MusketBase> MusketCDO;
};

class SActionBalanceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SActionBalanceWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenPrimaryAsset(TSharedPtr<FActionBalanceRow> Row) const;
	void CommitValue(TSharedPtr<FActionBalanceRow> Row, FName ColumnName, float NewValue);
	void CreateAttackDataForHitWindow(TSharedPtr<FActionBalanceRow> Row);
	void CreateMusketTuningData(TSharedPtr<FActionBalanceRow> Row);

private:
	using FRowPtr = TSharedPtr<FActionBalanceRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	TSharedRef<ITableRow> GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	FText GetStatsText() const;
	FReply OnRefreshClicked();
	FReply OnValidateClicked();
	FReply OnExportCsvClicked();

	TArray<FRowPtr> Rows;
	TSharedPtr<SListView<FRowPtr>> ListView;
	FText StatusText;
	int32 MusketBlueprintCount = 0;
	int32 MontageConfigCount = 0;
	int32 AttackDataCount = 0;
};
