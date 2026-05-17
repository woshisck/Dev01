#include "Tools/SStoryEventWorkbenchWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Story/StoryEventTypes.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SStoryEventWorkbenchWidget"

namespace
{
	template <typename T>
	TArray<T*> CollectAssetsOfClass()
	{
		TArray<T*> Out;
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		if (AssetRegistry.IsLoadingAssets())
		{
			AssetRegistry.SearchAllAssets(true);
		}

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (T* Loaded = Cast<T>(Asset.GetAsset()))
			{
				Out.Add(Loaded);
			}
		}

		Out.Sort([](const T& A, const T& B)
		{
			return A.GetName() < B.GetName();
		});
		return Out;
	}

	TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = FString())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip));
	}

	FString ActionTypeText(EStoryEventActionType ActionType)
	{
		switch (ActionType)
		{
		case EStoryEventActionType::BroadcastOnly:
			return TEXT("Broadcast");
		case EStoryEventActionType::TutorialPopup:
			return TEXT("Tutorial");
		case EStoryEventActionType::LevelFlow:
			return TEXT("LevelFlow");
		case EStoryEventActionType::None:
		default:
			return TEXT("None");
		}
	}

	int32 CountActionType(const UStoryEventRegistryDA* Registry, EStoryEventActionType ActionType)
	{
		if (!Registry)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FStoryEventEntry& Entry : Registry->Entries)
		{
			if (Entry.ActionType == ActionType)
			{
				++Count;
			}
		}
		return Count;
	}

	bool IsEventConfigured(const UStoryEventRegistryDA* Registry, FGameplayTag EventTag)
	{
		return Registry && EventTag.IsValid() && Registry->FindEntry(EventTag) != nullptr;
	}
}

class SStoryRegistryTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryRegistryRow>>
{
public:
	SLATE_BEGIN_ARGS(SStoryRegistryTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryRegistryRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SStoryEventWorkbenchWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FStoryRegistryRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryRegistryRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UStoryEventRegistryDA* Registry = Item.IsValid() ? Item->Registry.Get() : nullptr;
		if (!Registry)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Registry")) return MakeTextCell(Registry->GetName(), Registry->GetPathName());
		if (ColumnName == TEXT("Entries")) return MakeTextCell(FString::FromInt(Registry->Entries.Num()));
		if (ColumnName == TEXT("Tutorial")) return MakeTextCell(FString::FromInt(CountActionType(Registry, EStoryEventActionType::TutorialPopup)));
		if (ColumnName == TEXT("Flow")) return MakeTextCell(FString::FromInt(CountActionType(Registry, EStoryEventActionType::LevelFlow)));
		if (ColumnName == TEXT("Broadcast")) return MakeTextCell(FString::FromInt(CountActionType(Registry, EStoryEventActionType::BroadcastOnly)));
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SButton)
				.Text(LOCTEXT("OpenRegistryAsset", "Open"))
				.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
				{
					if (TSharedPtr<SStoryEventWorkbenchWidget> Pinned = Owner.Pin())
					{
						Pinned->OpenRegistryAsset(Row);
					}
					return FReply::Handled();
				});
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryRegistryRow> Item;
	TWeakPtr<SStoryEventWorkbenchWidget> OwnerWidget;
};

class SStoryCampaignTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryCampaignRow>>
{
public:
	SLATE_BEGIN_ARGS(SStoryCampaignTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryCampaignRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SStoryEventWorkbenchWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FStoryCampaignRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryCampaignRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UCampaignDataAsset* Campaign = Item.IsValid() ? Item->Campaign.Get() : nullptr;
		if (!Campaign)
		{
			return MakeTextCell(TEXT("-"));
		}

		int32 EventTagCount = 0;
		for (const FFloorConfig& FloorConfig : Campaign->FloorTable)
		{
			EventTagCount += FloorConfig.StoryEventTags.Num();
		}

		if (ColumnName == TEXT("Campaign")) return MakeTextCell(Campaign->GetName(), Campaign->GetPathName());
		if (ColumnName == TEXT("Floors")) return MakeTextCell(FString::FromInt(Campaign->FloorTable.Num()));
		if (ColumnName == TEXT("Events")) return MakeTextCell(FString::FromInt(EventTagCount));
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SButton)
				.Text(LOCTEXT("OpenCampaignAsset", "Open"))
				.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
				{
					if (TSharedPtr<SStoryEventWorkbenchWidget> Pinned = Owner.Pin())
					{
						Pinned->OpenCampaignAsset(Row);
					}
					return FReply::Handled();
				});
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryCampaignRow> Item;
	TWeakPtr<SStoryEventWorkbenchWidget> OwnerWidget;
};

class SStoryStageEventTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryStageEventRow>>
{
public:
	SLATE_BEGIN_ARGS(SStoryStageEventTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryStageEventRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FStoryStageEventRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryStageEventRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Floor")) return MakeTextCell(Item->FloorIndex == INDEX_NONE ? TEXT("-") : FString::FromInt(Item->FloorIndex + 1));
		if (ColumnName == TEXT("Stage")) return MakeTextCell(Item->StageTag.IsValid() ? Item->StageTag.ToString() : TEXT("-"));
		if (ColumnName == TEXT("Event")) return MakeTextCell(Item->EventTag.IsValid() ? Item->EventTag.ToString() : TEXT("-"));
		if (ColumnName == TEXT("Configured")) return MakeTextCell(Item->bConfigured ? TEXT("Yes") : TEXT("Missing"));

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryStageEventRow> Item;
};

void SStoryEventWorkbenchWidget::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;

	RegistryDetailsView = PropertyModule.CreateDetailView(DetailsArgs);
	CampaignDetailsView = PropertyModule.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.OnClicked(this, &SStoryEventWorkbenchWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return StatusText; })
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.30f)
			[
				SNew(SBorder)
				.Padding(6.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("RegistriesTitle", "Story Event Registries"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(this, &SStoryEventWorkbenchWidget::GetRegistrySummaryText)
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(RegistryListView, SListView<FRegistryRowPtr>)
						.ListItemsSource(&RegistryRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SStoryEventWorkbenchWidget::GenerateRegistryRow)
						.OnSelectionChanged(this, &SStoryEventWorkbenchWidget::OnRegistrySelectionChanged)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Registry")).DefaultLabel(LOCTEXT("RegistryColumn", "Registry")).FillWidth(0.38f)
							+ SHeaderRow::Column(TEXT("Entries")).DefaultLabel(LOCTEXT("EntriesColumn", "E")).FixedWidth(30.f)
							+ SHeaderRow::Column(TEXT("Tutorial")).DefaultLabel(LOCTEXT("TutorialColumn", "T")).FixedWidth(30.f)
							+ SHeaderRow::Column(TEXT("Flow")).DefaultLabel(LOCTEXT("FlowColumn", "F")).FixedWidth(30.f)
							+ SHeaderRow::Column(TEXT("Broadcast")).DefaultLabel(LOCTEXT("BroadcastColumn", "B")).FixedWidth(30.f)
							+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("ActionsColumn", "")).FixedWidth(58.f)
						)
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.38f)
			[
				SNew(SBorder)
				.Padding(6.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("RegistryDetailsTitle", "Selected Registry Details"))
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						RegistryDetailsView.ToSharedRef()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.32f)
			[
				SNew(SBorder)
				.Padding(6.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("CampaignsTitle", "Campaign Stage Events"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(this, &SStoryEventWorkbenchWidget::GetCampaignSummaryText)
					]
					+ SVerticalBox::Slot().FillHeight(0.22f)
					[
						SAssignNew(CampaignListView, SListView<FCampaignRowPtr>)
						.ListItemsSource(&CampaignRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SStoryEventWorkbenchWidget::GenerateCampaignRow)
						.OnSelectionChanged(this, &SStoryEventWorkbenchWidget::OnCampaignSelectionChanged)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Campaign")).DefaultLabel(LOCTEXT("CampaignColumn", "Campaign")).FillWidth(0.55f)
							+ SHeaderRow::Column(TEXT("Floors")).DefaultLabel(LOCTEXT("FloorsColumn", "F")).FixedWidth(30.f)
							+ SHeaderRow::Column(TEXT("Events")).DefaultLabel(LOCTEXT("EventsColumn", "E")).FixedWidth(30.f)
							+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("CampaignActionsColumn", "")).FixedWidth(58.f)
						)
					]
					+ SVerticalBox::Slot().FillHeight(0.34f).Padding(0.f, 6.f, 0.f, 0.f)
					[
						CampaignDetailsView.ToSharedRef()
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 4.f)
					[
						SNew(STextBlock).Text(this, &SStoryEventWorkbenchWidget::GetStageEventSummaryText)
					]
					+ SVerticalBox::Slot().FillHeight(0.44f)
					[
						SAssignNew(StageEventListView, SListView<FStageEventRowPtr>)
						.ListItemsSource(&StageEventRows)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SStoryEventWorkbenchWidget::GenerateStageEventRow)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Floor")).DefaultLabel(LOCTEXT("FloorColumn", "Floor")).FixedWidth(42.f)
							+ SHeaderRow::Column(TEXT("Stage")).DefaultLabel(LOCTEXT("StageColumn", "Stage")).FillWidth(0.32f)
							+ SHeaderRow::Column(TEXT("Event")).DefaultLabel(LOCTEXT("EventColumn", "Event")).FillWidth(0.44f)
							+ SHeaderRow::Column(TEXT("Configured")).DefaultLabel(LOCTEXT("ConfiguredColumn", "Config")).FixedWidth(62.f)
						)
					]
				]
			]
		]
	];

	RefreshData();
}

