#include "AssetEditor_ComboGraph.h"
#include "EdNode_ComboGraphRoot.h"

#include "AssetGraphSchema_GameplayAbilityComboGraph.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimationAsset.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "GenericGraphNode.h"
#include "IDetailsView.h"
#include "IAnimationEditor.h"
#include "IPersonaEditorModeManager.h"
#include "IPersonaPreviewScene.h"
#include "IPersonaToolkit.h"
#include "Interfaces/YogComboGraphActiveInstance.h"
#include "PersonaModule.h"
#include "PropertyEditorModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "YogComboGraphTransientEditorObject.h"
#include "Toolkits/ToolkitManager.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AssetEditor_ComboGraph"

namespace
{
	const FName SelectedMontageTabId(TEXT("YogComboGraph_SelectedMontage"));

	class FYogEmbeddedAnimationEditorHost : public IAnimationEditor, public FGCObject
	{
	public:
		void Init(const TSharedRef<FAssetEditorToolkit>& InOwnerToolkit, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, UAnimationAsset* InAnimationAsset)
		{
			OwnerToolkit = InOwnerToolkit;
			PersonaToolkit = InPersonaToolkit;
			AnimationAsset = InAnimationAsset;
			EditingObjects.Reset();
			if (InAnimationAsset)
			{
				EditingObjects.Add(InAnimationAsset);
			}

			if (!EditorModeManager.IsValid())
			{
				EditorModeManager = MakeShareable(FModuleManager::LoadModuleChecked<FPersonaModule>(TEXT("Persona")).CreatePersonaEditorModeManager());
			}

			// FAssetEditorToolkit always unregisters itself from UAssetEditorSubsystem during destruction.
			// This embedded host is never opened as a real asset editor, so pair that destructor path with
			// a private transient object instead of registering the selected montage itself.
			if (!TransientEditorObject && GEditor)
			{
				TransientEditorObject = NewObject<UYogComboGraphTransientEditorObject>(GetTransientPackage(), NAME_None, RF_Transient);
				if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
				{
					AssetEditorSubsystem->NotifyAssetOpened(TransientEditorObject, this);
				}
			}
		}

		void SetCurveEditorDelegate(FOnEditCurves InOnEditCurves)
		{
			OnEditCurves = InOnEditCurves;
		}

		virtual void SetAnimationAsset(UAnimationAsset* AnimAsset) override
		{
			AnimationAsset = AnimAsset;
			EditingObjects.Reset();
			if (AnimAsset)
			{
				EditingObjects.Add(AnimAsset);
			}
			if (PersonaToolkit.IsValid())
			{
				PersonaToolkit.Pin()->SetAnimationAsset(AnimAsset);
			}
		}

		virtual IAnimationSequenceBrowser* GetAssetBrowser() const override
		{
			return nullptr;
		}

		virtual void EditCurves(UAnimSequenceBase* InAnimSequence, const TArray<FCurveEditInfo>& InCurveInfo, const TSharedPtr<ITimeSliderController>& InExternalTimeSliderController) override
		{
			OnEditCurves.ExecuteIfBound(InAnimSequence, InCurveInfo, InExternalTimeSliderController);
		}

		virtual void StopEditingCurves(const TArray<FCurveEditInfo>& InCurveInfo) override
		{
		}

		virtual FOnAnimationEditorObjectsSelected& OnAnimationEditorObjectsSelected() override
		{
			return OnAnimationEditorObjectsSelectedDelegate;
		}

		virtual TSharedRef<IPersonaToolkit> GetPersonaToolkit() const override
		{
			return PersonaToolkit.Pin().ToSharedRef();
		}

		virtual FName GetToolkitFName() const override
		{
			return FName(TEXT("YogComboGraphEmbeddedMontageEditor"));
		}

		virtual FText GetBaseToolkitName() const override
		{
			return LOCTEXT("EmbeddedMontageToolkitName", "Yog Combo Montage Editor");
		}

		virtual FText GetToolkitName() const override
		{
			return AnimationAsset.IsValid() ? FText::FromString(AnimationAsset->GetName()) : GetBaseToolkitName();
		}

