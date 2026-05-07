#include "Tools/SDataEditorWidget.h"

#include "Editor.h"
#include "FlowAsset.h"
#include "HAL/PlatformApplicationMisc.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tools/DataEditorLibrary.h"
#include "Tools/DataValidator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SDataEditorWidget"

namespace
{
	FString DataEditorWidgetRarityToString(ERuneRarity Rarity)
	{
		switch (Rarity)
		{
		case ERuneRarity::Common:    return TEXT("Common");
		case ERuneRarity::Rare:      return TEXT("Rare");
		case ERuneRarity::Epic:      return TEXT("Epic");
		case ERuneRarity::Legendary: return TEXT("Legendary");
		}
		return TEXT("Unknown");
	}

	FString DataEditorWidgetTriggerTypeToString(ERuneTriggerType TriggerType)
	{
		switch (TriggerType)
		{
		case ERuneTriggerType::Passive:          return TEXT("Passive");
		case ERuneTriggerType::OnAttackHit:      return TEXT("OnAttackHit");
		case ERuneTriggerType::OnDash:           return TEXT("OnDash");
		case ERuneTriggerType::OnKill:           return TEXT("OnKill");
		case ERuneTriggerType::OnCritHit:        return TEXT("OnCritHit");
		case ERuneTriggerType::OnDamageReceived: return TEXT("OnDamageReceived");
		}
		return TEXT("Unknown");
	}

	FString ChainRoleToString(ERuneChainRole ChainRole)
	{
		switch (ChainRole)
		{
		case ERuneChainRole::None:     return TEXT("None");
		case ERuneChainRole::Producer: return TEXT("Producer");
		case ERuneChainRole::Consumer: return TEXT("Consumer");
		}
		return TEXT("Unknown");
	}

	ERuneRarity RarityFromString(const FString& Rarity)
	{
		if (Rarity.Equals(TEXT("Rare"), ESearchCase::IgnoreCase))
		{
			return ERuneRarity::Rare;
		}
		if (Rarity.Equals(TEXT("Epic"), ESearchCase::IgnoreCase))
		{
			return ERuneRarity::Epic;
		}
		if (Rarity.Equals(TEXT("Legendary"), ESearchCase::IgnoreCase))
		{
			return ERuneRarity::Legendary;
		}
		return ERuneRarity::Common;
	}

	ERuneTriggerType TriggerTypeFromString(const FString& TriggerType)
	{
		if (TriggerType.Equals(TEXT("OnAttackHit"), ESearchCase::IgnoreCase)) return ERuneTriggerType::OnAttackHit;
		if (TriggerType.Equals(TEXT("OnDash"), ESearchCase::IgnoreCase)) return ERuneTriggerType::OnDash;
		if (TriggerType.Equals(TEXT("OnKill"), ESearchCase::IgnoreCase)) return ERuneTriggerType::OnKill;
		if (TriggerType.Equals(TEXT("OnCritHit"), ESearchCase::IgnoreCase)) return ERuneTriggerType::OnCritHit;
		if (TriggerType.Equals(TEXT("OnDamageReceived"), ESearchCase::IgnoreCase)) return ERuneTriggerType::OnDamageReceived;
		return ERuneTriggerType::Passive;
	}

	TSharedRef<STextBlock> MakeCellText(const FString& Value, const FText& ToolTip = FText::GetEmpty())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Value))
			.ToolTipText(ToolTip);
	}
}

