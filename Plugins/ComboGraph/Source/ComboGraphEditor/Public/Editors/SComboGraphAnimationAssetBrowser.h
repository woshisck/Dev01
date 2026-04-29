// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "PersonaDelegates.h"
#include "IAnimationSequenceBrowser.h"

class IComboGraphBlueprintEditor;
class FComboGraphBlueprintEditor;

class FComboGraphAnimationAssetViewportClient : public FEditorViewportClient
{
public:
	FComboGraphAnimationAssetViewportClient(FPreviewScene& InPreviewScene);

	// FEditorViewportClient interface
	virtual FSceneInterface* GetScene() const override;
	virtual FLinearColor GetBackgroundColor() const override;
	// End of FEditorViewportClient interface
};

class SComboGraphAnimationAssetBrowser : public IAnimationSequenceBrowser
{
public:
	SLATE_BEGIN_ARGS(SComboGraphAnimationAssetBrowser)
		: _ShowHistory(false)
	{}

	SLATE_ARGUMENT(bool, ShowHistory)
	SLATE_ARGUMENT(FOnOpenNewAsset, OnOpenNewAsset)

	SLATE_END_ARGS()

	/** The section of EditorPerProjectUserSettings in which to save settings */
	static const FString SettingsIniSection;

	virtual ~SComboGraphAnimationAssetBrowser();

	/** Constructs this widget with InArgs */
	// void Construct(const FArguments& InArgs, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, const TSharedRef<FAssetEditorToolkit>& InHostingApp);
	void Construct(const FArguments& InArgs, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, const TSharedRef<IComboGraphBlueprintEditor>& InBlueprintEditor);

	/** Delegate that handles anim asset double clicked */
	void OnRequestOpenAsset(const FAssetData& AssetData, bool bFromHistory);

	/** Delegate that handles creation of context menu */
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);

	/** IAnimationSequenceBrowser interface */
	virtual void SelectAsset(UAnimationAsset * AnimAsset) override;
	virtual void AddToHistory(UAnimationAsset* AnimAsset) override;
	virtual void FilterBySkeletonNotify(const FName& InNotifyName) override;

	/** Delegate to handle "Find in Content Browser" context menu option */
	void FindInContentBrowser();

	/** Delegate to handle enabling the "Find in Content Browser" context menu option */
	bool CanFindInContentBrowser() const;

	/** Delegate to handle "Save" context menu option */
	void SaveSelectedAssets(TArray<FAssetData> ObjectsToSave) const;

	/** Delegate to handle enabling the "Save" context menu option */
	bool CanSaveSelectedAssets(TArray<FAssetData> ObjectsToSave) const;

	/** Delegate to handle changes for preview mesh in hosting app */
	void HandlePreviewMeshChanged(USkeletalMesh* InPreviewMesh);

	/** Force refresh of Asset Browser (for instance, in case of preview mesh changes) */
	void RefreshAssetView();