		virtual FText GetToolkitToolTipText() const override
		{
			return AnimationAsset.IsValid() ? FText::FromString(AnimationAsset->GetPathName()) : GetBaseToolkitName();
		}

		virtual FString GetWorldCentricTabPrefix() const override
		{
			return TEXT("YogComboGraph Montage ");
		}

		virtual FLinearColor GetWorldCentricTabColorScale() const override
		{
			return FLinearColor(0.2f, 0.35f, 0.45f, 0.5f);
		}

		virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override
		{
		}

		virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override
		{
		}

		virtual bool IsAssetEditor() const override
		{
			return false;
		}

		virtual const TArray<UObject*>* GetObjectsCurrentlyBeingEdited() const override
		{
			return &EditingObjects;
		}

		virtual bool IsHosted() const override
		{
			return OwnerToolkit.IsValid();
		}

		virtual const TSharedRef<IToolkitHost> GetToolkitHost() const override
		{
			return OwnerToolkit.Pin()->GetToolkitHost();
		}

		virtual TSharedPtr<SWidget> GetInlineContent() const override
		{
			return TSharedPtr<SWidget>();
		}

		virtual void InvokeTab(const FTabId& TabId) override
		{
			if (TSharedPtr<FAssetEditorToolkit> OwnerToolkitPinned = OwnerToolkit.Pin())
			{
				OwnerToolkitPinned->InvokeTab(TabId);
			}
		}

		virtual TSharedPtr<FTabManager> GetAssociatedTabManager() override
		{
			if (TSharedPtr<FAssetEditorToolkit> OwnerToolkitPinned = OwnerToolkit.Pin())
			{
				return OwnerToolkitPinned->GetAssociatedTabManager();
			}

			return TSharedPtr<FTabManager>();
		}

		virtual bool IncludeAssetInRestoreOpenAssetsPrompt(UObject* Asset) const override
		{
			return false;
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Collector.AddReferencedObject(AnimationAsset);
			Collector.AddReferencedObject(TransientEditorObject);
		}

		virtual FString GetReferencerName() const override
		{
			return TEXT("FYogEmbeddedAnimationEditorHost");
		}

	private:
		TWeakPtr<FAssetEditorToolkit> OwnerToolkit;
		TWeakPtr<IPersonaToolkit> PersonaToolkit;
		TWeakObjectPtr<UAnimationAsset> AnimationAsset;
		TObjectPtr<UYogComboGraphTransientEditorObject> TransientEditorObject;
		FOnEditCurves OnEditCurves;
		FOnAnimationEditorObjectsSelected OnAnimationEditorObjectsSelectedDelegate;
		TArray<UObject*> EditingObjects;
	};
}

FAssetEditor_ComboGraph::FAssetEditor_ComboGraph()
{
	PIEStartHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FAssetEditor_ComboGraph::OnPIEStarted);
	PIEEndHandle   = FEditorDelegates::PrePIEEnded.AddRaw(this,   &FAssetEditor_ComboGraph::OnPIEEnded);
}

FAssetEditor_ComboGraph::~FAssetEditor_ComboGraph()
{
	FEditorDelegates::PostPIEStarted.Remove(PIEStartHandle);
	FEditorDelegates::PrePIEEnded.Remove(PIEEndHandle);

	if (DebugTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DebugTickHandle);
	}

	ReleaseSelectedMontageWorkspace();
	ClearDebugState();
}

void FAssetEditor_ComboGraph::CreateEditorModeManager()
{
	EditorModeManager = MakeShareable(FModuleManager::LoadModuleChecked<FPersonaModule>(TEXT("Persona")).CreatePersonaEditorModeManager());
}

