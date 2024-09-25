// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FComboGraphAssetEditor;
class FComboGraphBlueprintEditor;

class FComboGraphEditorToolbar : public TSharedFromThis<FComboGraphEditorToolbar>
{
public:
	FComboGraphEditorToolbar(const TSharedPtr<FComboGraphAssetEditor> InAssetEditor) : AssetEditor(InAssetEditor) {}

	void AddComboGraphEditorToolbar(TSharedPtr<FExtender> Extender);
	void AddDebuggerToolbar(TSharedPtr<FExtender> Extender);

protected:
	/** Pointer back to the asset editor tool that owns us */
	TWeakPtr<FComboGraphAssetEditor> AssetEditor;

	void FillModesToolbar(FToolBarBuilder& ToolbarBuilder);
	void FillDebuggerToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<FComboGraphAssetEditor> InAssetEditor);
};
