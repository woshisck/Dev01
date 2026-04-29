// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "AssetEditor/ComboGraphAssetEditor.h"

#include "AnimCustomInstanceHelper.h"
#include "AnimPreviewInstance.h"
#include "AnimationEditorPreviewActor.h"
#include "ComboGraphEditorLog.h"
#include "EdGraphUtilities.h"
#include "GraphEditorActions.h"
#include "IPersonaEditorModeManager.h"
#include "IPersonaPreviewScene.h"
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "AssetEditor/ComboGraphDebugger.h"
#include "Editors/ComboGraphBlueprintEditorCommands.h"
#include "Editors/ComboGraphEditorStyle.h"
#include "Editors/ComboGraphEditorTabs.h"
#include "Editors/ComboGraphEditorToolbar.h"
#include "Editors/SComboGraphAnimationAssetBrowser.h"
#include "Editors/ApplicationModes/ComboGraphDefaultAppMode.h"
#include "Editors/ApplicationModes/ComboGraphPersonaAppMode.h"
#include "Framework/Commands/GenericCommands.h"
#include "Graph/ComboGraph.h"
#include "Graph/ComboGraphEdGraph.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "Graph/ComboGraphSchema.h"
#include "Graph/Nodes/ComboGraphEdNode.h"
#include "Graph/Nodes/ComboGraphEdNodeEdge.h"
#include "Graph/Nodes/ComboGraphEdNodeEntry.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/ObjectSaveContext.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3
#include "Persona/Private/AnimationEditorPreviewScene.h"
#endif

#define LOCTEXT_NAMESPACE "ComboGraphAssetEditor"

FComboGraphAssetEditor::FComboGraphAssetEditor()
	: ComboGraphBeingEdited(nullptr)
    , PreviewInstance(nullptr)
	, AnimAssetBrowser(nullptr)
	, LastAutoArrangeStrategy(EComboGraphAutoArrangeStrategy::Vertical)
{
	UPackage::PackageSavedWithContextEvent.AddRaw(this, &FComboGraphAssetEditor::OnPackageSaved);
}

FComboGraphAssetEditor::~FComboGraphAssetEditor()
{
	UPackage::PackageSavedWithContextEvent.RemoveAll(this);
}

void FComboGraphAssetEditor::InitComboGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UComboGraph* InComboGraph)
{
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphBlueprintEditor::InitCombatAbilitiesEditor"))

	check(InComboGraph);

	ComboGraphBeingEdited = InComboGraph;

	CreateEditorGraph();
	CreatePersonaToolkit();
	SetupComboGraphEditor(Mode, InitToolkitHost);

	RebuildComboGraph();
}

void FComboGraphAssetEditor::CreatePersonaToolkit()
{
	check(ComboGraphBeingEdited);

	const FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");

	FPersonaToolkitArgs PersonaToolkitArgs;
	PersonaToolkitArgs.OnPreviewSceneCreated = FOnPreviewSceneCreated::FDelegate::CreateSP(this, &FComboGraphAssetEditor::HandlePreviewSceneCreated);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	PersonaToolkitArgs.bPreviewMeshCanUseDifferentSkeleton = true;
#endif

	PersonaToolkit = PersonaModule.CreatePersonaToolkit(ComboGraphBeingEdited, PersonaToolkitArgs);
	PersonaToolkit->GetPreviewScene()->SetDefaultAnimationMode(EPreviewSceneDefaultAnimationMode::ReferencePose);
	PersonaToolkit->GetPreviewScene()->RegisterOnPreviewMeshChanged(FOnPreviewMeshChanged::CreateSP(this, &FComboGraphAssetEditor::HandlePreviewMeshChanged));

	FPersonaViewportArgs ViewportArgs(PersonaToolkit->GetPreviewScene());
	ViewportArgs.bShowStats = false;
	ViewportArgs.bShowPlaySpeedMenu = false;
	ViewportArgs.bShowTimeline = false;
	ViewportArgs.bShowTurnTable = false;
	ViewportArgs.bAlwaysShowTransformToolbar = true;
	ViewportArgs.OnViewportCreated = FOnViewportCreated::CreateSP(this, &FComboGraphAssetEditor::HandleViewportCreated);
}

