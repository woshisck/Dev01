#include "AssetUtility/StoryFlowAssetFactory.h"

#include "Graph/FlowGraph.h"
#include "Story/Flow/StoryFlowAsset.h"

#define LOCTEXT_NAMESPACE "StoryFlowAssetFactory"

UStoryFlowAssetFactory::UStoryFlowAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStoryFlowAsset::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

FText UStoryFlowAssetFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Story Director Flow");
}

UObject* UStoryFlowAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UStoryFlowAsset* Asset = NewObject<UStoryFlowAsset>(InParent, UStoryFlowAsset::StaticClass(), Name, Flags | RF_Transactional);
	UFlowGraph::CreateGraph(Asset);
	return Asset;
}

#undef LOCTEXT_NAMESPACE
