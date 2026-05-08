#pragma once

#include "CoreMinimal.h"

class UClass;
class UFlowAsset;
class UFlowNode;

struct FRuneEditorFlowNodeSummary
{
	TWeakObjectPtr<UFlowNode> Node;
	FText DisplayName;
	FString ClassName;
	FString Description;
	FString OutgoingSummary;
	FVector2D GraphPosition = FVector2D::ZeroVector;
	bool bDefaultEntryNode = false;
};

class FRuneEditorFlowAuthoring
{
public:
	static TArray<FRuneEditorFlowNodeSummary> BuildFlowNodeSummaries(UFlowAsset* FlowAsset);
	static UFlowNode* AddNodeAfter(UFlowAsset* FlowAsset, UFlowNode* AfterNode, UClass* NodeClass, FText& OutMessage);
	static FText RelinkEntryToNode(UFlowAsset* FlowAsset, UFlowNode* TargetNode);
	static FText RelinkNodes(UFlowAsset* FlowAsset, UFlowNode* FromNode, UFlowNode* ToNode);

private:
	static void FinalizeFlowGraph(UFlowAsset* FlowAsset);
};