class SDataEditorRuneTableRow : public SMultiColumnTableRow<TSharedPtr<FDataEditorRuneRow>>
{
public:
	SLATE_BEGIN_ARGS(SDataEditorRuneTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FDataEditorRuneRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SDataEditorWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;

		SMultiColumnTableRow<TSharedPtr<FDataEditorRuneRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FDataEditorRuneRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		URuneDataAsset* Rune = Item.IsValid() ? Item->Asset.Get() : nullptr;
		if (!Rune)
		{
			return MakeCellText(TEXT("-"));
		}

		if (ColumnName == TEXT("Asset"))
		{
			return MakeCellText(Rune->GetName(), FText::FromString(Rune->GetPathName()));
		}
		if (ColumnName == TEXT("RuneIdTag"))
		{
			return MakeCellText(Rune->GetRuneIdTag().ToString());
		}
		if (ColumnName == TEXT("LegacyRuneID"))
		{
			return MakeCellText(FString::FromInt(Rune->GetLegacyRuneID()));
		}
		if (ColumnName == TEXT("RuneName"))
		{
			return MakeCellText(Rune->GetRuneName().ToString());
		}
		if (ColumnName == TEXT("RuneType"))
		{
			return MakeCellText(FString::FromInt(static_cast<int32>(Rune->GetRuneType())));
		}
		if (ColumnName == TEXT("Rarity"))
		{
			return MakeCellText(DataEditorWidgetRarityToString(Rune->GetRarity()));
		}
		if (ColumnName == TEXT("TriggerType"))
		{
			return MakeCellText(DataEditorWidgetTriggerTypeToString(Rune->GetTriggerType()));
		}
		if (ColumnName == TEXT("GoldCost"))
		{
			return MakeCellText(FString::FromInt(Rune->GetGoldCost()));
		}
		if (ColumnName == TEXT("SellPrice"))
		{
			return MakeCellText(FString::FromInt(Rune->GetSellPrice()));
		}
		if (ColumnName == TEXT("ChainRole"))
		{
			return MakeCellText(ChainRoleToString(Rune->GetChainRole()));
		}
		if (ColumnName == TEXT("FlowAsset"))
		{
			return MakeCellText(GetNameSafe(Rune->GetFlowAsset()));
		}
		if (ColumnName == TEXT("Tuning"))
		{
			return MakeCellText(FString::FromInt(Rune->GetTuningScalars().Num()));
		}
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenAssetButton", "Open"))
					.ToolTipText(LOCTEXT("OpenAssetTooltip", "Open this Rune DataAsset in the asset editor."))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SDataEditorWidget> PinnedOwner = Owner.Pin())
						{
							PinnedOwner->OpenRuneAsset(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("CopyTagButton", "Copy Tag"))
					.ToolTipText(LOCTEXT("CopyTagTooltip", "Copy RuneIdTag to the clipboard."))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SDataEditorWidget> PinnedOwner = Owner.Pin())
						{
							PinnedOwner->CopyRuneIdTag(Row);
						}
						return FReply::Handled();
					})
				];
		}

		return MakeCellText(TEXT("-"));
	}

private:
	TSharedPtr<FDataEditorRuneRow> Item;
	TWeakPtr<SDataEditorWidget> OwnerWidget;
};