void FComboGraphAssetEditor::SetupComboGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	check(ComboGraphBeingEdited);

	FGenericCommands::Register();
	FGraphEditorCommands::Register();

	// Toolbar and Commands
	FComboGraphBlueprintEditorCommands::Register();
	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FComboGraphEditorToolbar(SharedThis(this)));
	}

	ExtendToolbar();

	CreateDefaultCommands();
	BindToolkitCommands();

	// Create custom widgets for this asset editor
	CreatePropertyWidget();
	GraphEditorWidget = CreateGraphWidget();

	// Initialize the asset editor and spawn tabs
	InitAssetEditor(
		Mode,
		InitToolkitHost,
		FName(TEXT("ComboGraphAssetEditor")),
		FTabManager::FLayout::NullLayout,
		true,
		true,
		ComboGraphBeingEdited
	);

	if (UComboGraphEdGraph* ComboGraphEd = GetComboEditorGraph())
	{
		CG_EDITOR_LOG(
			Verbose,
			TEXT("FComboGraphAssetEditor Test SetupComboGraphEditor Creating debugger. Already ? not nullptr: %s, valid: %s"),
			ComboGraphEd->Debugger.Get() != nullptr ? TEXT("true") : TEXT("false"),
			ComboGraphEd->Debugger.IsValid() ? TEXT("true") : TEXT("false")
		)

		ComboGraphEd->Debugger = MakeShareable(new FComboGraphDebugger);
		ComboGraphEd->Debugger->Setup(ComboGraphBeingEdited, SharedThis(this));
	}

	AddApplicationMode(FComboGraphEditorModes::ComboGraphPersonaModeID, MakeShareable(new FComboGraphPersonaAppMode(SharedThis(this))));
	AddApplicationMode(FComboGraphEditorModes::ComboGraphDefaultModeID, MakeShareable(new FComboGraphDefaultAppMode(SharedThis(this))));

	const EComboGraphSelectedAppMode SelectedAppMode = ComboGraphBeingEdited->SelectedAppMode;
	const FName CurrentMode = SelectedAppMode == EComboGraphSelectedAppMode::Graph ? FComboGraphEditorModes::ComboGraphDefaultModeID :
		SelectedAppMode == EComboGraphSelectedAppMode::Persona ? FComboGraphEditorModes::ComboGraphPersonaModeID :
		FComboGraphEditorModes::ComboGraphPersonaModeID;

	CG_EDITOR_LOG(Verbose, TEXT("Set mode ... %s (%s)"), *CurrentMode.ToString(), *UEnum::GetValueAsString(SelectedAppMode))
	SetCurrentMode(CurrentMode);

	// TODO: Check this
	// GetToolkitCommands()->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	ExtendMenu();
	RegenerateMenusAndToolbars();
}

USkeleton* FComboGraphAssetEditor::GetSkeleton() const
{
	if (USkeletalMesh* PreviewMesh = GetPreviewMesh())
	{
		return PreviewMesh->GetSkeleton();
	}

	return nullptr;
}

USkeletalMesh* FComboGraphAssetEditor::GetPreviewMesh() const
{
	if (ComboGraphBeingEdited)
	{
		return ComboGraphBeingEdited->GetPreviewMesh();
	}

	return nullptr;
}

void FComboGraphAssetEditor::SetPreviewMesh(USkeletalMesh* InSkeletalMesh, bool bSetPreviewMeshInAsset)
{
	if (ComboGraphBeingEdited)
	{
		return ComboGraphBeingEdited->SetPreviewMesh(InSkeletalMesh, false);
	}
}

FName FComboGraphAssetEditor::GetToolkitFName() const
{
	return FName("ComboGraphEditor");
}

FText FComboGraphAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("ComboGraphEditor", "Combo Graph Editor");
}

FText FComboGraphAssetEditor::GetToolkitName() const
{
	const TArray<UObject*>& EditingObjs = GetEditingObjects();
	check(EditingObjs.Num() > 0);

	const UObject* EditingObject = EditingObjs[0];
	check(EditingObject);

	FFormatNamedArguments Args;
	Args.Add(TEXT("ComboGraphName"), FText::FromString(EditingObject->GetName()));
	Args.Add(TEXT("DirtyState"), EditingObject->GetOutermost()->IsDirty() ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("ComboGraphAssetEditorToolkitName", "{ComboGraphName}{DirtyState}"), Args);
}

FText FComboGraphAssetEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolkitToolTipText();
}

FString FComboGraphAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("ComboGraphEditor");
}

FLinearColor FComboGraphAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FComboGraphAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphAssetEditor RegisterTabSpawners ..."))

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("ComboGraphWorkspaceMenu", "Combo Graph Editor"));
	const TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FComboGraphEditorTabs::GraphViewportID, FOnSpawnTab::CreateSP(this, &FComboGraphAssetEditor::SpawnGraphViewportTab))
		.SetDisplayName(LOCTEXT("ComboGraphGraphViewportTab", "GraphViewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		// .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "GraphEditor.EventGraph_16x"));
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FComboGraphEditorTabs::PropertyDetailsID, FOnSpawnTab::CreateSP(this, &FComboGraphAssetEditor::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("ComboGraphDetailsTab", "PropertyDetails"))
		.SetGroup(WorkspaceMenuCategoryRef)
		// .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FComboGraphAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FComboGraphEditorTabs::GraphViewportID);
	InTabManager->UnregisterTabSpawner(FComboGraphEditorTabs::PropertyDetailsID);
}

void FComboGraphAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ComboGraphBeingEdited);
	Collector.AddReferencedObject(ComboGraphBeingEdited->EditorGraph);
}

FString FComboGraphAssetEditor::GetReferencerName() const
{
	return TEXT("FComboGraphAssetEditor");
}

