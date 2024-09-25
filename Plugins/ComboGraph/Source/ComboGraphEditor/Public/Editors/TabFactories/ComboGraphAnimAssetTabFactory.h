// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDocumentation.h"
#include "PersonaModule.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class IComboGraphBlueprintEditor;
class IPersonaToolkit;

struct FComboGraphAnimAssetTabFactory : public FWorkflowTabFactory
{
	FComboGraphAnimAssetTabFactory(const TSharedPtr<class FAssetEditorToolkit>& InHostingApp, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, const TSharedRef<IComboGraphBlueprintEditor>& InBlueprintEditor, FOnOpenNewAsset InOnOpenNewAsset, FOnAnimationSequenceBrowserCreated InOnAnimationSequenceBrowserCreated, bool bInShowHistory);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;

private:
	TWeakPtr<IPersonaToolkit> PersonaToolkit;
	TWeakPtr<IComboGraphBlueprintEditor> BlueprintEditor;

	FOnOpenNewAsset OnOpenNewAsset;
	FOnAnimationSequenceBrowserCreated OnAnimationSequenceBrowserCreated;
	bool bShowHistory;
};
