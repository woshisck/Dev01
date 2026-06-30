#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;

class SMaterialBatchToolsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialBatchToolsWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply RunDryRunCommandlet();
	FReply CopyDryRunCommand();
	FReply CopyPartialApplyCommand();
	FReply OpenReportFolder();
	FString BuildCommand(bool bPartialApply, bool bForPowerShell) const;
	FString BuildCommandletArgs(bool bPartialApply) const;
	FString GetTextOrDefault(const TSharedPtr<SEditableTextBox>& TextBox, const FString& DefaultValue) const;
	void SetStatus(const FText& InStatus) const;

	TSharedPtr<SEditableTextBox> MapTextBox;
	TSharedPtr<SEditableTextBox> ClusterTextBox;
	TSharedPtr<SEditableTextBox> TierTextBox;
	TSharedPtr<SEditableTextBox> RequireTagTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
};
