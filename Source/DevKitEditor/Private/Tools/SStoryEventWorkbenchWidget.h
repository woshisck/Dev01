#pragma once

#include "CoreMinimal.h"
#include "Data/CampaignDataAsset.h"
#include "Story/StoryEventRegistryDA.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;

struct FStoryRegistryRow
{
	explicit FStoryRegistryRow(UStoryEventRegistryDA* InRegistry)
		: Registry(InRegistry)
	{
	}

	TWeakObjectPtr<UStoryEventRegistryDA> Registry;
};

struct FStoryCampaignRow
{
	explicit FStoryCampaignRow(UCampaignDataAsset* InCampaign)
		: Campaign(InCampaign)
	{
	}

	TWeakObjectPtr<UCampaignDataAsset> Campaign;
};

struct FStoryStageEventRow
{
	TWeakObjectPtr<UCampaignDataAsset> Campaign;
	int32 FloorIndex = INDEX_NONE;
	FGameplayTag StageTag;
	FGameplayTag EventTag;
	bool bConfigured = false;
};

class SStoryEventWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStoryEventWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenRegistryAsset(TSharedPtr<FStoryRegistryRow> Row) const;
	void OpenCampaignAsset(TSharedPtr<FStoryCampaignRow> Row) const;

private:
	using FRegistryRowPtr = TSharedPtr<FStoryRegistryRow>;
	using FCampaignRowPtr = TSharedPtr<FStoryCampaignRow>;
	using FStageEventRowPtr = TSharedPtr<FStoryStageEventRow>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RebuildStageEventRows();

	TSharedRef<ITableRow> GenerateRegistryRow(FRegistryRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateCampaignRow(FCampaignRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateStageEventRow(FStageEventRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	void OnRegistrySelectionChanged(FRegistryRowPtr Row, ESelectInfo::Type SelectInfo);
	void OnCampaignSelectionChanged(FCampaignRowPtr Row, ESelectInfo::Type SelectInfo);
	FReply OnRefreshClicked();

	FText GetRegistrySummaryText() const;
	FText GetCampaignSummaryText() const;
	FText GetStageEventSummaryText() const;

	TArray<FRegistryRowPtr> RegistryRows;
	TArray<FCampaignRowPtr> CampaignRows;
	TArray<FStageEventRowPtr> StageEventRows;

	TSharedPtr<SListView<FRegistryRowPtr>> RegistryListView;
	TSharedPtr<SListView<FCampaignRowPtr>> CampaignListView;
	TSharedPtr<SListView<FStageEventRowPtr>> StageEventListView;
	TSharedPtr<IDetailsView> RegistryDetailsView;
	TSharedPtr<IDetailsView> CampaignDetailsView;

	FRegistryRowPtr SelectedRegistryRow;
	FCampaignRowPtr SelectedCampaignRow;
	FText StatusText;
};