FString FComboGraphAssetEditor::GetDocumentationLink() const
{
	// TODO: Figure out how to do our own custom extended tooltip documentation
	return TEXT("");
}

void FComboGraphAssetEditor::HandleOpenNewAsset(UObject* InNewAsset)
{
	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(InNewAsset);
	}
}

void FComboGraphAssetEditor::HandleOnAnimationBrowserCreated(const TSharedRef<IAnimationSequenceBrowser>& InWidget)
{
	SComboGraphAnimationAssetBrowser* AssetBrowser = static_cast<SComboGraphAnimationAssetBrowser*>(&InWidget.Get());
	if (AssetBrowser)
	{
		AnimAssetBrowser = AssetBrowser;
	}
}

bool FComboGraphAssetEditor::CanAccessComboGraphMode() const
{
	return true;
}

void FComboGraphAssetEditor::RegisterToolbarTab(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FComboGraphAssetEditor::DebuggerUpdateGraph(const bool bIsPIEActive)
{
	if (bIsPIEActive)
	{
		RebuildComboGraph();
	}
}

bool FComboGraphAssetEditor::IsDebuggerReady() const
{
	UComboGraphEdGraph* EditorGraph = GetComboEditorGraph();
	return EditorGraph && EditorGraph->Debugger.IsValid() && EditorGraph->Debugger->IsDebuggerReady();
}

TSharedRef<SWidget> FComboGraphAssetEditor::OnGetDebuggerActorsMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	UComboGraphEdGraph* EditorGraph = GetComboEditorGraph();
	if (!EditorGraph)
	{
		return MenuBuilder.MakeWidget();
	}

	if (EditorGraph && EditorGraph->Debugger.IsValid())
	{
		TSharedPtr<FComboGraphDebugger> Debugger = EditorGraph->Debugger;

		TArray<AActor*> MatchingActors;
		TArray<UComboGraphAbilityTask_StartGraph*> MatchingInstances;
		Debugger->GetMatchingInstances(MatchingInstances, MatchingActors);

		// Fill the combo menu with actors that started the combo graph once
		for (int32 i = 0; i < MatchingActors.Num(); i++)
		{
			if (MatchingActors[i])
			{
				const FText ActorDesc = FText::FromString(Debugger->DescribeInstance(*MatchingActors[i]));
				TWeakObjectPtr<AActor> InstancePtr = MatchingActors[i];

				FUIAction ItemAction(FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::OnDebuggerActorSelected, InstancePtr));
				MenuBuilder.AddMenuEntry(ActorDesc, TAttribute<FText>(), FSlateIcon(), ItemAction);
			}
		}

		// Failsafe when no actor match
		if (MatchingActors.Num() == 0)
		{
			const FText ActorDesc = LOCTEXT("NoMatchForDebug","Can't find matching actors");
			TWeakObjectPtr<AActor> InstancePtr;

			FUIAction ItemAction(FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::OnDebuggerActorSelected, InstancePtr));
			MenuBuilder.AddMenuEntry(ActorDesc, TAttribute<FText>(), FSlateIcon(), ItemAction);
		}
	}

	return MenuBuilder.MakeWidget();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::OnDebuggerActorSelected(TWeakObjectPtr<AActor> ActorToDebug)
{
	UComboGraphEdGraph* EditorGraph = GetComboEditorGraph();
	if (EditorGraph && EditorGraph->Debugger.IsValid())
	{
		EditorGraph->Debugger->OnInstanceSelectedInDropdown(ActorToDebug.Get());
	}
}

FText FComboGraphAssetEditor::GetDebuggerActorDesc() const
{
	UComboGraphEdGraph* EditorGraph = GetComboEditorGraph();
	// return Debugger.IsValid() ? FText::FromString(Debugger->GetDebuggedInstanceDesc()) : FText::GetEmpty();
	return EditorGraph && EditorGraph->Debugger.IsValid() ? FText::FromString(EditorGraph->Debugger->GetDebuggedInstanceDesc()) : FText::GetEmpty();
}

AActor* FComboGraphAssetEditor::GetDebuggerSelectedActor() const
{
	UComboGraphEdGraph* EditorGraph = GetComboEditorGraph();
	// return Debugger.IsValid() ? Debugger->GetSelectedActor() : nullptr;
	return EditorGraph && EditorGraph->Debugger.IsValid() ? EditorGraph->Debugger->GetSelectedActor() : nullptr;
}

UComboGraphAbilityTask_StartGraph* FComboGraphAssetEditor::GetDebuggerSelectedTask() const
{
	UComboGraphEdGraph* EditorGraph = GetComboEditorGraph();
	return EditorGraph && EditorGraph->Debugger.IsValid() ? EditorGraph->Debugger->GetDebuggedTaskForSelectedActor() : nullptr;
}

