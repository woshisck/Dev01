#pragma once

#include "CoreMinimal.h"
#include "Tools/LevelBatch/DevKitLevelBatchService.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class ITableRow;
class STableViewBase;
class STextBlock;
template <typename ItemType> class SListView;

class SDevKitLevelBatchProcessorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitLevelBatchProcessorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FLevelBatchItemPtr = TSharedPtr<FDevKitLevelBatchStatus>;

	FReply RefreshLevels();
	FReply BatchCurrentLevel();
	FReply BatchSelectedLevels();
	FReply BatchAllLevels();
	FReply CleanSelectedLevels();
	FReply MarkSelectedArtReviewed();
	FReply MarkSelectedPerformanceApproved();
	FReply OpenSelectedBatchedAssetFolder();

	void RefreshLevelItems();
	void BatchItems(const TArray<FLevelBatchItemPtr>& Items);
	void MarkSelectedLevels(EDevKitLevelBatchReviewStatus Status);
	FString GetRootPackagePath() const;
	FString GetTierName() const;
	TArray<FLevelBatchItemPtr> GetSelectedItems() const;
	bool IsCurrentEditorMapLevel(const FDevKitLevelBatchStatus& Status) const;
	void SetStatus(const FText& InStatus) const;
	FText GetListSummaryText() const;
	TSharedRef<ITableRow> GenerateLevelRow(FLevelBatchItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const;

	TSharedPtr<SEditableTextBox> RootFolderTextBox;
	TSharedPtr<SEditableTextBox> TierTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SListView<FLevelBatchItemPtr>> LevelListView;
	TArray<FLevelBatchItemPtr> LevelItems;
};
