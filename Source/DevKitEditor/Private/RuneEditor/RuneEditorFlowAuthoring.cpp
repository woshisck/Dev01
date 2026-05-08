#include "RuneEditor/RuneEditorFlowAuthoring.h"

#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"

#define LOCTEXT_NAMESPACE "RuneEditorFlowAuthoring"

namespace
{
	UFlowGraphNode* GetGraphNode(UFlowNode* Node)
	{
		return Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
	}

	FString GetNodeDisplayName(UFlowNode* Node)
	{
		return Node ? Node->GetNodeTitle().ToString() : FString();
	}

	FString BuildOutgoingSummary(UFlowNode* Node)
	{
		const UFlowGraphNode* GraphNode = GetGraphNode(Node);
		if (!GraphNode)
		{
			return TEXT("No graph node");
		}

		TArray<FString> LinkedNames;
		for (const UEdGraphPin* Pin : GraphNode->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				const UFlowGraphNode* LinkedGraphNode = LinkedPin ? Cast<UFlowGraphNode>(LinkedPin->GetOwningNode()) : nullptr;
				UFlowNode* LinkedFlowNode = LinkedGraphNode ? Cast<UFlowNode>(LinkedGraphNode->GetFlowNodeBase()) : nullptr;
				if (LinkedFlowNode)
				{
					LinkedNames.AddUnique(GetNodeDisplayName(LinkedFlowNode));
				}
			}
		}

		return LinkedNames.Num() > 0 ? FString::Join(LinkedNames, TEXT(" / ")) : TEXT("No outgoing links");
	}

	UEdGraphPin* FindFirstOutputPin(UFlowNode* Node)
	{
		UFlowGraphNode* GraphNode = GetGraphNode(Node);
		return GraphNode ? GraphNode->GetOutputPin(0) : nullptr;
	}

	bool TryRelinkFirstCompatiblePins(UFlowNode* FromNode, UFlowNode* ToNode)
	{
		UFlowGraphNode* FromGraphNode = GetGraphNode(FromNode);
		UFlowGraphNode* ToGraphNode = GetGraphNode(ToNode);
		if (!FromGraphNode || !ToGraphNode)
		{
			return false;
		}

		const UEdGraphSchema* Schema = FromGraphNode->GetSchema();
		if (!Schema)
		{
			return false;
		}

		for (UEdGraphPin* FromPin : FromGraphNode->Pins)
		{
			if (!FromPin || FromPin->Direction != EGPD_Output)
			{
				continue;
			}

			for (UEdGraphPin* ToPin : ToGraphNode->Pins)
			{
				if (!ToPin || ToPin->Direction != EGPD_Input)
				{
					continue;
				}

				const ECanCreateConnectionResponse Response = Schema->CanCreateConnection(FromPin, ToPin).Response;
				if (Response == CONNECT_RESPONSE_MAKE || Response == CONNECT_RESPONSE_BREAK_OTHERS_A || Response == CONNECT_RESPONSE_BREAK_OTHERS_B)
				{
					FromPin->Modify();
					ToPin->Modify();
					FromPin->BreakAllPinLinks();
					ToPin->BreakAllPinLinks();
					if (Schema->TryCreateConnection(FromPin, ToPin))
					{
						FromGraphNode->NodeConnectionListChanged();
						ToGraphNode->NodeConnectionListChanged();
						return true;
					}
				}
			}
		}

		return false;
	}

	FVector2D GetNextNodeLocation(UFlowAsset* FlowAsset, UFlowNode* AfterNode)
	{
		if (const UFlowGraphNode* AfterGraphNode = GetGraphNode(AfterNode))
		{
			return FVector2D(static_cast<float>(AfterGraphNode->NodePosX + 340), static_cast<float>(AfterGraphNode->NodePosY));
		}

		int32 MaxX = 0;
		int32 NodeCount = 0;
		if (FlowAsset)
		{
			for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
			{
				if (const UFlowGraphNode* GraphNode = GetGraphNode(Pair.Value))
				{
					MaxX = FMath::Max(MaxX, GraphNode->NodePosX);
					++NodeCount;
				}
			}
		}
		return FVector2D(static_cast<float>(MaxX + 340), static_cast<float>(NodeCount * 90));
	}
}