void FComboGraphAssetEditor::SetCurrentMode(const FName NewMode)
{
	if (ComboGraphBeingEdited)
	{
		ComboGraphBeingEdited->SelectedAppMode = NewMode == FComboGraphEditorModes::ComboGraphDefaultModeID ? EComboGraphSelectedAppMode::Graph :
			NewMode == FComboGraphEditorModes::ComboGraphPersonaModeID ? EComboGraphSelectedAppMode::Persona :
			EComboGraphSelectedAppMode::Persona;
	}

	FWorkflowCentricApplication::SetCurrentMode(NewMode);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::CreateEditorGraph()
{
	if (!ComboGraphBeingEdited)
	{
		return;
	}

	if (!ComboGraphBeingEdited->EditorGraph)
	{
		// State Machine Graph
		ComboGraphBeingEdited->EditorGraph = FBlueprintEditorUtils::CreateNewGraph(
			ComboGraphBeingEdited,
			NAME_None,
			UComboGraphEdGraph::StaticClass(),
			UComboGraphSchema::StaticClass()
		);
		ComboGraphBeingEdited->EditorGraph->bAllowRenaming = false;
		ComboGraphBeingEdited->EditorGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes (like the entry node)
		const UEdGraphSchema* GraphSchema = ComboGraphBeingEdited->EditorGraph->GetSchema();
		GraphSchema->CreateDefaultNodesForGraph(*ComboGraphBeingEdited->EditorGraph);
	}
}

void FComboGraphAssetEditor::SaveAsset_Execute()
{
	if (ComboGraphBeingEdited)
	{
		RebuildComboGraph();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

bool FComboGraphAssetEditor::CanSaveAsset() const
{
	if (IsPIESimulating())
	{
		return false;
	}

	return FWorkflowCentricApplication::CanSaveAsset();
}

void FComboGraphAssetEditor::CreateEditorModeManager()
{
	const TSharedPtr<FAssetEditorModeManager> ModeManager = MakeShareable(FModuleManager::LoadModuleChecked<FPersonaModule>("Persona").CreatePersonaEditorModeManager());

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	SetAssetEditorModeManager(ModeManager.Get());
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void FComboGraphAssetEditor::CreateDefaultCommands()
{
	if (DefaultCommands.IsValid())
	{
		return;
	}

	DefaultCommands = MakeShareable(new FUICommandList);

	// Common generic commands
	DefaultCommands->MapAction(
		FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanSelectAllNodes)
	);

	DefaultCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanDeleteNodes)
	);

	DefaultCommands->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanCopyNodes)
	);

	DefaultCommands->MapAction(
		FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanCutNodes)
	);

	DefaultCommands->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanPasteNodes)
	);

	DefaultCommands->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanDuplicateNodes)
	);

	DefaultCommands->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FComboGraphAssetEditor::CanRenameNodes)
	);

	DefaultCommands->MapAction(
		FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::OnCreateComment),
		FCanExecuteAction::CreateRaw(this, &FComboGraphAssetEditor::CanCreateComment)
	);
}

void FComboGraphAssetEditor::BindToolkitCommands()
{
	// Auto Arrange commands
	ToolkitCommands->MapAction(
		FComboGraphBlueprintEditorCommands::Get().AutoArrange,
		FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::HandleAutoArrange),
		FCanExecuteAction::CreateSP(this, &FComboGraphAssetEditor::CanAutoArrange)
	);

	ToolkitCommands->MapAction(
		FComboGraphBlueprintEditorCommands::Get().AutoArrangeVertical,
		FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::SetAndHandleAutoArrange, EComboGraphAutoArrangeStrategy::Vertical),
		FCanExecuteAction::CreateSP(this, &FComboGraphAssetEditor::CanAutoArrange)
	);

	ToolkitCommands->MapAction(
		FComboGraphBlueprintEditorCommands::Get().AutoArrangeHorizontal,
		FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::SetAndHandleAutoArrange, EComboGraphAutoArrangeStrategy::Horizontal),
		FCanExecuteAction::CreateSP(this, &FComboGraphAssetEditor::CanAutoArrange)
	);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::HandleAutoArrange()
{
	UComboGraphEdGraph* Graph = GetComboEditorGraph();
	if (Graph)
	{
		Graph->AutoArrange(LastAutoArrangeStrategy == EComboGraphAutoArrangeStrategy::Vertical);
	}
}

bool FComboGraphAssetEditor::CanAutoArrange() const
{
	return IsPIENotSimulating();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TSharedRef<SDockTab> FComboGraphAssetEditor::SpawnGraphViewportTab(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FComboGraphEditorTabs::GraphViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ComboGraphViewportTab_Title", "Viewport"));

	if (GraphEditorWidget.IsValid())
	{
		SpawnedTab->SetContent(GraphEditorWidget.ToSharedRef());
	}

	return SpawnedTab;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TSharedRef<SDockTab> FComboGraphAssetEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FComboGraphEditorTabs::PropertyDetailsID);

	return SNew(SDockTab)
		.Label(LOCTEXT("ComboGraphPropertyDetails_Title", "Property"))
		[
			PropertyDetailsWidget.ToSharedRef()
		];
}

