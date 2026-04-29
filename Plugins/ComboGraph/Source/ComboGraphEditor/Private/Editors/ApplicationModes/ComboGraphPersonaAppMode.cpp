// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/ApplicationModes/ComboGraphPersonaAppMode.h"

#include "BlueprintEditorModule.h"
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "PersonaTabs.h"
#include "AssetEditor/ComboGraphAssetEditor.h"
#include "Editors/ComboGraphEditorTabs.h"
#include "Editors/ComboGraphEditorToolbar.h"
#include "Editors/TabFactories/ComboGraphAnimAssetTabFactory.h"
#include "Editors/TabFactories/ComboGraphPropertyDetailsTabFactory.h"
#include "Editors/TabFactories/ComboGraphViewportTabFactory.h"

FComboGraphPersonaAppMode::FComboGraphPersonaAppMode(const TSharedRef<FComboGraphAssetEditor>& InAssetEditor)
	: FApplicationMode(FComboGraphEditorModes::ComboGraphPersonaModeID)
	, AssetEditor(InAssetEditor)
{
	// setup toolbar - clear existing toolbar extender from the BP mode
	ToolbarExtender = MakeShareable(new FExtender);

	// Extend toolbar with addition modes button
	InAssetEditor->GetToolbarBuilder()->AddComboGraphEditorToolbar(ToolbarExtender);
	InAssetEditor->GetToolbarBuilder()->AddDebuggerToolbar(ToolbarExtender);

	const FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");

	// Create the tab factories
	FPersonaViewportArgs ViewportArgs(InAssetEditor->GetPersonaToolkit()->GetPreviewScene());
	// ViewportArgs.BlueprintEditor = AssetEditor;
	ViewportArgs.bShowStats = false;
	ViewportArgs.bShowPlaySpeedMenu = false;
	ViewportArgs.bShowTimeline = false;
	ViewportArgs.bShowTurnTable = false;
	ViewportArgs.bAlwaysShowTransformToolbar = true;
	ViewportArgs.OnViewportCreated = FOnViewportCreated::CreateSP(InAssetEditor, &FComboGraphAssetEditor::HandleViewportCreated);

	// PersonaModule.RegisterPersonaViewportTabFactories(TabFactories, InAssetEditor, ViewportArgs);
 	TabFactories.RegisterFactory(PersonaModule.CreatePersonaViewportTabFactory(InAssetEditor, ViewportArgs));
	TabFactories.RegisterFactory(PersonaModule.CreateAdvancedPreviewSceneTabFactory(InAssetEditor, InAssetEditor->GetPersonaToolkit()->GetPreviewScene()));

	// Register our custom asset browser factory
	TabFactories.RegisterFactory(CreateAnimationAssetBrowserTabFactory(
		InAssetEditor,
		InAssetEditor->GetPersonaToolkit(),
		InAssetEditor,
		FOnOpenNewAsset::CreateSP(&InAssetEditor.Get(), &FComboGraphAssetEditor::HandleOpenNewAsset),
		FOnAnimationSequenceBrowserCreated::CreateSP(&InAssetEditor.Get(), &FComboGraphAssetEditor::HandleOnAnimationBrowserCreated),
		true
	));

	// Register our custom factories
	TabFactories.RegisterFactory(CreateComboGraphViewportTabFactory(InAssetEditor, InAssetEditor));
	TabFactories.RegisterFactory(CreatePropertyDetailsTabFactory(InAssetEditor, InAssetEditor));

	TabLayout = GenerateTabLayout(InAssetEditor);

	// BlueprintEditorModule.OnRegisterTabsForEditor().Broadcast(TabFactories, FComboGraphEditorModes::BlueprintEditorModeID, InBlueprintEditor);

	// TODO: Needed ?
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	LayoutExtender = MakeShared<FLayoutExtender>();
	TabLayout->ProcessExtensions(*LayoutExtender.Get());
	BlueprintEditorModule.OnRegisterLayoutExtensions().Broadcast(*LayoutExtender);
}