void SDataEditorWidget::Construct(const FArguments& InArgs)
{
	RarityOptions.Add(MakeShared<FString>(TEXT("Common")));
	RarityOptions.Add(MakeShared<FString>(TEXT("Rare")));
	RarityOptions.Add(MakeShared<FString>(TEXT("Epic")));
	RarityOptions.Add(MakeShared<FString>(TEXT("Legendary")));
	SelectedRarityOption = RarityOptions[0];
	TriggerTypeOptions.Add(MakeShared<FString>(TEXT("Passive")));
	TriggerTypeOptions.Add(MakeShared<FString>(TEXT("OnAttackHit")));
	TriggerTypeOptions.Add(MakeShared<FString>(TEXT("OnDash")));
	TriggerTypeOptions.Add(MakeShared<FString>(TEXT("OnKill")));
	TriggerTypeOptions.Add(MakeShared<FString>(TEXT("OnCritHit")));
	TriggerTypeOptions.Add(MakeShared<FString>(TEXT("OnDamageReceived")));
	SelectedTriggerTypeOption = TriggerTypeOptions[0];
	StatusText = LOCTEXT("ReadyStatus", "Ready");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshButton", "Refresh"))
				.ToolTipText(LOCTEXT("RefreshTooltip", "Reload RuneDA and EffectDA counts from the Asset Registry."))
				.OnClicked(this, &SDataEditorWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ValidateAllButton", "Validate All"))
				.ToolTipText(LOCTEXT("ValidateAllTooltip", "Run UDataValidator::ValidateAll and print details to Output Log."))
				.OnClicked(this, &SDataEditorWidget::OnValidateAllClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ExportCsvButton", "Export CSV"))
				.ToolTipText(LOCTEXT("ExportCsvTooltip", "Export Rune and Effect CSV snapshots to Saved/Balance."))
				.OnClicked(this, &SDataEditorWidget::OnExportCsvClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("MigrationPrepareButton", "Migration Prepare"))
				.ToolTipText(LOCTEXT("MigrationPrepareTooltip", "Write missing legacy RuneID tags to Config/Tags/RuneIDs.ini. Restart the editor before Apply."))
				.OnClicked(this, &SDataEditorWidget::OnMigrationPrepareClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("MigrationApplyButton", "Migration Apply"))
				.ToolTipText(LOCTEXT("MigrationApplyTooltip", "Apply registered Rune.ID.Legacy_* tags to RuneDA assets. Marks assets dirty, does not auto-save."))
				.OnClicked(this, &SDataEditorWidget::OnMigrationApplyClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 0.f, 8.f, 6.f)
		[
			SNew(STextBlock)
			.Text(this, &SDataEditorWidget::GetStatsText)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BatchLabel", "Batch selected:"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(120.f)
				[
					SNew(SNumericEntryBox<int32>)
					.MinValue(0)
					.Value(this, &SDataEditorWidget::GetPendingGoldCost)
					.OnValueChanged(this, &SDataEditorWidget::OnGoldCostChanged)
					.OnValueCommitted(this, &SDataEditorWidget::OnGoldCostCommitted)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("SetGoldCostButton", "Set GoldCost"))
				.ToolTipText(LOCTEXT("SetGoldCostTooltip", "Set GoldCost on selected RuneDA assets. Uses Undo transaction and does not auto-save."))
				.IsEnabled(this, &SDataEditorWidget::HasSelectedRunes)
				.OnClicked(this, &SDataEditorWidget::OnBatchGoldCostClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(140.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&RarityOptions)
					.InitiallySelectedItem(SelectedRarityOption)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
					})
					.OnSelectionChanged(this, &SDataEditorWidget::OnRaritySelectionChanged)
					[
						SNew(STextBlock)
						.Text(this, &SDataEditorWidget::GetSelectedRarityText)
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("SetRarityButton", "Set Rarity"))
				.ToolTipText(LOCTEXT("SetRarityTooltip", "Set Rarity on selected RuneDA assets. Uses Undo transaction and does not auto-save."))
				.IsEnabled(this, &SDataEditorWidget::HasSelectedRunes)
				.OnClicked(this, &SDataEditorWidget::OnBatchRarityClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(160.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&TriggerTypeOptions)
					.InitiallySelectedItem(SelectedTriggerTypeOption)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
					})
					.OnSelectionChanged(this, &SDataEditorWidget::OnTriggerTypeSelectionChanged)
					[
						SNew(STextBlock)
						.Text(this, &SDataEditorWidget::GetSelectedTriggerTypeText)
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("SetTriggerTypeButton", "Set Trigger"))
				.ToolTipText(LOCTEXT("SetTriggerTypeTooltip", "Set TriggerType on selected RuneDA assets. Uses Undo transaction and does not auto-save."))
				.IsEnabled(this, &SDataEditorWidget::HasSelectedRunes)
				.OnClicked(this, &SDataEditorWidget::OnBatchTriggerTypeClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ScalarLabel", "Tuning scalar:"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(160.f)
				[
					SAssignNew(ScalarKeyTextBox, SEditableTextBox)
					.HintText(LOCTEXT("ScalarKeyHint", "Key, e.g. Damage"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(120.f)
				[
					SNew(SNumericEntryBox<float>)
					.Value(this, &SDataEditorWidget::GetPendingScalarValue)
					.OnValueChanged(this, &SDataEditorWidget::OnScalarValueChanged)
					.OnValueCommitted(this, &SDataEditorWidget::OnScalarValueCommitted)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("SetScalarButton", "Set Scalar"))
				.ToolTipText(LOCTEXT("SetScalarTooltip", "Add or update a tuning scalar on selected RuneDA assets. Uses Undo transaction and does not auto-save."))
				.IsEnabled(this, &SDataEditorWidget::HasSelectedRunes)
				.OnClicked(this, &SDataEditorWidget::OnSetTuningScalarClicked)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SBorder)
			.Padding(2.f)
			[
				SAssignNew(RuneListView, SListView<FRuneRowPtr>)
				.ListItemsSource(&RuneRows)
				.SelectionMode(ESelectionMode::Multi)
				.OnGenerateRow(this, &SDataEditorWidget::GenerateRuneRow)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Asset")).DefaultLabel(LOCTEXT("AssetColumn", "Asset")).FillWidth(1.2f)
					+ SHeaderRow::Column(TEXT("RuneIdTag")).DefaultLabel(LOCTEXT("RuneIdTagColumn", "RuneIdTag")).FillWidth(1.8f)
					+ SHeaderRow::Column(TEXT("LegacyRuneID")).DefaultLabel(LOCTEXT("LegacyRuneIDColumn", "LegacyRuneID")).FixedWidth(92.f)
					+ SHeaderRow::Column(TEXT("RuneName")).DefaultLabel(LOCTEXT("RuneNameColumn", "RuneName")).FillWidth(1.f)
					+ SHeaderRow::Column(TEXT("RuneType")).DefaultLabel(LOCTEXT("RuneTypeColumn", "RuneType")).FixedWidth(80.f)
					+ SHeaderRow::Column(TEXT("Rarity")).DefaultLabel(LOCTEXT("RarityColumn", "Rarity")).FixedWidth(82.f)
					+ SHeaderRow::Column(TEXT("TriggerType")).DefaultLabel(LOCTEXT("TriggerTypeColumn", "TriggerType")).FixedWidth(130.f)
					+ SHeaderRow::Column(TEXT("GoldCost")).DefaultLabel(LOCTEXT("GoldCostColumn", "GoldCost")).FixedWidth(78.f)
					+ SHeaderRow::Column(TEXT("SellPrice")).DefaultLabel(LOCTEXT("SellPriceColumn", "SellPrice")).FixedWidth(78.f)
					+ SHeaderRow::Column(TEXT("ChainRole")).DefaultLabel(LOCTEXT("ChainRoleColumn", "ChainRole")).FixedWidth(90.f)
					+ SHeaderRow::Column(TEXT("FlowAsset")).DefaultLabel(LOCTEXT("FlowAssetColumn", "FlowAsset")).FillWidth(1.f)
					+ SHeaderRow::Column(TEXT("Tuning")).DefaultLabel(LOCTEXT("TuningColumn", "Tuning")).FixedWidth(70.f)
					+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("ActionsColumn", "Actions")).FixedWidth(150.f)
				)
			]
		]
	];

	RefreshData(LOCTEXT("InitialRefreshStatus", "Ready. Rune list refreshed."));
}

