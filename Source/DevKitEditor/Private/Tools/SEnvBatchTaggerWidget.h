#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class AActor;
class SEditableTextBox;
class STextBlock;

struct FEnvBatchSourceTagListItem
{
	FString Tag;
	int32 ActorCount = 0;
};

enum class EEnvBatchActorKind : uint8
{
	Prop,
	Building
};

enum class EEnvBatchProcessingMode : uint8
{
	Instance,
	Batched
};

class SEnvBatchTaggerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnvBatchTaggerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FSourceTagItemPtr = TSharedPtr<FEnvBatchSourceTagListItem>;

	TArray<AActor*> GetSelectedActors() const;
	TArray<AActor*> GetEditorWorldActors() const;
	FString GetCurrentLevelName() const;
	FString GetLevelName() const;
	FString GetVTCGroup() const;
	int32 GetSerialNumber() const;
	FString BuildSourceTag() const;
	FString BuildSourceTagPrefix() const;
	FText GetTagPreviewText() const;
	FText GetSourceTagListSummaryText() const;
	EVisibility GetEmptySourceTagHintVisibility() const;
	FReply ApplySourceTag();
	FReply RemoveSourceTags();
	FReply RefreshSourceTags();
	FReply SelectSelectedSourceTagActors();
	FReply UseSelectedSourceTagAsTemplate();
	FReply UseCurrentLevelName();
	FReply UseNextSerialNumber();
	void RefreshSourceTagList();
	void SelectActorsWithSourceTag(const FString& SourceTag) const;
	void ApplySourceTagToControls(const FString& SourceTag);
	void SetActorKind(ECheckBoxState NewState, EEnvBatchActorKind InActorKind);
	void SetProcessingMode(ECheckBoxState NewState, EEnvBatchProcessingMode InProcessingMode);
	ECheckBoxState IsActorKindChecked(EEnvBatchActorKind InActorKind) const;
	ECheckBoxState IsProcessingModeChecked(EEnvBatchProcessingMode InProcessingMode) const;
	TSharedRef<ITableRow> GenerateSourceTagRow(FSourceTagItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnSourceTagSelectionChanged(FSourceTagItemPtr Item, ESelectInfo::Type SelectInfo);
	void OnSourceTagDoubleClicked(FSourceTagItemPtr Item);
	void SetStatus(const FText& InStatus) const;
	FText GetSelectionSummaryText() const;
	FText GetAssetReadinessSummaryText() const;

	TSharedPtr<SEditableTextBox> LevelNameTextBox;
	TSharedPtr<SEditableTextBox> VTCGroupTextBox;
	TSharedPtr<SEditableTextBox> SerialNumberTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SListView<FSourceTagItemPtr>> SourceTagListView;
	TArray<FSourceTagItemPtr> SourceTagItems;
	FSourceTagItemPtr SelectedSourceTagItem;
	int32 LastScannedActorCount = 0;
	EEnvBatchActorKind ActorKind = EEnvBatchActorKind::Prop;
	EEnvBatchProcessingMode ProcessingMode = EEnvBatchProcessingMode::Batched;
};
