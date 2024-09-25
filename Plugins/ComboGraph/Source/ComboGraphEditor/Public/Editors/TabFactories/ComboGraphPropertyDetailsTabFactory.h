// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDocumentation.h"
#include "PersonaModule.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FComboGraphAssetEditor;

struct FComboGraphPropertyDetailsTabFactory : public FWorkflowTabFactory
{
	FComboGraphPropertyDetailsTabFactory(const TSharedPtr<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FComboGraphAssetEditor> AssetEditor;
};
