#pragma once

#include "CoreMinimal.h"
#include "Tools/LevelRVT/DevKitLevelRVTService.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class AActor;
class SEditableTextBox;
class STextBlock;

struct FDevKitLevelRVTSourceTagListItem
{
	FString Tag;
	int32 ActorCount = 0;
	int32 PrimitiveComponentCount = 0;
	int32 BoundComponentCount = 0;
	int32 RuntimeVirtualTextureReferenceCount = 0;
};

class SDevKitLevelRVTWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitLevelRVTWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FSourceTagItemPtr = TSharedPtr<FDevKitLevelRVTSourceTagListItem>;

	FString GetCurrentWorldPackagePath() const;
	FString GetCurrentSourceTag() const;
	FString GetDefaultGroundSourceTag() const;
	FDevKitLevelRVTRequest BuildRequest() const;
	TSharedRef<SWidget> BuildStepHeader(int32 StepNumber, const FText& Title);
	bool IsStep1Complete() const;
	bool IsStep2Complete() const;
	bool CanAddSelectedRVT() const;
	FText GetStepStatusText(int32 StepNumber) const;
	FSlateColor GetStepStatusColor(int32 StepNumber) const;
	FSlateColor GetStepHeaderTint(int32 StepNumber) const;
	FText GetSelectionSummaryText() const;
	FText GetSourceTagListSummaryText() const;
	FText GetActiveBatchSummaryText() const;
	FText GetSelectedLayoutSummaryText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	void RefreshDefaultPaths();
	void RefreshSourceTagList();
	void RefreshActiveBatchStats();
	void RefreshRVTAssetState();
	void SelectActorsWithSourceTag(const FString& SourceTag);
	TArray<EDevKitLevelRVTLayout> GetSelectedLayouts() const;
	void SetLayoutEnabled(ECheckBoxState NewState, EDevKitLevelRVTLayout Layout);
	ECheckBoxState IsLayoutEnabled(EDevKitLevelRVTLayout Layout) const;
	FReply UseCurrentLevelPaths();
	FReply RefreshSourceTags();
	FReply AssignSourceTagToSelection();
	FReply RemoveSourceTagFromSelection();
	FReply SelectSelectedSourceTagActors();
	FReply CreateGroundRVTAssetsAndVolumes();
	FReply AddGroundRVTBindings();
	FReply RemoveGroundRVTBindings();
	FReply ClearGroundRVTBindings();
	TSharedRef<ITableRow> GenerateSourceTagRow(FSourceTagItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnSourceTagSelectionChanged(FSourceTagItemPtr Item, ESelectInfo::Type SelectInfo);
	void OnSourceTagTextCommitted(const FText& Text, ETextCommit::Type CommitType);
	void OnRVTSettingsTextCommitted(const FText& Text, ETextCommit::Type CommitType);
	void SetStatus(const FText& InStatus, bool bInIsError);

	TSharedPtr<SEditableTextBox> BakeInfoFolderTextBox;
	TSharedPtr<SEditableTextBox> RVTNameTextBox;
	TSharedPtr<SEditableTextBox> SourceTagTextBox;
	TSharedPtr<SListView<FSourceTagItemPtr>> SourceTagListView;
	TArray<FSourceTagItemPtr> SourceTagItems;
	FSourceTagItemPtr SelectedSourceTagItem;
	FDevKitLevelRVTBatchStats ActiveBatchStats;
	FDevKitLevelRVTAssetState ActiveRVTAssetState;
	int32 LastScannedActorCount = 0;
	bool bBaseColorNormalRoughness = true;
	bool bMask4 = false;
	bool bBaseColor = false;
	bool bBaseColorNormalSpecular = false;
	bool bYCoCgBaseColorNormalSpecular = false;
	bool bYCoCgBaseColorNormalSpecularMask = false;
	bool bWorldHeight = false;
	bool bDisplacement = false;
	FText StatusText;
	bool bStatusIsError = false;
};