void FComboGraphAssetEditor::CreatePropertyWidget()
{
	// const FDetailsViewArgs Args(false, false, true, FDetailsViewArgs::ActorsUseNameArea, true, this);

	// UE5: ComboGraphAssetEditor.cpp(583): [C4996] 'FDetailsViewArgs::FDetailsViewArgs': This constructor is deprecated, please create an empty
	// FDetailsViewArgs and explicitly set any values that you wish to change. Please update your code to the new API before upgrading to the
	// next release, otherwise your project will no longer compile.
	FDetailsViewArgs Args;
	Args.bUpdatesFromSelection = false;
	Args.bLockable = false;
	Args.bAllowSearch = true;
	Args.NameAreaSettings = FDetailsViewArgs::ActorsUseNameArea;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;
	Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Show;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyDetailsWidget = PropertyModule.CreateDetailView(Args);
	PropertyDetailsWidget->SetObject(ComboGraphBeingEdited);
	PropertyDetailsWidget->OnFinishedChangingProperties().AddSP(this, &FComboGraphAssetEditor::OnFinishedChangingProperties);

	PropertyDetailsWidget->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &FComboGraphAssetEditor::IsPropertyEditable));

	// Filter out properties tagged with HideInComboGraphDetailsPanel meta specifier
	PropertyDetailsWidget->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& InPropertyAndParent)
	{
		static const FName HideInDetailPanelName("HideInDetailPanel");
		static const FName HideInComboGraphDetailsPanel("HideInComboGraphDetailsPanel");

		if (InPropertyAndParent.Property.HasMetaData(HideInDetailPanelName))
		{
			return false;
		}

		if (InPropertyAndParent.Property.HasMetaData(HideInComboGraphDetailsPanel))
		{
			return false;
		}

		return true;
	}));
}

bool FComboGraphAssetEditor::IsPropertyEditable()
{
	return IsPIENotSimulating();
}

TSharedRef<SGraphEditor> FComboGraphAssetEditor::CreateGraphWidget()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("CornerText", "Combo Graph");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FComboGraphAssetEditor::OnGraphSelectionChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FComboGraphAssetEditor::OnGraphNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FComboGraphAssetEditor::OnNodeTitleCommitted);

	// UEdGraph* EditorGraph = ComboGraphBeingEdited->EditorGraph.Get();
	UEdGraph* EditorGraph = ComboGraphBeingEdited->EditorGraph;
	check(EditorGraph);

	// Make full graph editor
	const bool bGraphIsEditable = EditorGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(DefaultCommands)
		// .IsEditable(true)
		.IsEditable(this, &FComboGraphAssetEditor::IsGraphEditable, bGraphIsEditable)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditorGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(true);
}

bool FComboGraphAssetEditor::IsGraphEditable(const bool bGraphIsEditable) const
{
	return bGraphIsEditable && IsPIENotSimulating();
}

bool FComboGraphAssetEditor::IsPIESimulating()
{
	return GEditor->bIsSimulatingInEditor || GEditor->PlayWorld;
}

bool FComboGraphAssetEditor::IsPIENotSimulating()
{
	return !GEditor->bIsSimulatingInEditor && GEditor->PlayWorld == nullptr;
}

void FComboGraphAssetEditor::ExtendMenu()
{
	if (MenuExtender.IsValid())
	{
		RemoveMenuExtender(MenuExtender);
		MenuExtender.Reset();
	}

	MenuExtender = MakeShareable(new FExtender);
	AddMenuExtender(MenuExtender);
}

void FComboGraphAssetEditor::ExtendToolbar()
{
	// If the ToolbarExtender is valid, remove it before rebuilding it
	if(ToolbarExtender.IsValid())
	{
		RemoveToolbarExtender(ToolbarExtender);
		ToolbarExtender.Reset();
	}

	ToolbarExtender = MakeShareable(new FExtender);

	AddToolbarExtender(ToolbarExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FComboGraphAssetEditor::FillToolbar)
	);
}

void FComboGraphAssetEditor::FillToolbar(FToolBarBuilder& InToolbarBuilder)
{
	// Auto Arrange buttons in toolbar
	InToolbarBuilder.BeginSection("Toolbar");
	{
		InToolbarBuilder.AddToolBarButton(
			FComboGraphBlueprintEditorCommands::Get().AutoArrange,
			NAME_None,
			LOCTEXT("AutoArrangeVertical_Label", "Auto Arrange"),
			TAttribute<FText>(),
			TAttribute<FSlateIcon>::Create(TAttribute<FSlateIcon>::FGetter::CreateSP(this, &FComboGraphAssetEditor::GetAutoArrangeIcon))
		);

		const FUIAction DefaultAction;
		InToolbarBuilder.AddComboButton(
			DefaultAction,
			FOnGetContent::CreateSP(this, &FComboGraphAssetEditor::GenerateAutoArrangeMenuContent),
			LOCTEXT("EventQueue_Label", "Available Modes"),
			LOCTEXT("EventQueue_ToolTip", "Pick between different strategy / modes for re-arranging the Combo Graph"),
			// FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Recompile"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Recompile"),
			true
		);
	}
	InToolbarBuilder.EndSection();
}