void FAssetEditor_ComboGraph::CreateEdGraph()
{
	FAssetEditor_GenericGraph::CreateEdGraph();

	if (!EditingGraph || !EditingGraph->EdGraph)
	{
		return;
	}

	EditingGraph->EdGraph->Schema = UAssetGraphSchema_GameplayAbilityComboGraph::StaticClass();

	// Ensure every graph (new or opened from disk) has exactly one root node.
	// New graphs: FAssetEditor_GenericGraph::CreateEdGraph calls CreateDefaultNodesForGraph
	// on the generic schema (no-op), so we always reach here without a root.
	// Existing assets: also reach here without a root until saved with this change.
	const bool bHasRoot = EditingGraph->EdGraph->Nodes.ContainsByPredicate(
		[](const UEdGraphNode* N) { return Cast<const UEdNode_ComboGraphRoot>(N) != nullptr; });

	if (!bHasRoot)
	{
		const UEdGraphSchema* Schema = EditingGraph->EdGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*EditingGraph->EdGraph);
	}

	if (UAssetGraphSchema_GameplayAbilityComboGraph::EnsureRootConnectionEdges(*EditingGraph->EdGraph))
	{
		EditingGraph->EdGraph->NotifyGraphChanged();
		EditingGraph->MarkPackageDirty();
	}
}

void FAssetEditor_ComboGraph::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	FAssetEditor_GenericGraph::OnSelectedNodesChanged(NewSelection);
	UpdateSelectedMontage();
}

void FAssetEditor_ComboGraph::OpenSelectedMontageTab()
{
	InvokeTab(FTabId(SelectedMontageTabId));
}

void FAssetEditor_ComboGraph::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditor_GenericGraph::RegisterTabSpawners(InTabManager);

	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();
	InTabManager->RegisterTabSpawner(SelectedMontageTabId, FOnSpawnTab::CreateSP(this, &FAssetEditor_ComboGraph::SpawnTab_SelectedMontage))
		.SetDisplayName(LOCTEXT("SelectedMontageTab", "Selected Montage"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.AnimMontage"));
}

void FAssetEditor_ComboGraph::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(SelectedMontageTabId);
	FAssetEditor_GenericGraph::UnregisterTabSpawners(InTabManager);
}

TSharedRef<SDockTab> FAssetEditor_ComboGraph::SpawnTab_SelectedMontage(const FSpawnTabArgs& Args)
{
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	MontageDetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
	UpdateSelectedMontage();

	TSharedRef<SDockTab> MontageTab = SNew(SDockTab)
		.Label(LOCTEXT("SelectedMontageTabTitle", "Selected Montage"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &FAssetEditor_ComboGraph::GetSelectedMontageSummaryText)
					.AutoWrapText(true)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenSelectedMontage", "Open Montage Editor"))
					.IsEnabled_Lambda([this]() { return SelectedMontage.IsValid(); })
					.OnClicked(this, &FAssetEditor_ComboGraph::OpenSelectedMontage)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f, 8.f, 8.f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Horizontal)
				+ SSplitter::Slot()
				.Value(0.72f)
				[
					SNew(SSplitter)
					.Orientation(Orient_Vertical)
					+ SSplitter::Slot()
					.Value(0.55f)
					[
						SAssignNew(MontageViewportHost, SBox)
						[
							MakeMontageWorkspacePlaceholder()
						]
					]
					+ SSplitter::Slot()
					.Value(0.45f)
					[
						SAssignNew(MontageTimelineHost, SBox)
						[
							MakeMontageWorkspacePlaceholder()
						]
					]
				]
				+ SSplitter::Slot()
				.Value(0.28f)
				[
					SNew(SSplitter)
					.Orientation(Orient_Vertical)
					+ SSplitter::Slot()
					.Value(0.42f)
					[
						SAssignNew(MontageSectionsHost, SBox)
						[
							MakeMontageWorkspacePlaceholder()
						]
					]
					+ SSplitter::Slot()
					.Value(0.58f)
					[
						SNew(SBorder)
						.Padding(4.f)
						[
							MontageDetailsView.ToSharedRef()
						]
					]
				]
			]
		];

	RefreshSelectedMontageWorkspace();
	return MontageTab;
}

