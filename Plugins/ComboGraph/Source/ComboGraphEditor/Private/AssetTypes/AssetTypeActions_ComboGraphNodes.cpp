// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "AssetTypes/AssetTypeActions_ComboGraphNodes.h"

#include "Graph/ComboGraphNodeBlueprints.h"
#include "Utils/ComboGraphUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_ComboGraphNodeMontage"

FAssetTypeActions_ComboGraphNodeBase::FAssetTypeActions_ComboGraphNodeBase(const EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

uint32 FAssetTypeActions_ComboGraphNodeBase::GetCategories()
{
	return AssetCategory;
}

// ------- FAssetTypeActions_ComboGraphNodeMontage -------------

FAssetTypeActions_ComboGraphNodeMontage::FAssetTypeActions_ComboGraphNodeMontage(const EAssetTypeCategories::Type InAssetCategory)
	: FAssetTypeActions_ComboGraphNodeBase(InAssetCategory)
{
}

FText FAssetTypeActions_ComboGraphNodeMontage::GetName() const
{
	return LOCTEXT("AssetTypeActions_ComboGraphNodeMontage", "Combo Graph Node (Montage)");
}

FColor FAssetTypeActions_ComboGraphNodeMontage::GetTypeColor() const
{
	const FLinearColor MontageColor = FComboGraphUtils::GetPluginProjectSettings()->MontageNodeColor;
	return FColor(MontageColor.R * 255, MontageColor.G * 255, MontageColor.B * 255);
}

UClass* FAssetTypeActions_ComboGraphNodeMontage::GetSupportedClass() const
{
	return UComboGraphNodeMontageBlueprint::StaticClass();
}

// ------- FAssetTypeActions_ComboGraphNodeSequence -------------

FAssetTypeActions_ComboGraphNodeSequence::FAssetTypeActions_ComboGraphNodeSequence(EAssetTypeCategories::Type InAssetCategory)
	: FAssetTypeActions_ComboGraphNodeBase(InAssetCategory)
{
}

FText FAssetTypeActions_ComboGraphNodeSequence::GetName() const
{
	return LOCTEXT("AssetTypeActions_ComboGraphNodeSequence", "Combo Graph Node (Sequence)");
}

FColor FAssetTypeActions_ComboGraphNodeSequence::GetTypeColor() const
{
	const FLinearColor SequenceColor = FComboGraphUtils::GetPluginProjectSettings()->SequenceNodeColor;
	return FColor(SequenceColor.R * 255, SequenceColor.G * 255, SequenceColor.B * 255);
}

UClass* FAssetTypeActions_ComboGraphNodeSequence::GetSupportedClass() const
{
	return UComboGraphNodeSequenceBlueprint::StaticClass();
}

#undef LOCTEXT_NAMESPACE
