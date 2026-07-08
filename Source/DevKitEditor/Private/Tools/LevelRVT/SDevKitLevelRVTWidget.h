#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;

class SDevKitLevelRVTWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitLevelRVTWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString GetCurrentWorldPackagePath() const;
	FText GetSelectionSummaryText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	void RefreshDefaultPaths();
	FReply UseCurrentLevelPaths();
	FReply CreateGroundRVT();
	void SetStatus(const FText& InStatus, bool bInIsError);

	TSharedPtr<SEditableTextBox> BakeInfoFolderTextBox;
	TSharedPtr<SEditableTextBox> RVTNameTextBox;
	FText StatusText;
	bool bStatusIsError = false;
};
