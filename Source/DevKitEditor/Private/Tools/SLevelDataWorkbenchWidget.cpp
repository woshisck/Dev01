#include "Tools/SLevelDataWorkbenchWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "Map/AltarActor.h"
#include "Map/ShopActor.h"
#include "PropertyEditorModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SLevelDataWorkbenchWidget"

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

	FString RoomTypeText(const URoomDataAsset* Room)
	{
		if (!Room)
		{
			return TEXT("-");
		}
		if (Room->bIsHubRoom)
		{
			return TEXT("Hub");
		}
		if (Room->ShopData || Room->ShopActorClass.Get())
		{
			return TEXT("Shop");
		}
		if (Room->SacrificeEventAltarData || Room->SacrificeEventAltarClass.Get() || Room->bEnableTimedClearObjective)
		{
			return TEXT("Event");
		}
		if (Room->RoomTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Room.Type.Elite"), false)))
		{
			return TEXT("Elite");
		}
		return TEXT("Normal");
	}
}

class SLevelWorkbenchRoomTableRow : public SMultiColumnTableRow<TSharedPtr<FLevelWorkbenchRoomRow>>
{
public:
	SLATE_BEGIN_ARGS(SLevelWorkbenchRoomTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FLevelWorkbenchRoomRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SLevelDataWorkbenchWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FLevelWorkbenchRoomRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FLevelWorkbenchRoomRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		URoomDataAsset* Room = Item.IsValid() ? Item->Room.Get() : nullptr;
		if (!Room)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Room")) return MakeTextCell(Room->GetName(), Room->GetPathName());
		if (ColumnName == TEXT("Display")) return MakeTextCell(Room->DisplayName.ToString());
		if (ColumnName == TEXT("Type")) return MakeTextCell(RoomTypeText(Room));
		if (ColumnName == TEXT("Enemies")) return MakeTextCell(FString::FromInt(Room->EnemyPool.Num()));
		if (ColumnName == TEXT("Loot")) return MakeTextCell(FString::FromInt(Room->LootPool.Num()));
		if (ColumnName == TEXT("Portals")) return MakeTextCell(FString::FromInt(Room->PortalDestinations.Num()));
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SButton)
				.Text(LOCTEXT("OpenRoomAsset", "Open"))
				.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
				{
					if (TSharedPtr<SLevelDataWorkbenchWidget> Pinned = Owner.Pin())
					{
						Pinned->OpenRoomAsset(Row);
					}
					return FReply::Handled();
				});
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FLevelWorkbenchRoomRow> Item;
	TWeakPtr<SLevelDataWorkbenchWidget> OwnerWidget;
};

class SLevelWorkbenchCampaignTableRow : public SMultiColumnTableRow<TSharedPtr<FLevelWorkbenchCampaignRow>>
{
public:
	SLATE_BEGIN_ARGS(SLevelWorkbenchCampaignTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FLevelWorkbenchCampaignRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SLevelDataWorkbenchWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FLevelWorkbenchCampaignRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FLevelWorkbenchCampaignRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UCampaignDataAsset* Campaign = Item.IsValid() ? Item->Campaign.Get() : nullptr;
		if (!Campaign)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Campaign")) return MakeTextCell(Campaign->GetName(), Campaign->GetPathName());
		if (ColumnName == TEXT("Floors")) return MakeTextCell(FString::FromInt(Campaign->FloorTable.Num()));
		if (ColumnName == TEXT("Rooms")) return MakeTextCell(FString::FromInt(Campaign->RoomPool.Num()));
		if (ColumnName == TEXT("Layer")) return MakeTextCell(Campaign->LayerTag.IsValid() ? Campaign->LayerTag.ToString() : TEXT("-"));
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SButton)
				.Text(LOCTEXT("OpenCampaignAsset", "Open"))
				.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
				{
					if (TSharedPtr<SLevelDataWorkbenchWidget> Pinned = Owner.Pin())
					{
						Pinned->OpenCampaignAsset(Row);
					}
					return FReply::Handled();
				});
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FLevelWorkbenchCampaignRow> Item;
	TWeakPtr<SLevelDataWorkbenchWidget> OwnerWidget;
};

