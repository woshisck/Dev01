// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphEditorTypes.h"
#include "Editors/IComboGraphBlueprintEditor.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class UComboGraphAbilityTask_StartGraph;
class FComboGraphDebugger;
class FComboGraphDefaultAppMode;
class FComboGraphPersonaAppMode;
class UComboGraphEdGraph;
class IPersonaPreviewScene;
class SComboGraphAnimationAssetBrowser;
class UAnimPreviewInstance;
class IPersonaToolkit;
class FComboGraphEditorToolbar;
class UComboGraph;

class COMBOGRAPHEDITOR_API FComboGraphAssetEditor : public FWorkflowCentricApplication, public FNotifyHook, public FGCObject, public IComboGraphBlueprintEditor
{
public:
	FComboGraphAssetEditor();
	virtual ~FComboGraphAssetEditor();

	/**
	 * Edits the specified gameplay ability asset(s) with a specialized combat interface
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	InComboGraph				The graph model to edit
	 */
	void InitComboGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UComboGraph* InComboGraph);

	/** Creates and store PersonaToolkit and bind delegates */
	void CreatePersonaToolkit();

	/**
	 * Does the heavy initialization work for a new combo graph asset, with common initialization
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 */
	void SetupComboGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost);

	//~ Begin IComboGraphBlueprintEditor Interface
	virtual USkeleton* GetSkeleton() const override;
	virtual USkeletalMesh* GetPreviewMesh() const override;
	virtual void SetPreviewMesh(USkeletalMesh* InSkeletalMesh, bool bSetPreviewMeshInAsset) override;
	//~ End IComboGraphBlueprintEditor Interface

    TSharedPtr<FComboGraphEditorToolbar> GetToolbarBuilder() const { return ToolbarBuilder; }

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	//~ End of IToolkit Interface

	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	//~ End FGCObject Interface

	/** @return the documentation location for this editor */
	virtual FString GetDocumentationLink() const override;

	/** Get the persona toolkit */
	TSharedRef<IPersonaToolkit> GetPersonaToolkit() const { return PersonaToolkit.ToSharedRef(); }

	/** Get the Graph Editor Widget */
	TSharedPtr<SGraphEditor> GetGraphEditorWidget() const { return GraphEditorWidget; }
	TSharedPtr<IDetailsView> GetPropertyDetailsWidget() const { return PropertyDetailsWidget; }

	/** Handle opening a new asset from the asset browser */
	void HandleOpenNewAsset(UObject* InNewAsset);

	/** Handle creation of Animation Browser widget so that we can react and communicate with it (mainly to refresh asset browser on preview mesh changes) */
	void HandleOnAnimationBrowserCreated(const TSharedRef<class IAnimationSequenceBrowser>& InWidget);

	/** Controls whether we can switch to specific app modes */
    bool CanAccessComboGraphMode() const;

	// Engine @todo This is a hack for now until we reconcile the default toolbar with application modes [duplicated from counterpart in Blueprint Editor]
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager);

	/** Called from Editor debugger and rebuilds exec graph for the Combo Graph Asset, and indicates if debugger is in active state (eg. PIE running) */
	void DebuggerUpdateGraph(bool bIsPIEActive);

	/** Indicates to outside code that debugger is currently relevant (eg. isPieReady, PIE is currently playing) */
	bool IsDebuggerReady() const;

	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	void OnDebuggerActorSelected(TWeakObjectPtr<AActor> ActorToDebug);
	FText GetDebuggerActorDesc() const;

	AActor* GetDebuggerSelectedActor() const;
	UComboGraphAbilityTask_StartGraph* GetDebuggerSelectedTask() const;

	EComboGraphAutoArrangeStrategy GetLastAutoArrangeStrategy() const { return LastAutoArrangeStrategy; }
	bool IsVerticalPositioning() const { return LastAutoArrangeStrategy == EComboGraphAutoArrangeStrategy::Vertical; }

	/** Store last selected mode before letting Workflow editor handles the rest */
	virtual void SetCurrentMode(FName NewMode) override;

