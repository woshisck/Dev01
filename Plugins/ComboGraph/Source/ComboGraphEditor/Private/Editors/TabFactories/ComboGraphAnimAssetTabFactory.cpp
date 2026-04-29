// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/TabFactories/ComboGraphAnimAssetTabFactory.h"

#include "Editors/ComboGraphEditorTabs.h"
#include "Editors/SComboGraphAnimationAssetBrowser.h"

#define LOCTEXT_NAMESPACE "ComboGraphAnimAssetTabFactory"

FComboGraphAnimAssetTabFactory::FComboGraphAnimAssetTabFactory(const TSharedPtr<FAssetEditorToolkit>& InHostingApp, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, const TSharedRef<IComboGraphBlueprintEditor>& InBlueprintEditor, FOnOpenNewAsset InOnOpenNewAsset, FOnAnimationSequenceBrowserCreated InOnAnimationSequenceBrowserCreated, bool bInShowHistory)
	: FWorkflowTabFactory(FComboGraphEditorTabs::AssetBrowserID, InHostingApp)
	, PersonaToolkit(InPersonaToolkit)
	, BlueprintEditor(InBlueprintEditor)
	, OnOpenNewAsset(InOnOpenNewAsset)
	, OnAnimationSequenceBrowserCreated(InOnAnimationSequenceBrowserCreated)
	, bShowHistory(bInShowHistory)
{
	TabLabel = LOCTEXT("AssetBrowserTabTitle", "Asset Browser");
	// TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.TabIcon");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AssetBrowser", "Asset Browser");
	ViewMenuTooltip = LOCTEXT("AssetBrowser_ToolTip", "Shows the animation asset browser");
}

TSharedRef<SWidget> FComboGraphAnimAssetTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedRef<SComboGraphAnimationAssetBrowser> Widget = SNew(SComboGraphAnimationAssetBrowser, PersonaToolkit.Pin().ToSharedRef(), BlueprintEditor.Pin().ToSharedRef())
		.OnOpenNewAsset(OnOpenNewAsset)
		.ShowHistory(bShowHistory);

	// ReSharper disable once CppExpressionWithoutSideEffects
	OnAnimationSequenceBrowserCreated.ExecuteIfBound(Widget);
	return Widget;
}

TSharedPtr<SToolTip> FComboGraphAnimAssetTabFactory::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return IDocumentation::Get()->CreateToolTip(
		LOCTEXT("AnimAssetBrowserTooltip", "The Asset Browser lets you browse all animation-related assets (animations, blend spaces etc)."),
		nullptr,
		TEXT("Shared/Editors/Persona"),
		TEXT("AssetBrowser_Window")
	);
}

#undef LOCTEXT_NAMESPACE
