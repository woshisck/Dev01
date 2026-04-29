// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "AssetTypes/AssetTypeActions_ComboGraph.h"

#include "ComboGraphEditorLog.h"
#include "AssetEditor/ComboGraphAssetEditor.h"
#include "Graph/ComboGraph.h"

#define LOCTEXT_NAMESPACE "FAssetTypeActions_ComboGraph"

FAssetTypeActions_ComboGraph::FAssetTypeActions_ComboGraph(const EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

FText FAssetTypeActions_ComboGraph::GetName() const
{
	return LOCTEXT("FAssetTypeActions_ComboGraph", "Combo Graph");
}

UClass* FAssetTypeActions_ComboGraph::GetSupportedClass() const
{
	return UComboGraph::StaticClass();
}

FColor FAssetTypeActions_ComboGraph::GetTypeColor() const
{
	return FColor(129, 96, 128);
}

uint32 FAssetTypeActions_ComboGraph::GetCategories()
{
	return AssetCategory;
}

void FAssetTypeActions_ComboGraph::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		CG_EDITOR_LOG(Verbose, TEXT("Combo Graph OpenAssetEditor ... %s"), *GetNameSafe(*ObjIt))

		if (UComboGraph* Graph = Cast<UComboGraph>(*ObjIt))
		{
			const TSharedRef<FComboGraphAssetEditor> AssetEditor(new FComboGraphAssetEditor());
			AssetEditor->InitComboGraphEditor(Mode, EditWithinLevelEditor, Graph);
		}
	}
}


#undef LOCTEXT_NAMESPACE