protected:

	/** The Combo Graph model this editor handles */
	TObjectPtr<UComboGraph> ComboGraphBeingEdited;

	/** The Combo Editor Graph created part of this asset editor */
	// TWeakObjectPtr<UEdGraph> ComboEditorGraph;

	/** Persona toolkit used to support skeletal mesh preview */
	TSharedPtr<IPersonaToolkit> PersonaToolkit;

	/** preview animation instance */
	UAnimPreviewInstance* PreviewInstance;

	/** Anim asset browser cached here so that we can request refresh on preview mesh changes */
	SComboGraphAnimationAssetBrowser* AnimAssetBrowser;

	/** The command list for this asset editor with common functionality such as copy paste and so on */
	TSharedPtr<FUICommandList> DefaultCommands;

	/** Creates ComboGraphBeingEdited EdGraph and attaches it to ComboGraphBeingEdited Object */
	void CreateEditorGraph();

	//~ FBlueprintEditor interface
	// virtual void OnSelectedNodesChangedImpl(const TSet<UObject*>& NewSelection) override;
	// virtual FGraphAppearanceInfo GetGraphAppearance(UEdGraph* InGraph) const override;
	//~ End FBlueprintEditor interface

	//~ FAssetEditorToolkit interface
	virtual void SaveAsset_Execute() override;
	virtual bool CanSaveAsset() const override;
	virtual void CreateEditorModeManager() override;
	//~ End FAssetEditorToolkit interface

	/** Initialize command list for default commands */
	void CreateDefaultCommands();

	/** Initialize command list for more specific commands (Auto Arrange ...) */
	void BindToolkitCommands();

	void HandleAutoArrange();
	bool CanAutoArrange() const;

	/** Tab spawner for main graph area for combo asset */
	TSharedRef<SDockTab> SpawnGraphViewportTab(const FSpawnTabArgs& Args);

	/** Tab spawner for details tab */
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
private:
	/** The extender to pass to the level editor to extend it's window menu */
	TSharedPtr<FExtender> MenuExtender;

	/** Toolbar extender */
	TSharedPtr<FExtender> ToolbarExtender;

	/** Custom toolbar builder */
    TSharedPtr<FComboGraphEditorToolbar> ToolbarBuilder;

	/** Shared pointers to internal widgets */
	TSharedPtr<SGraphEditor> GraphEditorWidget;
	TSharedPtr<IDetailsView> PropertyDetailsWidget;

	/** Last selected Auto Arrange Strategy*/
	EComboGraphAutoArrangeStrategy LastAutoArrangeStrategy;

	/** Creates Property Editor Widget for Combo Graph object */
	void CreatePropertyWidget();
	bool IsPropertyEditable();

	/** Creates SGraphEditor widget viewport */
	TSharedRef<SGraphEditor> CreateGraphWidget();

	/** Returns whether the graph is in read only mode or not */
	bool IsGraphEditable(bool bGraphIsEditable) const;

	static bool IsPIESimulating();
	static bool IsPIENotSimulating();

	/** Extend menu */
	void ExtendMenu();

	/** Extend toolbar */
	void ExtendToolbar();

	/** Fill the toolbar with content */
	void FillToolbar(FToolBarBuilder& InToolbarBuilder);

	/** Returns correct toolbar icon depending on selected strategy in combo dropdown */
	FSlateIcon GetAutoArrangeIcon() const;
	FSlateIcon GetAutoArrangeIcon(EComboGraphAutoArrangeStrategy InStrategy) const;

	/** Auto arrange combo dropdown factory */
	TSharedRef<SWidget> GenerateAutoArrangeMenuContent();

	/** Updates LastAutoArrangeStrategy and invokes HandleAutoArrange */
	void SetAndHandleAutoArrange(EComboGraphAutoArrangeStrategy InStrategy);

	/** Handle preview scene setup */
	void HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene);
	void HandleViewportCreated(const TSharedRef<class IPersonaViewport>& InViewport);

	/** Handle switching skeletal meshes */
	void HandlePreviewMeshChanged(USkeletalMesh* InOldSkeletalMesh, USkeletalMesh* InNewSkeletalMesh);

	/** Rebind our anim instance to the preview's skeletal mesh component */
	void RebindToSkeletalMeshComponent();

	/** Loops through Ubergraph Pages and returns ACE Editor Graph */
	UComboGraphEdGraph* GetComboEditorGraph() const;

	/** Iterates through all Editor Nodes and updates error text based on their anim asset and preview skeleton, displaying an error text in case of skeleton mismatch */
	void UpdateGraphNodesError(USkeletalMesh* InSkeletalMesh) const;

	/** Called whenever Blueprint is compiled */
	void HandleBlueprintCompiled(UBlueprint* Blueprint);

	/** Triggered when the package saving has completed and was successful */
	void OnPackageSaved(const FString& PackageFileName, UPackage* Outer, FObjectPostSaveContext SaveContext);

	/** Property Widget changed delegate */
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	/** Updates and rebuild underlying combo graph */
	void RebuildComboGraph();

	/** Little helper to get current selection from currently focused editor graph */
	FGraphPanelSelectionSet GetSelectedNodes() const;

	/** Delegate handlers for common graph editor commands */
	void SelectAllNodes() const;
	bool CanSelectAllNodes();
	void DeleteSelectedNodes() const;
	bool CanDeleteNodes() const;
	void DeleteSelectedDuplicateNodes() const;
	void CutSelectedNodes();
	bool CanCutNodes();
	void CopySelectedNodes() const;
	bool CanCopyNodes() const;
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes();
	void DuplicateNodes();
	bool CanDuplicateNodes();
	void OnRenameNode();
	bool CanRenameNodes() const;

	bool CanCreateComment() const;
	void OnCreateComment() const;

	/** Slate Graph Editor delegate handlers */

	/** Called when the selection changes in the GraphEditor */
	void OnGraphSelectionChanged(const TSet<class UObject*>& NewSelection);
	/** Called when a node is double clicked (noop for now) */
	void OnGraphNodeDoubleClicked(UEdGraphNode* Node);
	/** Called on rename ? */
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);


	friend FComboGraphPersonaAppMode;
	friend FComboGraphDefaultAppMode;
};