void SDataEditorWidget::OpenRuneAsset(TSharedPtr<FDataEditorRuneRow> Row) const
{
	URuneDataAsset* Rune = Row.IsValid() ? Row->Asset.Get() : nullptr;
	if (!Rune || !GEditor)
	{
		return;
	}

	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditorSubsystem->OpenEditorForAsset(Rune);
	}
}

void SDataEditorWidget::CopyRuneIdTag(TSharedPtr<FDataEditorRuneRow> Row) const
{
	URuneDataAsset* Rune = Row.IsValid() ? Row->Asset.Get() : nullptr;
	if (!Rune)
	{
		return;
	}

	const FString TagText = Rune->GetRuneIdTag().ToString();
	FPlatformApplicationMisc::ClipboardCopy(*TagText);
}

void SDataEditorWidget::RefreshData(const FText& NewStatus)
{
	TArray<URuneDataAsset*> Runes = UDataEditorLibrary::GetAllRuneDAs();
	Runes.Sort([](const URuneDataAsset& A, const URuneDataAsset& B)
	{
		return A.GetName() < B.GetName();
	});

	RuneRows.Reset();
	RuneRows.Reserve(Runes.Num());
	for (URuneDataAsset* Rune : Runes)
	{
		RuneRows.Add(MakeShared<FDataEditorRuneRow>(Rune));
	}

	EffectCount = UDataEditorLibrary::GetAllEffectDAs().Num();
	if (RuneListView.IsValid())
	{
		RuneListView->RequestListRefresh();
	}

	if (!NewStatus.IsEmpty())
	{
		StatusText = NewStatus;
	}
}

