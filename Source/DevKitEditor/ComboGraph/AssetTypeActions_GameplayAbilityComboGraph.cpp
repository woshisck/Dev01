#include "ComboGraph/AssetTypeActions_GameplayAbilityComboGraph.h"

#include "Data/GameplayAbilityComboGraph.h"
#include "GenericGraphAssetEditor/AssetEditor_GenericGraph.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_GameplayAbilityComboGraph"

FAssetTypeActions_GameplayAbilityComboGraph::FAssetTypeActions_GameplayAbilityComboGraph(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

FText FAssetTypeActions_GameplayAbilityComboGraph::GetName() const
{
	return LOCTEXT("AssetTypeName", "Gameplay Ability Combo Graph");
}

FColor FAssetTypeActions_GameplayAbilityComboGraph::GetTypeColor() const
{
	return FColor(88, 166, 255);
}

UClass* FAssetTypeActions_GameplayAbilityComboGraph::GetSupportedClass() const
{
	return UGameplayAbilityComboGraph::StaticClass();
}

void FAssetTypeActions_GameplayAbilityComboGraph::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UGameplayAbilityComboGraph* ComboGraph = Cast<UGameplayAbilityComboGraph>(Object))
		{
			TSharedRef<FAssetEditor_GenericGraph> GraphEditor = MakeShared<FAssetEditor_GenericGraph>();
			GraphEditor->InitGenericGraphAssetEditor(Mode, EditWithinLevelEditor, ComboGraph);
		}
	}
}

uint32 FAssetTypeActions_GameplayAbilityComboGraph::GetCategories()
{
	return AssetCategory;
}

#undef LOCTEXT_NAMESPACE
