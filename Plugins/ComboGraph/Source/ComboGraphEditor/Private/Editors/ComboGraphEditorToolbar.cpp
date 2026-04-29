// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/ComboGraphEditorToolbar.h"

#include "ComboGraphEditorLog.h"
#include "AssetEditor/ComboGraphAssetEditor.h"
#include "Editors/ComboGraphEditorStyle.h"
#include "WorkflowOrientedApp/SModeWidget.h"

#define LOCTEXT_NAMESPACE "ComboGraphEditorToolbar"

void FComboGraphEditorToolbar::AddComboGraphEditorToolbar(const TSharedPtr<FExtender> Extender)
{
	check(AssetEditor.IsValid());
	const TSharedPtr<FComboGraphAssetEditor> AssetEditorPtr = AssetEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		AssetEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FComboGraphEditorToolbar::FillModesToolbar)
	);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphEditorToolbar::FillModesToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(AssetEditor.IsValid());
	const TSharedPtr<FComboGraphAssetEditor> AssetEditorPtr = AssetEditor.Pin();

	const TAttribute<FName> GetActiveMode(AssetEditorPtr.ToSharedRef(), &FComboGraphAssetEditor::GetCurrentMode);
	const FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(AssetEditorPtr.ToSharedRef(), &FComboGraphAssetEditor::SetCurrentMode);

	// Left side padding
	AssetEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

	// Combo Graph Persona Mode
	AssetEditorPtr->AddToolbarWidget(
		SNew(SModeWidget, FComboGraphEditorModes::GetLocalizedMode(FComboGraphEditorModes::ComboGraphPersonaModeID), FComboGraphEditorModes::ComboGraphPersonaModeID)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(AssetEditorPtr.Get(), &FComboGraphAssetEditor::CanAccessComboGraphMode)
		.ToolTipText(LOCTEXT("ToolbarComboGraphPersonaModeButtonTooltip", "Switch to Persona Preview Mode"))
		// TODO: Custom Editor style and custom brushes
		.IconImage(FComboGraphEditorStyle::Get().GetBrush("ComboGraph.AppMode.Persona"))
		// .IconImage(FEditorStyle::Get().GetBrush("BTEditor.Graph.NewDecorator"))
#if ENGINE_MAJOR_VERSION < 5
		.SmallIconImage(FComboGraphEditorStyle::Get().GetBrush("ComboGraph.AppMode.Persona.Small"))
#endif
	);

	// Combo Graph Persona Mode
	AssetEditorPtr->AddToolbarWidget(
		SNew(SModeWidget, FComboGraphEditorModes::GetLocalizedMode(FComboGraphEditorModes::ComboGraphDefaultModeID), FComboGraphEditorModes::ComboGraphDefaultModeID)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(AssetEditorPtr.Get(), &FComboGraphAssetEditor::CanAccessComboGraphMode)
		.ToolTipText(LOCTEXT("ToolbarComboGraphDefaultModeButtonTooltip", "Switch to Combo Graph Mode"))
		// TODO: Custom Editor style and custom brushes
		.IconImage(FComboGraphEditorStyle::Get().GetBrush("ComboGraph.AppMode.Default"))
		// .IconImage(FEditorStyle::Get().GetBrush("BTEditor.Graph.NewDecorator"))
#if ENGINE_MAJOR_VERSION < 5
		.SmallIconImage(FComboGraphEditorStyle::Get().GetBrush("ComboGraph.AppMode.Default.Small"))
#endif
	);

    // Right side padding
    AssetEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
}

void FComboGraphEditorToolbar::AddDebuggerToolbar(const TSharedPtr<FExtender> Extender)
{
	const TSharedPtr<FComboGraphAssetEditor> AssetEditorPtr = AssetEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		AssetEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FComboGraphEditorToolbar::FillDebuggerToolbar, AssetEditor)
	);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphEditorToolbar::FillDebuggerToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<FComboGraphAssetEditor> InAssetEditor)
{
	const TSharedPtr<FComboGraphAssetEditor> AssetEditorPtr = InAssetEditor.Pin();

	const bool bCanShowDebugger = AssetEditorPtr->IsDebuggerReady();

	CG_EDITOR_LOG(Verbose, TEXT("FComboGraphDebugger FComboGraphEditorToolbar::FillDebuggerToolbar %s"), bCanShowDebugger ? TEXT("true") : TEXT("false"))

	const TSharedRef<SWidget> SelectionBox = SNew(SComboButton)
		.OnGetMenuContent(AssetEditorPtr.Get(), &FComboGraphAssetEditor::OnGetDebuggerActorsMenu)
		.ButtonContent()
		[
			SNew(STextBlock)
			.ToolTipText( LOCTEXT("SelectDebugActor", "Pick actor to debug") )
			.Text(AssetEditorPtr.Get(), &FComboGraphAssetEditor::GetDebuggerActorDesc)
		];

	ToolbarBuilder.BeginSection("World");
	{
		ToolbarBuilder.AddWidget(SelectionBox);
	}
	ToolbarBuilder.EndSection();
}


#undef LOCTEXT_NAMESPACE
