// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PersonaModule.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

class IComboGraphBlueprintEditor;
class IPersonaToolkit;
class FComboGraphAssetEditor;

class FComboGraphDefaultAppMode : public FApplicationMode
{
public:
	FComboGraphDefaultAppMode(const TSharedRef<FComboGraphAssetEditor>& InAssetEditor);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;

protected:
	// Set of spawnable tabs
	FWorkflowAllowedTabSet TabFactories;

	TWeakPtr<FComboGraphAssetEditor> AssetEditor;

private:
	TSharedPtr<FTabManager::FLayout> GenerateTabLayout(const TSharedRef<FAssetEditorToolkit>& InAssetEditor);

	/** Create a custom tab factory for our specialized animation asset browser (ability to filter anim assets based on current preview mesh) */
	TSharedRef<FWorkflowTabFactory> CreateAnimationAssetBrowserTabFactory(
		const TSharedRef<FAssetEditorToolkit>& InHostingApp,
		const TSharedRef<IPersonaToolkit>& InPersonaToolkit,
		const TSharedRef<IComboGraphBlueprintEditor>& InBlueprintEditor,
		FOnOpenNewAsset InOnOpenNewAsset,
		FOnAnimationSequenceBrowserCreated InOnAnimationSequenceBrowserCreated,
		bool bInShowHistory
	) const;

	TSharedRef<FWorkflowTabFactory> CreateComboGraphViewportTabFactory(const TSharedRef<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor) const;
	TSharedRef<FWorkflowTabFactory> CreatePropertyDetailsTabFactory(const TSharedRef<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor) const;
};
