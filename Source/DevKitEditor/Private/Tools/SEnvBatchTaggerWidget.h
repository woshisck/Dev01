#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AActor;
class SEditableTextBox;
class STextBlock;

class SEnvBatchTaggerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnvBatchTaggerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TArray<AActor*> GetSelectedActors() const;
	FString GetGroupName() const;
	FReply ApplySourceTag();
	FReply ApplyProxyMidTag();
	FReply ApplyProxyLowTag();
	FReply ApplyBakedGroundMidTag();
	FReply ApplyBakedGroundLowTag();
	FReply ApplyBakedWallMidTag();
	FReply ApplyBakedWallLowTag();
	FReply ApplyExcludeTag();
	FReply RemoveEnvBatchTags();
	FReply RefreshSelection();
	FReply ApplyExclusiveTag(const FString& EnvBatchTag);
	void SetStatus(const FText& InStatus) const;
	FText GetSelectionSummaryText() const;
	FText GetAssetReadinessSummaryText() const;

	TSharedPtr<SEditableTextBox> GroupNameTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
};
