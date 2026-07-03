#pragma once

#include "CoreMinimal.h"
#include "Tools/MapCreator/DevKitMapCreatorService.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;

class SDevKitMapCreatorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitMapCreatorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FStringOption = TSharedPtr<FString>;

	TSharedRef<SWidget> BuildOptionCombo(
		TArray<FStringOption>& Options,
		FStringOption& SelectedOption,
		void (SDevKitMapCreatorWidget::*SelectionHandler)(FStringOption, ESelectInfo::Type),
		FText (SDevKitMapCreatorWidget::*TextGetter)() const,
		float Width);
	TSharedRef<SWidget> MakePathRow(const FText& Label, TAttribute<FText> Value) const;

	void RefreshPreview();
	void SetStatus(const FText& InStatus, bool bInIsError);
	FDevKitMapCreatorRequest BuildRequest() const;
	FText GetSelectedDungeonLayerText() const;
	FText GetSelectedLevelTypeText() const;
	FText GetBaseNameText() const;
	FText GetTargetFolderText() const;
	FText GetPersistentMapText() const;
	FText GetMapDefinitionText() const;
	FText GetSublevelText() const;
	FText GetTemplateText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	bool CanCreate() const;

	void OnDungeonLayerChanged(FStringOption NewSelection, ESelectInfo::Type SelectInfo);
	void OnLevelTypeChanged(FStringOption NewSelection, ESelectInfo::Type SelectInfo);
	void OnRootFolderChanged(const FText& NewText);
	void OnSuffixChanged(const FText& NewText);
	FReply UseContentBrowserFolder();
	FReply CreateMapPackage();

	TArray<FStringOption> DungeonLayerOptions;
	TArray<FStringOption> LevelTypeOptions;
	FStringOption SelectedDungeonLayer;
	FStringOption SelectedLevelType;

	TSharedPtr<SEditableTextBox> RootFolderTextBox;
	TSharedPtr<SEditableTextBox> SuffixTextBox;

	TOptional<FDevKitMapCreatorPaths> PreviewPaths;
	FText PreviewError;
	FText StatusText;
	bool bStatusIsError = false;
};