void SLevelDataWorkbenchWidget::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;

	RoomDetailsView = PropertyModule.CreateDetailView(DetailsArgs);
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
				.OnClicked(this, &SLevelDataWorkbenchWidget::OnRefreshClicked)
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
						SNew(STextBlock).Text(LOCTEXT("RoomsTitle", "Rooms"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(this, &SLevelDataWorkbenchWidget::GetRoomSummaryText)
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(RoomListView, SListView<FRoomRowPtr>)
						.ListItemsSource(&RoomRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SLevelDataWorkbenchWidget::GenerateRoomRow)
						.OnSelectionChanged(this, &SLevelDataWorkbenchWidget::OnRoomSelectionChanged)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Room")).DefaultLabel(LOCTEXT("RoomColumn", "Room")).FillWidth(0.30f)
							+ SHeaderRow::Column(TEXT("Display")).DefaultLabel(LOCTEXT("DisplayColumn", "Display")).FillWidth(0.22f)
							+ SHeaderRow::Column(TEXT("Type")).DefaultLabel(LOCTEXT("TypeColumn", "Type")).FillWidth(0.13f)
							+ SHeaderRow::Column(TEXT("Enemies")).DefaultLabel(LOCTEXT("EnemiesColumn", "E")).FixedWidth(34.f)
							+ SHeaderRow::Column(TEXT("Loot")).DefaultLabel(LOCTEXT("LootColumn", "L")).FixedWidth(34.f)
							+ SHeaderRow::Column(TEXT("Portals")).DefaultLabel(LOCTEXT("PortalsColumn", "P")).FixedWidth(34.f)
							+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("ActionsColumn", "")).FixedWidth(58.f)
						)
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.42f)
			[
				SNew(SBorder)
				.Padding(6.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("RoomDetailsTitle", "Selected Room Details"))
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						RoomDetailsView.ToSharedRef()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.28f)
			[
				SNew(SBorder)
				.Padding(6.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("CampaignsTitle", "Global Campaign Flow"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(this, &SLevelDataWorkbenchWidget::GetCampaignSummaryText)
					]
					+ SVerticalBox::Slot().FillHeight(0.38f)
					[
						SAssignNew(CampaignListView, SListView<FCampaignRowPtr>)
						.ListItemsSource(&CampaignRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SLevelDataWorkbenchWidget::GenerateCampaignRow)
						.OnSelectionChanged(this, &SLevelDataWorkbenchWidget::OnCampaignSelectionChanged)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Campaign")).DefaultLabel(LOCTEXT("CampaignColumn", "Campaign")).FillWidth(0.45f)
							+ SHeaderRow::Column(TEXT("Floors")).DefaultLabel(LOCTEXT("FloorsColumn", "F")).FixedWidth(32.f)
							+ SHeaderRow::Column(TEXT("Rooms")).DefaultLabel(LOCTEXT("RoomsColumn", "R")).FixedWidth(32.f)
							+ SHeaderRow::Column(TEXT("Layer")).DefaultLabel(LOCTEXT("LayerColumn", "Layer")).FillWidth(0.28f)
							+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("CampaignActionsColumn", "")).FixedWidth(58.f)
						)
					]
					+ SVerticalBox::Slot().FillHeight(0.62f).Padding(0.f, 6.f, 0.f, 0.f)
					[
						CampaignDetailsView.ToSharedRef()
					]
				]
			]
		]
	];

	RefreshData();
}

void SLevelDataWorkbenchWidget::RefreshData(const FText& NewStatus)
{
	RoomRows.Reset();
	for (URoomDataAsset* Room : CollectAssetsOfClass<URoomDataAsset>())
	{
		RoomRows.Add(MakeShared<FLevelWorkbenchRoomRow>(Room));
	}

	CampaignRows.Reset();
	for (UCampaignDataAsset* Campaign : CollectAssetsOfClass<UCampaignDataAsset>())
	{
		CampaignRows.Add(MakeShared<FLevelWorkbenchCampaignRow>(Campaign));
	}

	if (RoomListView)
	{
		RoomListView->RequestListRefresh();
	}
	if (CampaignListView)
	{
		CampaignListView->RequestListRefresh();
	}

	StatusText = !NewStatus.IsEmpty()
		? NewStatus
		: FText::Format(
			LOCTEXT("RefreshStatus", "Loaded {0} rooms and {1} campaigns."),
			FText::AsNumber(RoomRows.Num()),
			FText::AsNumber(CampaignRows.Num()));
}

TSharedRef<ITableRow> SLevelDataWorkbenchWidget::GenerateRoomRow(FRoomRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLevelWorkbenchRoomTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(StaticCastSharedRef<SLevelDataWorkbenchWidget>(AsShared()));
}

TSharedRef<ITableRow> SLevelDataWorkbenchWidget::GenerateCampaignRow(FCampaignRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLevelWorkbenchCampaignTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(StaticCastSharedRef<SLevelDataWorkbenchWidget>(AsShared()));
}

void SLevelDataWorkbenchWidget::OnRoomSelectionChanged(FRoomRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedRoomRow = Row;
	RoomDetailsView->SetObject(Row.IsValid() ? Row->Room.Get() : nullptr);
}

void SLevelDataWorkbenchWidget::OnCampaignSelectionChanged(FCampaignRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedCampaignRow = Row;
	CampaignDetailsView->SetObject(Row.IsValid() ? Row->Campaign.Get() : nullptr);
}

FReply SLevelDataWorkbenchWidget::OnRefreshClicked()
{
	RefreshData();
	return FReply::Handled();
}

void SLevelDataWorkbenchWidget::OpenRoomAsset(TSharedPtr<FLevelWorkbenchRoomRow> Row) const
{
	if (GEditor && Row.IsValid() && Row->Room.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Row->Room.Get());
	}
}

void SLevelDataWorkbenchWidget::OpenCampaignAsset(TSharedPtr<FLevelWorkbenchCampaignRow> Row) const
{
	if (GEditor && Row.IsValid() && Row->Campaign.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Row->Campaign.Get());
	}
}

FText SLevelDataWorkbenchWidget::GetRoomSummaryText() const
{
	return FText::Format(LOCTEXT("RoomSummary", "{0} RoomData assets"), FText::AsNumber(RoomRows.Num()));
}

FText SLevelDataWorkbenchWidget::GetCampaignSummaryText() const
{
	return FText::Format(LOCTEXT("CampaignSummary", "{0} Campaign assets"), FText::AsNumber(CampaignRows.Num()));
}

#undef LOCTEXT_NAMESPACE