FSlateIcon FComboGraphAssetEditor::GetAutoArrangeIcon() const
{
	return GetAutoArrangeIcon(LastAutoArrangeStrategy);
}

FSlateIcon FComboGraphAssetEditor::GetAutoArrangeIcon(EComboGraphAutoArrangeStrategy InStrategy) const
{
	FSlateIcon Icon;
	if (InStrategy == EComboGraphAutoArrangeStrategy::Vertical)
	{
		Icon = FSlateIcon(FComboGraphEditorStyle::Get().GetStyleSetName(), "ComboGraph.AutoArrange.Vertical");
	}
	else if (InStrategy == EComboGraphAutoArrangeStrategy::Horizontal)
	{
		Icon = FSlateIcon(FComboGraphEditorStyle::Get().GetStyleSetName(), "ComboGraph.AutoArrange.Horizontal");
	}
	else
	{
		CG_EDITOR_LOG(Error, TEXT("FComboGraphBlueprintEditor::GetAutoArrangeIcon failed because passed in strategy is invalid"))
	}

	return Icon;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TSharedRef<SWidget> FComboGraphAssetEditor::GenerateAutoArrangeMenuContent()
{
	FMenuBuilder MenuBuilder(true, GetToolkitCommands());

	MenuBuilder.BeginSection(TEXT("Events"));
	MenuBuilder.AddMenuEntry(FComboGraphBlueprintEditorCommands::Get().AutoArrangeVertical, NAME_None, TAttribute<FText>(), TAttribute<FText>(), GetAutoArrangeIcon(EComboGraphAutoArrangeStrategy::Vertical));
	MenuBuilder.AddMenuEntry(FComboGraphBlueprintEditorCommands::Get().AutoArrangeHorizontal, NAME_None, TAttribute<FText>(), TAttribute<FText>(), GetAutoArrangeIcon(EComboGraphAutoArrangeStrategy::Horizontal));
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FComboGraphAssetEditor::SetAndHandleAutoArrange(EComboGraphAutoArrangeStrategy InStrategy)
{
	LastAutoArrangeStrategy = InStrategy;
	HandleAutoArrange();
}

void FComboGraphAssetEditor::HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene)
{
	AAnimationEditorPreviewActor* Actor = InPersonaPreviewScene->GetWorld()->SpawnActor<AAnimationEditorPreviewActor>(AAnimationEditorPreviewActor::StaticClass(), FTransform::Identity);
	Actor->SetFlags(RF_Transient);
	InPersonaPreviewScene->SetActor(Actor);

	// Create the preview component
	UDebugSkelMeshComponent* EditorSkelComp = NewObject<UDebugSkelMeshComponent>(Actor);

	EditorSkelComp->SetSkeletalMesh(InPersonaPreviewScene->GetPersonaToolkit()->GetPreviewMesh());
	InPersonaPreviewScene->SetPreviewMeshComponent(EditorSkelComp);

	bool bWasCreated = false;
	FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UAnimInstance>(EditorSkelComp, bWasCreated);
	InPersonaPreviewScene->AddComponent(EditorSkelComp, FTransform::Identity);

	// set root component, so we can attach to it.
	Actor->SetRootComponent(EditorSkelComp);
	PreviewInstance = Cast<UAnimPreviewInstance>(EditorSkelComp->GetAnimInstance());

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	// remove the preview scene undo handling - it has unwanted side effects
	InPersonaPreviewScene->UnregisterForUndo();
#else
	if (GEditor)
	{
		// remove the preview scene undo handling - it has unwanted side effects
		FAnimationEditorPreviewScene* AnimationEditorPreviewScene = static_cast<FAnimationEditorPreviewScene*>(&InPersonaPreviewScene.Get());
		if (AnimationEditorPreviewScene)
		{
			GEditor->UnregisterForUndo(AnimationEditorPreviewScene);
		}
	}
#endif
}

void FComboGraphAssetEditor::HandleViewportCreated(const TSharedRef<IPersonaViewport>& InViewport)
{
	// TODO: Handle Notification ?
}

void FComboGraphAssetEditor::HandlePreviewMeshChanged(USkeletalMesh* InOldSkeletalMesh, USkeletalMesh* InNewSkeletalMesh)
{
	UAnimationAsset* PreviewAnimAsset = PersonaToolkit->GetPreviewScene()->GetPreviewAnimationAsset();

	PersonaToolkit->GetPreviewScene()->SetPreviewAnimationAsset(nullptr);

	RebindToSkeletalMeshComponent();

	if (ComboGraphBeingEdited)
	{
		ComboGraphBeingEdited->SetPreviewMesh(InNewSkeletalMesh, true);
	}

	if (AnimAssetBrowser)
	{
		AnimAssetBrowser->RefreshAssetView();
	}

	if (PreviewAnimAsset)
	{
		PersonaToolkit->GetPreviewScene()->SetPreviewAnimationAsset(PreviewAnimAsset);
	}

	// Updates error display for each nodes
	UpdateGraphNodesError(InNewSkeletalMesh);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::RebindToSkeletalMeshComponent()
{
	UDebugSkelMeshComponent* MeshComponent = GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent();
	if (MeshComponent)
	{
		bool bWasCreated = false;
		FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UAnimInstance>(MeshComponent , bWasCreated);
	}
}

UComboGraphEdGraph* FComboGraphAssetEditor::GetComboEditorGraph() const
{
	check(ComboGraphBeingEdited);
	return CastChecked<UComboGraphEdGraph>(ComboGraphBeingEdited->EditorGraph);
}

void FComboGraphAssetEditor::UpdateGraphNodesError(USkeletalMesh* InSkeletalMesh) const
{
	UComboGraphEdGraph* Graph = GetComboEditorGraph();
	if (Graph)
	{
		TArray<UComboGraphEdNode*> Nodes = Graph->GetAllNodes();
		for (const UComboGraphEdNode* Node : Nodes)
		{
			Node->UpdateErrorReporting(InSkeletalMesh, LOCTEXT("SkeletonMismatch", "Skeletons are not the same!"));
		}
	}
}

void FComboGraphAssetEditor::HandleBlueprintCompiled(UBlueprint* Blueprint)
{
	// TODO: Probably not needed anymore
}

void FComboGraphAssetEditor::OnPackageSaved(const FString& PackageFileName, UPackage* Outer, FObjectPostSaveContext SaveContext)
{
	// TODO: Needed ?
	RebuildComboGraph();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ComboGraphBeingEdited->EditorGraph)
	{
		ComboGraphBeingEdited->EditorGraph->GetSchema()->ForceVisualizationCacheClear();
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::RebuildComboGraph()
{
	if (UComboGraphEdGraph* Graph = GetComboEditorGraph())
	{
		Graph->RebuildGraph();
	}
}

FGraphPanelSelectionSet FComboGraphAssetEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	if (GraphEditorWidget.IsValid())
	{
		CurrentSelection = GraphEditorWidget->GetSelectedNodes();
	}

	return CurrentSelection;
}

void FComboGraphAssetEditor::SelectAllNodes() const
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->SelectAllNodes();
	}
}

