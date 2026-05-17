#pragma once

#include "CoreMinimal.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;

struct FLevelWorkbenchRoomRow
{
	explicit FLevelWorkbenchRoomRow(URoomDataAsset* InRoom)
		: Room(InRoom)
	{
	}

	TWeakObjectPtr<URoomDataAsset> Room;
};

struct FLevelWorkbenchCampaignRow
{
	explicit FLevelWorkbenchCampaignRow(UCampaignDataAsset* InCampaign)
		: Campaign(InCampaign)
	{
	}

	TWeakObjectPtr<UCampaignDataAsset> Campaign;
};

class SLevelDataWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLevelDataWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenRoomAsset(TSharedPtr<FLevelWorkbenchRoomRow> Row) const;
	void OpenCampaignAsset(TSharedPtr<FLevelWorkbenchCampaignRow> Row) const;

private:
	using FRoomRowPtr = TSharedPtr<FLevelWorkbenchRoomRow>;
	using FCampaignRowPtr = TSharedPtr<FLevelWorkbenchCampaignRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	TSharedRef<ITableRow> GenerateRoomRow(FRoomRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateCampaignRow(FCampaignRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnRoomSelectionChanged(FRoomRowPtr Row, ESelectInfo::Type SelectInfo);
	void OnCampaignSelectionChanged(FCampaignRowPtr Row, ESelectInfo::Type SelectInfo);
	FReply OnRefreshClicked();

	FText GetRoomSummaryText() const;
	FText GetCampaignSummaryText() const;

	TArray<FRoomRowPtr> RoomRows;
	TArray<FCampaignRowPtr> CampaignRows;
	TSharedPtr<SListView<FRoomRowPtr>> RoomListView;
	TSharedPtr<SListView<FCampaignRowPtr>> CampaignListView;
	TSharedPtr<IDetailsView> RoomDetailsView;
	TSharedPtr<IDetailsView> CampaignDetailsView;
	FRoomRowPtr SelectedRoomRow;
	FCampaignRowPtr SelectedCampaignRow;
	FText StatusText;
};
