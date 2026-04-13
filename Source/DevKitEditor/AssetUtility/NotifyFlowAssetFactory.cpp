#include "AssetUtility/NotifyFlowAssetFactory.h"
#include "BuffFlow/NotifyFlowAsset.h"
#include "Graph/FlowGraph.h"

#define LOCTEXT_NAMESPACE "NotifyFlowAssetFactory"

UNotifyFlowAssetFactory::UNotifyFlowAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass  = UNotifyFlowAsset::StaticClass();
	bCreateNew      = true;
	bEditorImport   = false;
	bEditAfterNew   = true;
}

FText UNotifyFlowAssetFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Notify Flow Asset");
}

UObject* UNotifyFlowAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UNotifyFlowAsset* Asset = NewObject<UNotifyFlowAsset>(InParent, UNotifyFlowAsset::StaticClass(), Name, Flags | RF_Transactional);
	UFlowGraph::CreateGraph(Asset);
	return Asset;
}

#undef LOCTEXT_NAMESPACE