void FAssetEditor_ComboGraph::UpdateSelectedMontage()
{
	UGameplayAbilityComboGraphNode* ComboNode = GetSingleSelectedComboNode();
	SelectedComboNode = ComboNode;
	SelectedMontage = ComboNode ? ComboNode->Montage.Get() : nullptr;

	if (MontageDetailsView.IsValid())
	{
		MontageDetailsView->SetObject(SelectedMontage.Get());
	}

	RefreshSelectedMontageWorkspace();
}

void FAssetEditor_ComboGraph::RefreshSelectedMontageWorkspace()
{
	UAnimMontage* Montage = SelectedMontage.Get();
	if (!MontageViewportHost.IsValid() || !MontageTimelineHost.IsValid() || !MontageSectionsHost.IsValid())
	{
		return;
	}

	if (!Montage)
	{
		ResetSelectedMontageWorkspace();
		return;
	}

	if (WorkspaceMontage.Get() == Montage && MontagePersonaToolkit.IsValid())
	{
		MontagePersonaToolkit->SetAnimationAsset(Montage);
		MontagePersonaToolkit->GetPreviewScene()->SetPreviewAnimationAsset(Montage);
		return;
	}

	BuildSelectedMontageWorkspace(Montage);
}

void FAssetEditor_ComboGraph::ResetSelectedMontageWorkspace()
{
	if (MontageViewportHost.IsValid())
	{
		MontageViewportHost->SetContent(MakeMontageWorkspacePlaceholder());
	}
	if (MontageTimelineHost.IsValid())
	{
		MontageTimelineHost->SetContent(MakeMontageWorkspacePlaceholder());
	}
	if (MontageSectionsHost.IsValid())
	{
		MontageSectionsHost->SetContent(MakeMontageWorkspacePlaceholder());
	}

	WorkspaceMontage = nullptr;
	MontageAnimationEditorHost.Reset();
	MontagePersonaToolkit.Reset();
}

void FAssetEditor_ComboGraph::ReleaseSelectedMontageWorkspace()
{
	if (MontageViewportHost.IsValid())
	{
		MontageViewportHost->SetContent(SNullWidget::NullWidget);
	}
	if (MontageTimelineHost.IsValid())
	{
		MontageTimelineHost->SetContent(SNullWidget::NullWidget);
	}
	if (MontageSectionsHost.IsValid())
	{
		MontageSectionsHost->SetContent(SNullWidget::NullWidget);
	}

	WorkspaceMontage = nullptr;
	MontageAnimationEditorHost.Reset();
	MontagePersonaToolkit.Reset();
	MontageSectionsHost.Reset();
	MontageTimelineHost.Reset();
	MontageViewportHost.Reset();
	MontageDetailsView.Reset();
}

