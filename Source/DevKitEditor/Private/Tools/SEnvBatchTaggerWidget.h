#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;
class AActor;

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
	FReply ApplyProxyMediumTag();
	FReply ApplyProxyLowTag();
	FReply ApplyBakedGroundLowTag();
	FReply ApplyExcludeTag();
	FReply RemoveEnvBatchTags();
	FReply RefreshSelection();
	FReply ApplyExclusiveTag(const FString& EnvBatchTag);
	void SetStatus(const FText& InStatus) const;
	FText GetSelectionSummaryText() const;

	TSharedPtr<SEditableTextBox> GroupNameTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
};