protected:
	/** The persona toolkit we are using */
	TWeakPtr<IPersonaToolkit> PersonaToolkitPtr;

	/** Hosting App we are using (should be Blueprint Editor) */
	TWeakPtr<IComboGraphBlueprintEditor> BlueprintEditorPtr;

	/** Delegate called to open a new asset for editing */
	FOnOpenNewAsset OnOpenNewAsset;

	/**
	 * Commands handled by this widget
	 */
	TSharedPtr<FUICommandList> Commands;

	// delegate to sync the asset picker to selected assets
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	FSyncToAssetsDelegate SyncToAssetsDelegate;

	/** Whether to show the history widgets */
	bool bShowHistory = false;

	/** Track if we have tried to cache the first asset we were playing */
	bool bTriedToCacheOriginalAsset = false;

	/** Max assets to save in history */
	static const int32 MaxAssetsHistory;

	/** Current position in the asset history */
	int32 CurrentAssetHistoryIndex = 0;

	/** Set of tags to prevent creating details view columns for (infrequently used) */
	TSet<FName> AssetRegistryTagsToIgnore;

	/** List of recently opened assets */
	TArray<FAssetData> AssetHistory;

	/** Delegate used to set the AR filter after the fact */
	FSetARFilterDelegate SetFilterDelegate;

	/** Delegate used to refresh the Asset Browser after the fact */
	FRefreshAssetViewDelegate RefreshAssetPickerAssetViewDelegate;

	/** Keep the AR filter around so we can modify it */
	FARFilter Filter;

	/**
	 * The actual viewport widget
	 */
	TSharedPtr<SViewport> ViewportWidget;

	/**
	 * The scene viewport data
	 */
	TSharedPtr<FSceneViewport> SceneViewport;

	/**
	 * Custom viewport client used for tooltip previews
	 */
	TSharedPtr<FComboGraphAnimationAssetViewportClient> ViewportClient;

	/**
	 * Skeletal component to preview the animation asset on
	 */
	UDebugSkelMeshComponent* PreviewComponent = nullptr;

	/**
	 * The scene to show in the asset previews
	 */
	FPreviewScene PreviewScene;

	/** Populate supplied OutPackages with the packages for the supplied Assets array */
	void GetSelectedPackages(const TArray<FAssetData>& Assets, TArray<UPackage*>& OutPackages) const;

	/**
	 * Create the viewport and required objects to control the viewport used in tooltips
	 */
	void CreateAssetTooltipResources();

	/** Adds the supplied asset to the asset history */
	void AddAssetToHistory(const FAssetData& AssetData);

	void CacheOriginalAnimAssetHistory();

	/**
	 * Has the animation asset preview tool tip been visualized
	 */
	bool IsToolTipPreviewVisible();

	bool CanShowColumnForAssetRegistryTag(FName AssetType, FName TagName) const;

	/** Perform additional filtering */
	bool HandleFilterAsset(const FAssetData& InAssetData) const;

	/**
	 * Create a custom asset view tooltip for the provided asset
	 */
	TSharedRef<SToolTip> CreateCustomAssetToolTip(FAssetData& AssetData);

	/**
	 * Called as a tooltip is about to show;
	 */
	bool OnVisualizeAssetToolTip(const TSharedPtr<SWidget>& TooltipContent, FAssetData& AssetData);

	/**
	* Called as a tooltip is closing
	*/
	void OnAssetToolTipClosing();

	/** Returns TRUE if stepping backward in history is allowed */
	bool CanStepBackwardInHistory() const;

	/** Returns TRUE if stepping forward in history is allowed */
	bool CanStepForwardInHistory() const;

	/**
	 * Mouse down callback to display a history menu
	 *
	 * @param InMenuAnchor		This is the anchor the menu will use for positioning
	 */
	FReply OnMouseDownHistory(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TWeakPtr<SMenuAnchor> InMenuAnchor);

	/**
	 * Callback to create the history menu.
	 *
	 * @param bInBackHistory		TRUE if the back history is requested, FALSE if the forward history is
	 *
	 * @return						The menu widget displaying all available history
	 */
	TSharedRef<SWidget> CreateHistoryMenu(bool bInBackHistory) const;

	/** Jumps immediately to an index in the history if valid */
	void GoToHistoryIndex(int32 InHistoryIdx);

	/** Single step forward in history */
	FReply OnGoForwardInHistory();

	/** Single step back in history */
	FReply OnGoBackInHistory();

	/** Single step back in history */
	FReply OnRefreshRequested();

	/** Returns visible when not in a Blueprint mode (anim mode, etc...) */
	EVisibility GetHistoryVisibility() const;

	/**
	 * Cleanup a component from the preview scene along with any attached children
	 */
	void CleanupPreviewSceneComponent(USceneComponent* Component);

	/** Returns the text to render when no assets are shown */
	FText GetAssetShowWarningText() const;

private:
	/** Whether the active timer is currently registered */
	bool bIsActiveTimerRegistered = true;

	/** Updates the animation preview in the tooltip */
	EActiveTimerReturnType UpdateTooltipPreview(double InCurrentTime, float InDeltaTime);
};
