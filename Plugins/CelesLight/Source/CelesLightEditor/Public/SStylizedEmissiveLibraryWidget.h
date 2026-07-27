#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

struct FPropertyChangedEvent;
class IDetailsView;
class SSearchBox;
class SStylizedEmissivePreviewViewport;
class UActorFactoryStylizedEmissivePreset;
class UStylizedEmissiveModelLibrary;
class UStylizedEmissivePresetEditProxy;

struct FStylizedEmissivePresetListItem
{
	int32 EntryIndex = INDEX_NONE;
	FName PresetId = NAME_None;
	FText DisplayName;
	FString MeshName;
	bool bUsesMesh = false;
};

/** Artist-facing preset browser, preview, editor, and level-placement tool. */
class CELESLIGHTEDITOR_API SStylizedEmissiveLibraryWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStylizedEmissiveLibraryWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SelectPresetByIndex(int32 EntryIndex);
	FReply BeginPresetDrag(int32 EntryIndex);

private:
	using FPresetItemPtr = TSharedPtr<FStylizedEmissivePresetListItem>;

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildPresetPanel();
	TSharedRef<SWidget> BuildPreviewPanel();
	TSharedRef<SWidget> BuildSettingsPanel();
	TSharedRef<ITableRow> GeneratePresetRow(FPresetItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

	void RefreshPresetItems(FName PresetToSelect = NAME_None);
	void HandlePresetSelectionChanged(FPresetItemPtr Item, ESelectInfo::Type SelectInfo);
	void HandleSearchChanged(const FText& SearchText);
	void HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void ApplyProxyToSelectedEntry();
	void LoadSelectedEntryIntoProxy();
	void RefreshPreview();

	FReply CreatePreset();
	FReply DuplicatePreset();
	FReply DeletePreset();
	FReply SaveLibrary();
	FReply PlaceSelectedPreset();
	FReply ShowLibraryInContentBrowser();

	FName MakeUniquePresetId(const FString& BaseName) const;
	bool HasSelectedPreset() const;
	FText GetSelectedPresetTitle() const;
	FText GetSelectedPresetSummary() const;
	FText GetStatusText() const;
	EVisibility GetDataOnlyMessageVisibility() const;

	TWeakObjectPtr<UStylizedEmissiveModelLibrary> Library;
	/** Slate widgets are not GC objects, so keep transient helpers rooted as UObject. */
	TStrongObjectPtr<UObject> EditProxyRoot;
	TStrongObjectPtr<UObject> ActiveDragFactoryRoot;
	UStylizedEmissivePresetEditProxy* EditProxy = nullptr;

	TArray<FPresetItemPtr> PresetItems;
	TSharedPtr<SListView<FPresetItemPtr>> PresetListView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SStylizedEmissivePreviewViewport> PreviewViewport;

	int32 SelectedEntryIndex = INDEX_NONE;
	FString SearchFilter;
	FText StatusText;
};