void FAssetEditor_ComboGraph::BuildSelectedMontageWorkspace(UAnimMontage* Montage)
{
	ResetSelectedMontageWorkspace();

	if (!Montage || !MontageViewportHost.IsValid() || !MontageTimelineHost.IsValid() || !MontageSectionsHost.IsValid())
	{
		return;
	}

	FPersonaToolkitArgs ToolkitArgs;
	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>(TEXT("Persona"));
	MontagePersonaToolkit = PersonaModule.CreatePersonaToolkit(Cast<UAnimationAsset>(Montage), ToolkitArgs);
	MontagePersonaToolkit->SetAnimationAsset(Montage);
	MontagePersonaToolkit->GetPreviewScene()->SetDefaultAnimationMode(EPreviewSceneDefaultAnimationMode::Animation);
	MontagePersonaToolkit->GetPreviewScene()->SetPreviewAnimationAsset(Montage);

	TSharedRef<IPersonaToolkit> PersonaToolkitRef = MontagePersonaToolkit.ToSharedRef();
	TSharedPtr<IEditableSkeleton> EditableSkeleton = PersonaToolkitRef->GetEditableSkeleton();
	if (!EditableSkeleton.IsValid())
	{
		ResetSelectedMontageWorkspace();
		return;
	}

	TSharedRef<FYogEmbeddedAnimationEditorHost> AnimationEditorHost = MakeShared<FYogEmbeddedAnimationEditorHost>();
	AnimationEditorHost->Init(StaticCastSharedRef<FAssetEditorToolkit>(AsShared()), PersonaToolkitRef, Montage);
	MontageAnimationEditorHost = AnimationEditorHost;
	TSharedRef<IAnimationEditor> AnimationEditorHostRef = StaticCastSharedRef<IAnimationEditor>(AnimationEditorHost);
	TSharedRef<FWorkflowCentricApplication> WorkflowHostRef = StaticCastSharedRef<FWorkflowCentricApplication>(AnimationEditorHost);

	FPersonaViewportArgs ViewportArgs(PersonaToolkitRef->GetPreviewScene());
	ViewportArgs.ContextName = TEXT("YogComboGraph.SelectedMontageViewport");
	ViewportArgs.bShowTimeline = true;
	ViewportArgs.bShowStats = true;
	ViewportArgs.bShowPhysicsMenu = false;

	FWorkflowTabSpawnInfo SpawnInfo;
	TSharedRef<SWidget> MontageViewportWidget = PersonaModule.CreatePersonaViewportTabFactory(WorkflowHostRef, ViewportArgs)->CreateTabBody(SpawnInfo);

	FAnimDocumentArgs DocumentArgs(PersonaToolkitRef->GetPreviewScene(), PersonaToolkitRef, EditableSkeleton.ToSharedRef(), MontageSectionsChanged);
	DocumentArgs.OnDespatchObjectsSelected = FOnObjectsSelected::CreateSP(this, &FAssetEditor_ComboGraph::HandleMontageObjectsSelected);
	DocumentArgs.OnDespatchInvokeTab = FOnInvokeTab::CreateSP(this, &FAssetEditor_ComboGraph::HandleMontageInvokeTab);
	DocumentArgs.OnDespatchSectionsChanged = FSimpleDelegate::CreateSP(this, &FAssetEditor_ComboGraph::HandleMontageSectionsChanged);

	FString DocumentLink;
	TSharedRef<SWidget> MontageTimeline = PersonaModule.CreateEditorWidgetForAnimDocument(AnimationEditorHostRef, Montage, DocumentArgs, DocumentLink);
	TSharedRef<SWidget> SectionsPanel = PersonaModule.CreateAnimMontageSectionsTabFactory(WorkflowHostRef, PersonaToolkitRef, MontageSectionsChanged)->CreateTabBody(SpawnInfo);

	MontageViewportHost->SetContent(MontageViewportWidget);
	MontageTimelineHost->SetContent(MontageTimeline);
	MontageSectionsHost->SetContent(SectionsPanel);
	WorkspaceMontage = Montage;
}

TSharedRef<SWidget> FAssetEditor_ComboGraph::MakeMontageWorkspacePlaceholder() const
{
	return SNew(SBorder)
		.Padding(12.f)
		[
			SNew(STextBlock)
			.Text(this, &FAssetEditor_ComboGraph::GetSelectedMontageSummaryText)
			.AutoWrapText(true)
		];
}

UGameplayAbilityComboGraphNode* FAssetEditor_ComboGraph::GetSingleSelectedComboNode() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		return nullptr;
	}

	for (UObject* SelectedObject : SelectedNodes)
	{
		if (const UEdNode_GenericGraphNode* EdNode = Cast<UEdNode_GenericGraphNode>(SelectedObject))
		{
			return Cast<UGameplayAbilityComboGraphNode>(EdNode->GenericGraphNode);
		}
		return Cast<UGameplayAbilityComboGraphNode>(SelectedObject);
	}

	return nullptr;
}

FText FAssetEditor_ComboGraph::GetSelectedMontageSummaryText() const
{
	if (!SelectedComboNode.IsValid())
	{
		return LOCTEXT("NoComboNodeSelected", "Select one combo node to edit its montage.");
	}

	if (!SelectedMontage.IsValid())
	{
		return FText::Format(
			LOCTEXT("SelectedNodeNoMontage", "Node {0} has no montage assigned."),
			FText::FromString(SelectedComboNode->NodeId.IsNone() ? SelectedComboNode->GetName() : SelectedComboNode->NodeId.ToString()));
	}

	return FText::Format(
		LOCTEXT("SelectedMontageSummary", "Node {0}: {1}"),
		FText::FromString(SelectedComboNode->NodeId.IsNone() ? SelectedComboNode->GetName() : SelectedComboNode->NodeId.ToString()),
		FText::FromString(SelectedMontage->GetPathName()));
}