void SStoryEventWorkbenchWidget::RefreshData(const FText& NewStatus)
{
	RegistryRows.Reset();
	for (UStoryEventRegistryDA* Registry : CollectAssetsOfClass<UStoryEventRegistryDA>())
	{
		RegistryRows.Add(MakeShared<FStoryRegistryRow>(Registry));
	}

	CampaignRows.Reset();
	for (UCampaignDataAsset* Campaign : CollectAssetsOfClass<UCampaignDataAsset>())
	{
		CampaignRows.Add(MakeShared<FStoryCampaignRow>(Campaign));
	}

	if (RegistryListView)
	{
		RegistryListView->RequestListRefresh();
	}
	if (CampaignListView)
	{
		CampaignListView->RequestListRefresh();
	}

	RebuildStageEventRows();

	StatusText = !NewStatus.IsEmpty()
		? NewStatus
		: FText::Format(
			LOCTEXT("RefreshStatus", "Loaded {0} story registries and {1} campaigns."),
			FText::AsNumber(RegistryRows.Num()),
			FText::AsNumber(CampaignRows.Num()));
}

void SStoryEventWorkbenchWidget::RebuildStageEventRows()
{
	StageEventRows.Reset();

	UCampaignDataAsset* Campaign = SelectedCampaignRow.IsValid() ? SelectedCampaignRow->Campaign.Get() : nullptr;
	UStoryEventRegistryDA* Registry = SelectedRegistryRow.IsValid() ? SelectedRegistryRow->Registry.Get() : nullptr;
	if (Campaign)
	{
		for (int32 FloorIndex = 0; FloorIndex < Campaign->FloorTable.Num(); ++FloorIndex)
		{
			const FFloorConfig& FloorConfig = Campaign->FloorTable[FloorIndex];
			TArray<FGameplayTag> EventTags;
			FloorConfig.StoryEventTags.GetGameplayTagArray(EventTags);
			for (const FGameplayTag& EventTag : EventTags)
			{
				FStageEventRowPtr Row = MakeShared<FStoryStageEventRow>();
				Row->Campaign = Campaign;
				Row->FloorIndex = FloorIndex;
				Row->StageTag = FloorConfig.GlobalStageTag;
				Row->EventTag = EventTag;
				Row->bConfigured = IsEventConfigured(Registry, EventTag);
				StageEventRows.Add(Row);
			}
		}
	}

	if (StageEventListView)
	{
		StageEventListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SStoryEventWorkbenchWidget::GenerateRegistryRow(FRegistryRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryRegistryTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(StaticCastSharedRef<SStoryEventWorkbenchWidget>(AsShared()));
}

TSharedRef<ITableRow> SStoryEventWorkbenchWidget::GenerateCampaignRow(FCampaignRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryCampaignTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(StaticCastSharedRef<SStoryEventWorkbenchWidget>(AsShared()));
}

TSharedRef<ITableRow> SStoryEventWorkbenchWidget::GenerateStageEventRow(FStageEventRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryStageEventTableRow, OwnerTable)
		.Item(Row);
}

void SStoryEventWorkbenchWidget::OnRegistrySelectionChanged(FRegistryRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedRegistryRow = Row;
	RegistryDetailsView->SetObject(Row.IsValid() ? Row->Registry.Get() : nullptr);
	RebuildStageEventRows();
}

void SStoryEventWorkbenchWidget::OnCampaignSelectionChanged(FCampaignRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedCampaignRow = Row;
	CampaignDetailsView->SetObject(Row.IsValid() ? Row->Campaign.Get() : nullptr);
	RebuildStageEventRows();
}

FReply SStoryEventWorkbenchWidget::OnRefreshClicked()
{
	RefreshData();
	return FReply::Handled();
}

void SStoryEventWorkbenchWidget::OpenRegistryAsset(TSharedPtr<FStoryRegistryRow> Row) const
{
	if (GEditor && Row.IsValid() && Row->Registry.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Row->Registry.Get());
	}
}

void SStoryEventWorkbenchWidget::OpenCampaignAsset(TSharedPtr<FStoryCampaignRow> Row) const
{
	if (GEditor && Row.IsValid() && Row->Campaign.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Row->Campaign.Get());
	}
}

FText SStoryEventWorkbenchWidget::GetRegistrySummaryText() const
{
	return FText::Format(LOCTEXT("RegistrySummary", "{0} StoryEventRegistry assets"), FText::AsNumber(RegistryRows.Num()));
}

FText SStoryEventWorkbenchWidget::GetCampaignSummaryText() const
{
	return FText::Format(LOCTEXT("CampaignSummary", "{0} Campaign assets"), FText::AsNumber(CampaignRows.Num()));
}

FText SStoryEventWorkbenchWidget::GetStageEventSummaryText() const
{
	int32 MissingCount = 0;
	for (const FStageEventRowPtr& Row : StageEventRows)
	{
		if (Row.IsValid() && !Row->bConfigured)
		{
			++MissingCount;
		}
	}

	return FText::Format(
		LOCTEXT("StageEventSummary", "Selected campaign uses {0} story event tags. Missing registry entries: {1}."),
		FText::AsNumber(StageEventRows.Num()),
		FText::AsNumber(MissingCount));
}

#undef LOCTEXT_NAMESPACE