void FComboGraphPersonaAppMode::RegisterTabFactories(const TSharedPtr<FTabManager> InTabManager)
{
	check(AssetEditor.IsValid());
	TSharedPtr<FComboGraphAssetEditor> AssetEditorPtr = AssetEditor.Pin();

	AssetEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	AssetEditorPtr->PushTabFactories(TabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

TSharedPtr<FTabManager::FLayout> FComboGraphPersonaAppMode::GenerateTabLayout(const TSharedRef<FAssetEditorToolkit>& InAssetEditor)
{
	return FTabManager::NewLayout("Standalone_ComboGraphEditMode_Layout_v4.0.2")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION < 5
		->Split
		(
			// Top toolbar
			FTabManager::NewStack()
			->SetSizeCoefficient(0.186721f)
			->SetHideTabWell(true)
			->AddTab(InAssetEditor->GetToolbarTabId(), ETabState::OpenedTab)
		)
#endif
		->Split
		(
			// Main application area
			FTabManager::NewSplitter()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				// Left side
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.25f)
				->SetOrientation(Orient_Vertical)
				->Split
				(
					// Left top - viewport
					FTabManager::NewStack()
					->SetSizeCoefficient(0.5f)
					->SetHideTabWell(true)
					->AddTab(FPersonaTabs::PreviewViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					//	Left bottom - preview settings
					FTabManager::NewStack()
					->SetSizeCoefficient(0.5f)
					->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
					// ->AddTab(FBlueprintEditorTabs::MyBlueprintID, ETabState::OpenedTab)
				)
			)
			->Split
			(
				// Middle
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.55f)
				->Split
				(
					// Middle top - document edit area
					FTabManager::NewStack()
					->SetSizeCoefficient(0.8f)
					->SetHideTabWell(true)
					->AddTab(FComboGraphEditorTabs::GraphViewportID, ETabState::OpenedTab)
				)
			)
			->Split
			(
				// Right side
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.2f)
				->SetOrientation(Orient_Vertical)
				->Split
				(
					// Right top - selection details panel & overrides
					FTabManager::NewStack()
					->SetHideTabWell(false)
					->SetSizeCoefficient(0.5f)
					->AddTab(FComboGraphEditorTabs::PropertyDetailsID, ETabState::OpenedTab)
					// ->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
					->SetForegroundTab(FComboGraphEditorTabs::PropertyDetailsID)
				)
				->Split
				(
					// Right bottom - Asset browser & advanced preview settings
					FTabManager::NewStack()
					->SetHideTabWell(false)
					->SetSizeCoefficient(0.5f)
					// ->AddTab(FPersonaTabs::AssetBrowserID, ETabState::OpenedTab)
					->AddTab(FComboGraphEditorTabs::AssetBrowserID, ETabState::OpenedTab)
					// ->SetForegroundTab(ACEBlueprintEditorTabs::AssetBrowserTab)
				)
			)
		)
	);
}

TSharedRef<FWorkflowTabFactory> FComboGraphPersonaAppMode::CreateAnimationAssetBrowserTabFactory(const TSharedRef<FAssetEditorToolkit>& InHostingApp, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, const TSharedRef<IComboGraphBlueprintEditor>& InBlueprintEditor, FOnOpenNewAsset InOnOpenNewAsset, FOnAnimationSequenceBrowserCreated InOnAnimationSequenceBrowserCreated, bool bInShowHistory) const
{
	return MakeShareable(new FComboGraphAnimAssetTabFactory(
		InHostingApp,
		InPersonaToolkit,
		InBlueprintEditor,
		InOnOpenNewAsset,
		InOnAnimationSequenceBrowserCreated,
		bInShowHistory
	));
}

TSharedRef<FWorkflowTabFactory> FComboGraphPersonaAppMode::CreateComboGraphViewportTabFactory(const TSharedRef<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor) const
{
	return MakeShareable(new FComboGraphViewportTabFactory(InHostingApp, InAssetEditor));
}

TSharedRef<FWorkflowTabFactory> FComboGraphPersonaAppMode::CreatePropertyDetailsTabFactory(const TSharedRef<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor) const
{
	return MakeShareable(new FComboGraphPropertyDetailsTabFactory(InHostingApp, InAssetEditor));
}