FReply FAssetEditor_ComboGraph::OpenSelectedMontage()
{
	if (GEditor && SelectedMontage.IsValid())
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(SelectedMontage.Get());
		}
	}
	return FReply::Handled();
}

void FAssetEditor_ComboGraph::HandleMontageObjectsSelected(const TArray<UObject*>& SelectedObjects)
{
	if (MontageDetailsView.IsValid())
	{
		if (SelectedObjects.Num() > 0)
		{
			MontageDetailsView->SetObjects(SelectedObjects);
		}
		else
		{
			MontageDetailsView->SetObject(SelectedMontage.Get());
		}
	}
}

void FAssetEditor_ComboGraph::HandleMontageSectionsChanged()
{
	MontageSectionsChanged.Broadcast();
	if (SelectedMontage.IsValid())
	{
		SelectedMontage->MarkPackageDirty();
	}
}

void FAssetEditor_ComboGraph::HandleMontageInvokeTab(const FTabId& TabId)
{
	InvokeTab(TabId);
}

void FAssetEditor_ComboGraph::OnPIEStarted(bool /*bIsSimulating*/)
{
	DebugTickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FAssetEditor_ComboGraph::TickDebugger));
}

void FAssetEditor_ComboGraph::OnPIEEnded(bool /*bIsSimulating*/)
{
	if (DebugTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DebugTickHandle);
		DebugTickHandle.Reset();
	}
	ClearDebugState();
}

bool FAssetEditor_ComboGraph::TickDebugger(float /*DeltaTime*/)
{
	UGameplayAbilityComboGraph* ComboGraph = Cast<UGameplayAbilityComboGraph>(EditingGraph);
	if (!ComboGraph)
	{
		return true;
	}

	// Clear all debug highlights first.
	for (UGenericGraphNode* Node : ComboGraph->AllNodes)
	{
		if (UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node))
		{
			ComboNode->bDebugActive = false;
		}
	}

	if (!GEditor || !GEditor->PlayWorld)
	{
		return true;
	}

	// Find any object in the PIE world that implements IYogComboGraphActiveInstance
	// and is currently running this specific ComboGraph asset.
	FName ActiveNodeId = NAME_None;
	for (TActorIterator<AActor> ActorIt(GEditor->PlayWorld); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!Actor)
		{
			continue;
		}

		for (UActorComponent* Component : Actor->GetComponents())
		{
			IYogComboGraphActiveInstance* Active = Cast<IYogComboGraphActiveInstance>(Component);
			if (!Active || Active->GetActiveComboGraph() != ComboGraph)
			{
				continue;
			}
			const FName CandidateId = Active->GetActiveComboNodeId();
			if (!CandidateId.IsNone())
			{
				ActiveNodeId = CandidateId;
				break;
			}
		}

		if (!ActiveNodeId.IsNone())
		{
			break;
		}
	}

	if (ActiveNodeId.IsNone())
	{
		return true;
	}

	for (UGenericGraphNode* Node : ComboGraph->AllNodes)
	{
		UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
		if (!ComboNode)
		{
			continue;
		}
		// Match by NodeId; fall back to object name (mirrors GetRuntimeNodeId in .cpp).
		const FName RuntimeId = ComboNode->NodeId.IsNone() ? FName(*ComboNode->GetName()) : ComboNode->NodeId;
		if (RuntimeId == ActiveNodeId)
		{
			ComboNode->bDebugActive = true;
			break;
		}
	}

	return true;
}

void FAssetEditor_ComboGraph::ClearDebugState()
{
	if (!EditingGraph)
	{
		return;
	}
	for (UGenericGraphNode* Node : EditingGraph->AllNodes)
	{
		if (UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node))
		{
			ComboNode->bDebugActive = false;
		}
	}
}

#undef LOCTEXT_NAMESPACE