TArray<FRuneEditorFlowNodeSummary> FRuneEditorFlowAuthoring::BuildFlowNodeSummaries(UFlowAsset* FlowAsset)
{
	TArray<FRuneEditorFlowNodeSummary> Summaries;
	if (!FlowAsset)
	{
		return Summaries;
	}

	FlowAsset->HarvestNodeConnections();

	UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
	TArray<UFlowNode*> OrderedNodes;
	FlowAsset->GetNodesInExecutionOrder<UFlowNode>(EntryNode, OrderedNodes);

	TSet<FGuid> AddedGuids;
	auto AddNodeSummary = [&](UFlowNode* Node)
	{
		if (!Node || AddedGuids.Contains(Node->GetGuid()))
		{
			return;
		}

		FRuneEditorFlowNodeSummary Summary;
		Summary.Node = Node;
		Summary.DisplayName = Node->GetNodeTitle();
		Summary.ClassName = Node->GetClass()->GetName();
		Summary.Description = Node->GetNodeDescription();
		Summary.OutgoingSummary = BuildOutgoingSummary(Node);
		Summary.bDefaultEntryNode = Node == EntryNode;

		if (const UFlowGraphNode* GraphNode = GetGraphNode(Node))
		{
			Summary.GraphPosition = FVector2D(static_cast<float>(GraphNode->NodePosX), static_cast<float>(GraphNode->NodePosY));
		}

		Summaries.Add(Summary);
		AddedGuids.Add(Node->GetGuid());
	};

	for (UFlowNode* Node : OrderedNodes)
	{
		AddNodeSummary(Node);
	}

	TArray<UFlowNode*> RemainingNodes;
	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (Pair.Value && !AddedGuids.Contains(Pair.Key))
		{
			RemainingNodes.Add(Pair.Value);
		}
	}
	RemainingNodes.Sort([](const UFlowNode& A, const UFlowNode& B)
	{
		const UFlowGraphNode* GraphNodeA = GetGraphNode(const_cast<UFlowNode*>(&A));
		const UFlowGraphNode* GraphNodeB = GetGraphNode(const_cast<UFlowNode*>(&B));
		const int32 AX = GraphNodeA ? GraphNodeA->NodePosX : 0;
		const int32 BX = GraphNodeB ? GraphNodeB->NodePosX : 0;
		if (AX != BX)
		{
			return AX < BX;
		}
		return A.GetName() < B.GetName();
	});

	for (UFlowNode* Node : RemainingNodes)
	{
		AddNodeSummary(Node);
	}

	return Summaries;
}

UFlowNode* FRuneEditorFlowAuthoring::AddNodeAfter(UFlowAsset* FlowAsset, UFlowNode* AfterNode, UClass* NodeClass, FText& OutMessage)
{
	if (!FlowAsset)
	{
		OutMessage = LOCTEXT("AddNodeMissingFlow", "Cannot add node: selected rune has no FlowAsset.");
		return nullptr;
	}
	if (!NodeClass)
	{
		OutMessage = LOCTEXT("AddNodeMissingClass", "Cannot add node: no catalog node selected.");
		return nullptr;
	}

	FText FailureReason;
	if (!FlowAsset->IsNodeOrAddOnClassAllowed(NodeClass, &FailureReason))
	{
		OutMessage = FText::Format(LOCTEXT("AddNodeClassRejected", "Cannot add node: {0}"), FailureReason);
		return nullptr;
	}

	UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	if (!FlowGraph)
	{
		OutMessage = LOCTEXT("AddNodeMissingGraph", "Cannot add node: FlowAsset has no FlowGraph.");
		return nullptr;
	}

	UFlowNode* AnchorNode = AfterNode ? AfterNode : FlowAsset->GetDefaultEntryNode();
	UEdGraphPin* FromPin = FindFirstOutputPin(AnchorNode);
	const FVector2D NodeLocation = GetNextNodeLocation(FlowAsset, AnchorNode);
	UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, FromPin, NodeClass, NodeLocation, false);
	UFlowNode* NewNode = NewGraphNode ? Cast<UFlowNode>(NewGraphNode->GetFlowNodeBase()) : nullptr;
	if (!NewNode)
	{
		OutMessage = LOCTEXT("AddNodeFailed", "Failed to add Flow node.");
		return nullptr;
	}

	FinalizeFlowGraph(FlowAsset);
	OutMessage = FText::Format(
		LOCTEXT("AddNodeSucceeded", "Added Flow node {0}."),
		NewNode->GetNodeTitle());
	return NewNode;
}

FText FRuneEditorFlowAuthoring::RelinkEntryToNode(UFlowAsset* FlowAsset, UFlowNode* TargetNode)
{
	if (!FlowAsset)
	{
		return LOCTEXT("RelinkEntryMissingFlow", "Cannot relink: selected rune has no FlowAsset.");
	}

	UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
	if (!EntryNode || !TargetNode || EntryNode == TargetNode)
	{
		return LOCTEXT("RelinkEntryMissingNode", "Select a non-entry Flow node to connect from Entry.");
	}

	return RelinkNodes(FlowAsset, EntryNode, TargetNode);
}

FText FRuneEditorFlowAuthoring::RelinkNodes(UFlowAsset* FlowAsset, UFlowNode* FromNode, UFlowNode* ToNode)
{
	if (!FlowAsset || !FromNode || !ToNode)
	{
		return LOCTEXT("RelinkMissingNode", "Cannot relink: missing source or target Flow node.");
	}

	if (!TryRelinkFirstCompatiblePins(FromNode, ToNode))
	{
		return LOCTEXT("RelinkFailed", "No compatible output/input pins were found for selected nodes.");
	}

	FinalizeFlowGraph(FlowAsset);
	return FText::Format(
		LOCTEXT("RelinkSucceeded", "Linked {0} -> {1}."),
		FromNode->GetNodeTitle(),
		ToNode->GetNodeTitle());
}

void FRuneEditorFlowAuthoring::FinalizeFlowGraph(UFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return;
	}

	FlowAsset->HarvestNodeConnections();
	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();

	if (UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph()))
	{
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();
	}
}

#undef LOCTEXT_NAMESPACE