bool FComboGraphAssetEditor::CanSelectAllNodes()
{
	return true;
}

void FComboGraphAssetEditor::DeleteSelectedNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	GraphEditorWidget->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
		if (!EdNode || !EdNode->CanUserDeleteNode())
		{
			continue;
		}

		if (UComboGraphEdNodeBase* ComboGraphEdNode = Cast<UComboGraphEdNodeBase>(EdNode))
		{
			ComboGraphEdNode->Modify();

			const UEdGraphSchema* Schema = ComboGraphEdNode->GetSchema();
			if (Schema)
			{
				Schema->BreakNodeLinks(*ComboGraphEdNode);
			}

			ComboGraphEdNode->DestroyNode();
		}
		else
		{
			EdNode->Modify();
			EdNode->DestroyNode();
		}
	}
}

bool FComboGraphAssetEditor::CanDeleteNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}

	if (IsPIESimulating())
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FComboGraphAssetEditor::DeleteSelectedDuplicateNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			GraphEditorWidget->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicate-able nodes
	DeleteSelectedNodes();

	GraphEditorWidget->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			GraphEditorWidget->SetNodeSelection(Node, true);
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicateNodes();
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool FComboGraphAssetEditor::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FComboGraphAssetEditor::CopySelectedNodes() const
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (const UComboGraphEdNodeEdge* ComboGraphEdge = Cast<UComboGraphEdNodeEdge>(*SelectedIter))
		{
			const UComboGraphEdNode* StartNode = ComboGraphEdge->GetStartNodeAsGraphNode();
			const UComboGraphEdNode* EndNode = ComboGraphEdge->GetEndNode();

			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FComboGraphAssetEditor::CanCopyNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FComboGraphAssetEditor::PasteNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		PasteNodesHere(GraphEditorWidget->GetPasteLocation());
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::PasteNodesHere(const FVector2D& Location)
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	UEdGraph* EdGraph = GraphEditorWidget->GetCurrentGraph();

	{
		const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
		EdGraph->Modify();

		GraphEditorWidget->ClearSelectionSet();

		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

		FVector2D AvgNodePosition(0.0f, 0.0f);

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			const UEdGraphNode* Node = *It;
			AvgNodePosition.X += Node->NodePosX;
			AvgNodePosition.Y += Node->NodePosY;
		}

		const float InvNumNodes = 1.0f / (PastedNodes.Num() == 0 ? 1 : PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			GraphEditorWidget->SetNodeSelection(Node, true);

			Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
			Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

			Node->SnapToGrid(16);

			// Give new node a different Guid from the old one
			Node->CreateNewGuid();
		}
	}

	GraphEditorWidget->NotifyGraphChanged();

	UObject* GraphOwner = EdGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		// ReSharper disable once CppExpressionWithoutSideEffects
		GraphOwner->MarkPackageDirty();
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool FComboGraphAssetEditor::CanPasteNodes()
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}

	if (IsPIESimulating())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(GraphEditorWidget->GetCurrentGraph(), ClipboardContent);
}

void FComboGraphAssetEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool FComboGraphAssetEditor::CanDuplicateNodes()
{
	return CanCopyNodes();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::OnRenameNode()
{
	if (GraphEditorWidget.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode && SelectedNode->bCanRenameNode)
			{
				GraphEditorWidget->IsNodeTitleVisible(SelectedNode, true);
				break;
			}
		}
	}
}

bool FComboGraphAssetEditor::CanRenameNodes() const
{
	if (!ComboGraphBeingEdited)
	{
		return false;
	}

	if (IsPIESimulating())
	{
		return false;
	}

	UComboGraphEdGraph* ComboGraphEd = GetComboEditorGraph();
	check(ComboGraphEd);

	UComboGraph* ComboGraph = ComboGraphEd->GetComboGraphModel();
	check(ComboGraph)

	return ComboGraph->bCanRenameNode && GetSelectedNodes().Num() == 1;
}

bool FComboGraphAssetEditor::CanCreateComment() const
{
 	return GraphEditorWidget.IsValid() ? (GraphEditorWidget->GetNumberOfSelectedNodes() != 0) : false;
}

void FComboGraphAssetEditor::OnCreateComment() const
{
	if (UEdGraph* EdGraph = GraphEditorWidget.IsValid() ? GraphEditorWidget->GetCurrentGraph() : nullptr)
	{
		const TSharedPtr<FEdGraphSchemaAction> Action = EdGraph->GetSchema()->GetCreateCommentAction();
		if (Action.IsValid())
		{
			Action->PerformAction(EdGraph, nullptr, FVector2D());
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::OnGraphSelectionChanged(const TSet<UObject*>& NewSelection)
{
	CG_EDITOR_LOG(Verbose, TEXT("OnGraphSelectionChanged - %d"), NewSelection.Num())

	TArray<UObject*> SelectedNodes;
	TArray<UComboGraphEdNode*> GraphNodes;
	for (UObject* Selection : NewSelection)
	{
		SelectedNodes.Add(Selection);

		if (UComboGraphEdNode* Node = Cast<UComboGraphEdNode>(Selection))
		{
			GraphNodes.Add(Node);
		}
	}

	// Update Property details panel
	if (PropertyDetailsWidget.IsValid())
	{
		if (SelectedNodes.Num() == 0)
		{
			PropertyDetailsWidget->SetObject(ComboGraphBeingEdited);
		}
		else
		{
			// Only allow edition of actual combo graph nodes
			//
			// Handle special case of node selection we don't to allow edition (such as entry node)
			// TODO: Possibly conduit too
			const TArray<UObject*> DetailViewObjects = SelectedNodes.FilterByPredicate([](const UObject* Item)
			{
				return !Item->IsA(UComboGraphEdNodeEntry::StaticClass());
			});

			PropertyDetailsWidget->SetObjects(DetailViewObjects);
		}
	}

	// Handle Persona viewport anim asset

	UComboGraphEdNode* NodeToPreview = nullptr;
	if (GraphNodes.Num() == 0)
	{
		// Should we clear up anim asset on no selection ?
		PersonaToolkit->GetPreviewScene()->SetPreviewAnimationAsset(nullptr);
	}
	else if (GraphNodes.Num() == 1)
	{
		NodeToPreview = GraphNodes[0];
		CG_EDITOR_LOG(Verbose, TEXT("OnGraphSelectionChanged - One node selected, update persona viewport ... %s"), *NodeToPreview->GetName())
	}
	else
	{
		NodeToPreview = GraphNodes.Last();
		CG_EDITOR_LOG(Verbose, TEXT("OnGraphSelectionChanged - More than one graph node selected, how to handle ? %d (preview last: %s)"), GraphNodes.Num(), *NodeToPreview->GetName())
	}

	if (NodeToPreview && NodeToPreview->RuntimeNode)
	{
		UAnimationAsset* AnimationAsset = NodeToPreview->RuntimeNode->GetAnimationAsset();
		CG_EDITOR_LOG(Verbose, TEXT("OnGraphSelectionChanged - Try preview %s"), *GetNameSafe(AnimationAsset))
		PersonaToolkit->GetPreviewScene()->SetPreviewAnimationAsset(AnimationAsset);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphAssetEditor::OnGraphNodeDoubleClicked(UEdGraphNode* Node)
{
	CG_EDITOR_LOG(Verbose, TEXT("OnGraphNodeDoubleClicked - %s"), *GetNameSafe(Node))

	// TODO: Handle rename of node on double click
	// or opening of animation editor
}

void FComboGraphAssetEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		static const FText TransactionTitle = FText::FromString(FString(TEXT("Rename Node")));
		const FScopedTransaction Transaction(TransactionTitle);
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