TSharedRef<ITableRow> SDataEditorWidget::GenerateRuneRow(FRuneRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDataEditorRuneTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

FText SDataEditorWidget::GetStatsText() const
{
	const int32 SelectedCount = RuneListView.IsValid() ? RuneListView->GetNumItemsSelected() : 0;
	return FText::Format(
		LOCTEXT("StatsText", "RuneDA: {0} | EffectDA: {1} | Selected: {2} | {3}"),
		FText::AsNumber(RuneRows.Num()),
		FText::AsNumber(EffectCount),
		FText::AsNumber(SelectedCount),
		StatusText);
}

FText SDataEditorWidget::GetSelectedRarityText() const
{
	return FText::FromString(SelectedRarityOption.IsValid() ? *SelectedRarityOption : FString(TEXT("Common")));
}

FText SDataEditorWidget::GetSelectedTriggerTypeText() const
{
	return FText::FromString(SelectedTriggerTypeOption.IsValid() ? *SelectedTriggerTypeOption : FString(TEXT("Passive")));
}

TOptional<int32> SDataEditorWidget::GetPendingGoldCost() const
{
	return PendingGoldCost;
}

TOptional<float> SDataEditorWidget::GetPendingScalarValue() const
{
	return PendingScalarValue;
}

FReply SDataEditorWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("RefreshedStatus", "Rune list refreshed."));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnValidateAllClicked()
{
	const FDataValidationReport Report = UDataValidator::ValidateAll();
	StatusText = FText::Format(
		LOCTEXT("ValidatedStatus", "Validate All: {0} scanned, {1} errors, {2} warnings. See Output Log."),
		FText::AsNumber(Report.ScannedCount),
		FText::AsNumber(Report.ErrorCount),
		FText::AsNumber(Report.WarningCount));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnExportCsvClicked()
{
	const FString RunePath = UDataEditorLibrary::ExportRuneDAsToCSV(TEXT(""));
	const FString EffectPath = UDataEditorLibrary::ExportEffectDAsToCSV(TEXT(""));
	StatusText = FText::Format(
		LOCTEXT("ExportedStatus", "Exported CSV: Rune={0} Effect={1}"),
		FText::FromString(FPaths::GetCleanFilename(RunePath)),
		FText::FromString(FPaths::GetCleanFilename(EffectPath)));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnMigrationPrepareClicked()
{
	const int32 Added = UDataEditorLibrary::PrepareRuneIdTagIni();
	StatusText = FText::Format(
		LOCTEXT("MigrationPrepareStatus", "Migration Prepare wrote {0} new tag lines. Restart editor before Apply if new tags were added."),
		FText::AsNumber(Added));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnMigrationApplyClicked()
{
	const int32 Applied = UDataEditorLibrary::ApplyRuneIdTagsAfterRestart();
	RefreshData(FText::Format(
		LOCTEXT("MigrationApplyStatus", "Migration Apply updated {0} RuneDA assets. Assets are dirty, not auto-saved."),
		FText::AsNumber(Applied)));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnBatchGoldCostClicked()
{
	TArray<URuneDataAsset*> SelectedRunes = GetSelectedRuneAssets();
	UDataEditorLibrary::BatchSetRuneGoldCost(SelectedRunes, PendingGoldCost);
	RefreshData(FText::Format(
		LOCTEXT("BatchGoldStatus", "Set GoldCost={0} on {1} selected RuneDA assets. Assets are dirty, not auto-saved."),
		FText::AsNumber(PendingGoldCost),
		FText::AsNumber(SelectedRunes.Num())));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnBatchRarityClicked()
{
	TArray<URuneDataAsset*> SelectedRunes = GetSelectedRuneAssets();
	const ERuneRarity NewRarity = GetSelectedRarity();
	UDataEditorLibrary::BatchSetRuneRarity(SelectedRunes, NewRarity);
	RefreshData(FText::Format(
		LOCTEXT("BatchRarityStatus", "Set Rarity={0} on {1} selected RuneDA assets. Assets are dirty, not auto-saved."),
		FText::FromString(DataEditorWidgetRarityToString(NewRarity)),
		FText::AsNumber(SelectedRunes.Num())));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnBatchTriggerTypeClicked()
{
	TArray<URuneDataAsset*> SelectedRunes = GetSelectedRuneAssets();
	const ERuneTriggerType NewType = GetSelectedTriggerType();
	UDataEditorLibrary::BatchSetRuneTriggerType(SelectedRunes, NewType);
	RefreshData(FText::Format(
		LOCTEXT("BatchTriggerStatus", "Set TriggerType={0} on {1} selected RuneDA assets. Assets are dirty, not auto-saved."),
		FText::FromString(DataEditorWidgetTriggerTypeToString(NewType)),
		FText::AsNumber(SelectedRunes.Num())));
	return FReply::Handled();
}

FReply SDataEditorWidget::OnSetTuningScalarClicked()
{
	const FString KeyString = ScalarKeyTextBox.IsValid() ? ScalarKeyTextBox->GetText().ToString().TrimStartAndEnd() : FString();
	if (KeyString.IsEmpty())
	{
		StatusText = LOCTEXT("ScalarMissingKey", "Tuning scalar key is empty.");
		return FReply::Handled();
	}

	TArray<URuneDataAsset*> SelectedRunes = GetSelectedRuneAssets();
	const FName Key(*KeyString);
	const FScopedTransaction Tx(LOCTEXT("SetRuneTuningScalar", "Set Rune Tuning Scalar"));
	for (URuneDataAsset* Rune : SelectedRunes)
	{
		if (!Rune)
		{
			continue;
		}
		Rune->Modify();
		TArray<FRuneTuningScalar>& Scalars = Rune->RuneInfo.RuneConfig.TuningScalars;
		FRuneTuningScalar* Existing = Scalars.FindByPredicate([Key](const FRuneTuningScalar& Scalar)
		{
			return Scalar.Key == Key;
		});
		if (!Existing)
		{
			Existing = &Scalars.AddDefaulted_GetRef();
			Existing->Key = Key;
			Existing->DisplayName = FText::FromName(Key);
		}
		Existing->Value = PendingScalarValue;
		Rune->MarkPackageDirty();
	}

	RefreshData(FText::Format(
		LOCTEXT("ScalarSetStatus", "Set tuning scalar {0}={1} on {2} selected RuneDA assets. Assets are dirty, not auto-saved."),
		FText::FromString(KeyString),
		FText::AsNumber(PendingScalarValue),
		FText::AsNumber(SelectedRunes.Num())));
	return FReply::Handled();
}

void SDataEditorWidget::OnGoldCostChanged(int32 NewValue)
{
	PendingGoldCost = NewValue;
}

void SDataEditorWidget::OnGoldCostCommitted(int32 NewValue, ETextCommit::Type CommitType)
{
	PendingGoldCost = NewValue;
}

void SDataEditorWidget::OnScalarValueChanged(float NewValue)
{
	PendingScalarValue = NewValue;
}

void SDataEditorWidget::OnScalarValueCommitted(float NewValue, ETextCommit::Type CommitType)
{
	PendingScalarValue = NewValue;
}

void SDataEditorWidget::OnRaritySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedRarityOption = NewSelection;
	}
}

void SDataEditorWidget::OnTriggerTypeSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedTriggerTypeOption = NewSelection;
	}
}

bool SDataEditorWidget::HasSelectedRunes() const
{
	return RuneListView.IsValid() && RuneListView->GetNumItemsSelected() > 0;
}

TArray<URuneDataAsset*> SDataEditorWidget::GetSelectedRuneAssets() const
{
	TArray<URuneDataAsset*> SelectedRunes;
	if (!RuneListView.IsValid())
	{
		return SelectedRunes;
	}

	TArray<FRuneRowPtr> SelectedRows = RuneListView->GetSelectedItems();
	for (const FRuneRowPtr& Row : SelectedRows)
	{
		if (Row.IsValid())
		{
			if (URuneDataAsset* Rune = Row->Asset.Get())
			{
				SelectedRunes.Add(Rune);
			}
		}
	}
	return SelectedRunes;
}

ERuneRarity SDataEditorWidget::GetSelectedRarity() const
{
	return RarityFromString(SelectedRarityOption.IsValid() ? *SelectedRarityOption : FString(TEXT("Common")));
}

ERuneTriggerType SDataEditorWidget::GetSelectedTriggerType() const
{
	return TriggerTypeFromString(SelectedTriggerTypeOption.IsValid() ? *SelectedTriggerTypeOption : FString(TEXT("Passive")));
}

#undef LOCTEXT_NAMESPACE
