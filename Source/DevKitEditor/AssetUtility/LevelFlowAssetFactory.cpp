#include "AssetUtility/LevelFlowAssetFactory.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Graph/FlowGraph.h"

#define LOCTEXT_NAMESPACE "LevelFlowAssetFactory"

ULevelFlowAssetFactory::ULevelFlowAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass  = ULevelFlowAsset::StaticClass();
	bCreateNew      = true;
	bEditorImport   = false;
	bEditAfterNew   = true;
}

FText ULevelFlowAssetFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Level Event Flow");
}

UObject* ULevelFlowAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULevelFlowAsset* Asset = NewObject<ULevelFlowAsset>(InParent, ULevelFlowAsset::StaticClass(), Name, Flags | RF_Transactional);
	UFlowGraph::CreateGraph(Asset);
	return Asset;
}

#undef LOCTEXT_NAMESPACE
