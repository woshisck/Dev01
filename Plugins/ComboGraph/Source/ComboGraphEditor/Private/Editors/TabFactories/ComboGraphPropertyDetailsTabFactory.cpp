// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/TabFactories/ComboGraphPropertyDetailsTabFactory.h"

#include "AssetEditor/ComboGraphAssetEditor.h"
#include "Editors/ComboGraphEditorTabs.h"


#define LOCTEXT_NAMESPACE "ComboGraphAnimAssetTabFactory"

FComboGraphPropertyDetailsTabFactory::FComboGraphPropertyDetailsTabFactory(const TSharedPtr<FAssetEditorToolkit>& InHostingApp, const TSharedRef<FComboGraphAssetEditor>& InAssetEditor)
	: FWorkflowTabFactory(FComboGraphEditorTabs::PropertyDetailsID, InHostingApp)
	, AssetEditor(InAssetEditor)
{
	TabLabel = LOCTEXT("TabTitle", "Details");
	// TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("MenuDescription", "Details");
	ViewMenuTooltip = LOCTEXT("MenuTooltip", "Opens up the property details panel");

}

TSharedRef<SWidget> FComboGraphPropertyDetailsTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	if (!AssetEditor.IsValid())
	{
		return FWorkflowTabFactory::CreateTabBody(Info);
	}

	return AssetEditor.Pin()->GetPropertyDetailsWidget().ToSharedRef();
}

TSharedPtr<SToolTip> FComboGraphPropertyDetailsTabFactory::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return FWorkflowTabFactory::CreateTabToolTipWidget(Info);
}

#undef LOCTEXT_NAMESPACE
