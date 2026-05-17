#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class UAN_MeleeDamage;
class UAnimMontage;
class UGameplayAbilityComboGraph;
class UGameplayAbilityComboGraphNode;
class UMontageConfigDA;
class UWeaponDefinition;

struct FComboGraphManagerRow
{
	TWeakObjectPtr<UWeaponDefinition> Weapon;
	TWeakObjectPtr<UGameplayAbilityComboGraph> Graph;
	TWeakObjectPtr<UGameplayAbilityComboGraphNode> Node;
	TWeakObjectPtr<UMontageConfigDA> MontageConfig;
	TWeakObjectPtr<UAnimMontage> Montage;
	TWeakObjectPtr<UAN_MeleeDamage> DamageNotify;
	FString OwnerType;
	FString OwnerName;
	FString Status;
};

class SComboGraphManagerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SComboGraphManagerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenPrimaryAsset(TSharedPtr<FComboGraphManagerRow> Row) const;
	void CommitFloat(TSharedPtr<FComboGraphManagerRow> Row, FName ColumnName, float NewValue);
	void MigrateNodeFromNotify(TSharedPtr<FComboGraphManagerRow> Row);

private:
	using FRowPtr = TSharedPtr<FComboGraphManagerRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	TSharedRef<ITableRow> GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(FRowPtr Row, ESelectInfo::Type SelectInfo);

	TSharedRef<SWidget> BuildNumberRow(FName ColumnName, const FString& Label, const FString& Description);
	FText GetStatsText() const;
	FText GetSelectionText() const;
	FReply OnRefreshClicked();

	UObject* GetDetailsObject(const FComboGraphManagerRow& Row) const;
	bool CanEditFloat(const FComboGraphManagerRow& Row, FName ColumnName) const;

	TArray<FRowPtr> Rows;
	TSharedPtr<FComboGraphManagerRow> SelectedRow;
	TSharedPtr<SListView<FRowPtr>> ListView;
	TSharedPtr<IDetailsView> DetailsView;
	FText StatusText;
};
