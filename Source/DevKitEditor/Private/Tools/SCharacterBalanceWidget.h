#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class UAnimMontage;
class UAN_MeleeDamage;
class UBlueprint;
class UCharacterData;
class UDataTable;
class UGA_MusketBase;
class UMontageAttackDataAsset;
class UMontageConfigDA;
class UMontageNotifyEntry;
class UMusketActionTuningDataAsset;

enum class ECharacterFieldSource : uint8
{
	BaseAttributes,
	Movement
};

struct FCharacterFieldDef
{
	FName ColumnName;
	FString DisplayName;
	FString ChineseName;
	FString Description;
	FString TypeToken;
	FString Platform;
	float DefaultValue = 0.f;
	float MinValue = 0.f;
	float MaxValue = 0.f;
	bool bHasRange = false;
	ECharacterFieldSource Source = ECharacterFieldSource::BaseAttributes;
};

struct FCharacterWorkbenchRow
{
	explicit FCharacterWorkbenchRow(UCharacterData* InAsset)
		: Asset(InAsset)
	{
	}

	TWeakObjectPtr<UCharacterData> Asset;
	FString Status;
};

enum class ECharacterWorkbenchPage : uint8
{
	Summary = 0,
	BaseAttributes,
	Movement,
	AbilityMontage,
	ActData,
	ValidationExport
};

enum class EActDataRowKind : uint8
{
	MontageMap,
	MontageConfig,
	HitWindow,
	AttackDataCandidate,
	ComboWindow,
	EarlyExit,
	TagWindow,
	GameplayEvent,
	NotifyFallback,
	MusketLight,
	MusketHeavy,
	MusketSprint
};

struct FActDataRow
{
	EActDataRowKind Kind = EActDataRowKind::MontageMap;
	FGameplayTag AbilityTag;
	FString TypeLabel;
	FString SourceLabel;
	FString Status;
	FGameplayTagContainer RequiredTags;
	FGameplayTagContainer BlockedTags;
	int32 Priority = 0;
	int32 CandidateIndex = INDEX_NONE;

	TWeakObjectPtr<UCharacterData> Character;
	TWeakObjectPtr<UAnimMontage> Montage;
	TWeakObjectPtr<UMontageConfigDA> MontageConfig;
	TWeakObjectPtr<UMontageNotifyEntry> MontageEntry;
	TWeakObjectPtr<UMontageAttackDataAsset> AttackData;
	TWeakObjectPtr<UAN_MeleeDamage> DamageNotify;
	TWeakObjectPtr<UBlueprint> BlueprintAsset;
	TWeakObjectPtr<UGA_MusketBase> MusketCDO;
};

class SCharacterBalanceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterBalanceWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenAsset(TSharedPtr<FCharacterWorkbenchRow> Row) const;
	void OpenBaseTable(TSharedPtr<FCharacterWorkbenchRow> Row) const;
	void OpenMovementTable(TSharedPtr<FCharacterWorkbenchRow> Row) const;
	void OpenSelectedAbilityData() const;
	void OpenSelectedGasTemplate() const;
	void OpenActPrimaryAsset(TSharedPtr<FActDataRow> Row) const;
	void CommitFloat(TSharedPtr<FCharacterWorkbenchRow> Row, FName ColumnName, float NewValue);
	void CommitActFloat(TSharedPtr<FActDataRow> Row, FName ColumnName, float NewValue);
	void CreateAttackDataForActRow(TSharedPtr<FActDataRow> Row);
	void CreateMusketTuningData(TSharedPtr<FActDataRow> Row);

private:
	using FCharacterRowPtr = TSharedPtr<FCharacterWorkbenchRow>;
	using FActRowPtr = TSharedPtr<FActDataRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RebuildFilteredRows();
	void RebuildAbilityMontageContent();
	void CollectActRowsForSelectedCharacter();
	void CollectMusketActRows();

	TSharedRef<ITableRow> GenerateCharacterRow(FCharacterRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateActRow(FActRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCharacterSelectionChanged(FCharacterRowPtr Row, ESelectInfo::Type SelectInfo);
	void OnActSelectionChanged(FActRowPtr Row, ESelectInfo::Type SelectInfo);
	void OnSearchTextChanged(const FText& NewText);

	TSharedRef<SWidget> BuildCharacterListPanel();
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildSummaryPage();
	TSharedRef<SWidget> BuildFieldsPage(ECharacterFieldSource Source);
	TSharedRef<SWidget> BuildAbilityMontagePage();
	TSharedRef<SWidget> BuildActDataPage();
	TSharedRef<SWidget> BuildValidationExportPage();
	TSharedRef<SWidget> BuildPageButton(const FText& Label, ECharacterWorkbenchPage Page);
	TSharedRef<SWidget> BuildCharacterNumberRow(const FCharacterFieldDef& Field);
	TSharedRef<SWidget> BuildActNumberRow(FName ColumnName, const FString& Label, const FString& Description);
	TSharedRef<SWidget> BuildFieldDictionarySection();

	FText GetStatsText() const;
	FText GetSelectedCharacterNameText() const;
	FText GetSelectedCharacterSummaryText() const;
	FText GetSelectedActSummaryText() const;
	FText GetSelectedActDetailsText() const;
	int32 GetActivePageIndex() const;

	FReply OnRefreshClicked();
	FReply OnValidateClicked();
	FReply OnExportCsvClicked();
	FReply OnPageButtonClicked(ECharacterWorkbenchPage Page);

	UCharacterData* GetSelectedCharacter() const;
	void OpenObject(UObject* Object) const;
	UObject* GetActDetailsObject(const FActDataRow& Row) const;
	bool CanEditActField(const FActDataRow& Row, FName ColumnName) const;

	TArray<FCharacterRowPtr> Rows;
	TArray<FCharacterRowPtr> FilteredRows;
	TArray<FActRowPtr> ActRows;
	TSharedPtr<FCharacterWorkbenchRow> SelectedRow;
	TSharedPtr<FActDataRow> SelectedActRow;

	TSharedPtr<SListView<FCharacterRowPtr>> CharacterListView;
	TSharedPtr<SListView<FActRowPtr>> ActListView;
	TSharedPtr<IDetailsView> CharacterDetailsView;
	TSharedPtr<IDetailsView> ActDetailsView;
	TSharedPtr<SVerticalBox> AbilityMontageContent;

	FText SearchText;
	FText StatusText;
	ECharacterWorkbenchPage ActivePage = ECharacterWorkbenchPage::Summary;
	int32 MusketBlueprintCount = 0;
};
