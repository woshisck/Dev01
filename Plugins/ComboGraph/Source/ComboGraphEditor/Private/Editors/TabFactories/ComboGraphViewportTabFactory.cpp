// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/TabFactories/ComboGraphViewportTabFactory.h"

#include "Editors/ComboGraphEditorTabs.h"
#include "AssetEditor/ComboGraphAssetEditor.h"

#define LOCTEXT_NAMESPACE "ComboGraphViewportTabFactory"

FComboGraphViewportTabFactory::FComboGraphViewportTabFactory(const TSharedPtr<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor)
	: FWorkflowTabFactory(FComboGraphEditorTabs::GraphViewportID, InHostingApp)
	, AssetEditor(InAssetEditor)
{
	TabLabel = LOCTEXT("TabTitle", "Combo Graph");
	// TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.TabIcon");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("MenuDescription", "Combo Graph");
	ViewMenuTooltip = LOCTEXT("MenuTooltip", "Opens up the combo graph");
}

TSharedRef<SWidget> FComboGraphViewportTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	if (!AssetEditor.IsValid())
	{
		return FWorkflowTabFactory::CreateTabBody(Info);
	}

	return AssetEditor.Pin()->GetGraphEditorWidget().ToSharedRef();
}

TSharedPtr<SToolTip> FComboGraphViewportTabFactory::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return FWorkflowTabFactory::CreateTabToolTipWidget(Info);

	// return IDocumentation::Get()->CreateToolTip(
	// 	LOCTEXT("AnimAssetBrowserTooltip", "The Asset Browser lets you browse all animation-related assets (animations, blend spaces etc)."),
	// 	nullptr,
	// 	TEXT("Shared/Editors/Persona"),
	// 	TEXT("AssetBrowser_Window")
	// );
}

#undef LOCTEXT_NAMESPACE
