// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraphEdGraph.h"

#include "ComboGraphEditorTypes.h"
#include "ComboGraphEditorLog.h"
#include "SGraphNode.h"
#include "SGraphPanel.h"
#include "Graph/ComboGraph.h"
#include "Graph/ComboGraphEdge.h"
#include "Graph/ComboGraphNodeBase.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "Graph/Nodes/ComboGraphEdNode.h"
#include "Graph/Nodes/ComboGraphEdNodeConduit.h"
#include "Graph/Nodes/ComboGraphEdNodeEdge.h"
#include "Graph/Nodes/ComboGraphEdNodeEntry.h"

UComboGraph* UComboGraphEdGraph::GetComboGraphModel() const
{
	return CastChecked<UComboGraph>(GetOuter());
}

void UComboGraphEdGraph::RebuildGraph()
{
	CG_EDITOR_LOG(Verbose, TEXT("UComboGraphEdGraph::RebuildGraph has been called. Nodes Num: %d"), Nodes.Num())

	UComboGraph* ComboGraphModel = GetComboGraphModel();
	check(ComboGraphModel)

	Clear();

	for (UEdGraphNode* CurrentNode : Nodes)
	{
		CG_EDITOR_LOG(Verbose, TEXT("UComboGraphEdGraph::RebuildGraph for node: %s (%s)"), *CurrentNode->GetName(), *CurrentNode->GetClass()->GetName())
		if (UComboGraphEdNode* GraphNode = Cast<UComboGraphEdNode>(CurrentNode))
		{
			RebuildGraphForNodeBase(ComboGraphModel, GraphNode);
		}
		else if (UComboGraphEdNodeConduit* ConduitNode = Cast<UComboGraphEdNodeConduit>(CurrentNode))
		{
			RebuildGraphForNodeBase(ComboGraphModel, ConduitNode);
		}
		else if (UComboGraphEdNodeEdge* GraphEdge = Cast<UComboGraphEdNodeEdge>(CurrentNode))
		{
			RebuildGraphForEdge(ComboGraphModel, GraphEdge);
		}
		else if (UComboGraphEdNodeEntry* GraphEntryNode = Cast<UComboGraphEdNodeEntry>(CurrentNode))
		{
			RebuildGraphForEntry(ComboGraphModel, GraphEntryNode);
		}
	}

	for (UComboGraphNodeBase* Node : ComboGraphModel->AllNodes)
	{
		if (Node->ParentNodes.Num() == 0)
		{
			ComboGraphModel->RootNodes.Add(Node);
			// May cause a weird issue, no crash but editor goes unresponsive
			// SortNodes(Node);
		}

		Node->Rename(nullptr, ComboGraphModel, REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	// ComboGraphModel->RootNodes.Sort([&](const UComboGraphNodeBase& Left, const UComboGraphNodeBase& Right)
	// {
	// 	const UComboGraphEdNodeBase* EdNodeLeft = NodeMap[&Left];
	// 	const UComboGraphEdNodeBase* EdNodeRight = NodeMap[&Right];
	// 	return EdNodeLeft->NodePosX < EdNodeRight->NodePosX;
	// });
}

void UComboGraphEdGraph::RebuildGraphForEdge(UComboGraph* OwningGraph, UComboGraphEdNodeEdge* EdGraphEdge)
{
	UComboGraphEdNodeBase* StartNode = EdGraphEdge->GetStartNodeAsBase();
	UComboGraphEdNode* EndNode = EdGraphEdge->GetEndNode();
	UComboGraphEdge* Edge = EdGraphEdge->RuntimeEdge;

	if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
	{
		CG_EDITOR_LOG(
			Error,
			TEXT("UComboGraphEdGraph::RebuildGraph add edge failed. StartNode: %s, EndNode: %s, Edge: %s"),
			StartNode ? *StartNode->GetName() : TEXT("NONE"),
			EndNode ? *EndNode->GetName() : TEXT("NONE"),
			Edge ? *Edge->GetName() : TEXT("NONE")
		);
		return;
	}

	EdGraphEdge->UpdateCachedIcon();

	EdgeMap.Add(Edge, EdGraphEdge);

	Edge->Rename(nullptr, OwningGraph, REN_DontCreateRedirectors | REN_DoNotDirty);

	if (const UComboGraphEdNode* StartComboNode = Cast<UComboGraphEdNode>(StartNode))
	{
		Edge->StartNode = StartComboNode->RuntimeNode;
	}
	else if (const UComboGraphEdNodeConduit* NodeConduit = Cast<UComboGraphEdNodeConduit>(StartNode))
	{
		Edge->StartNode = NodeConduit->RuntimeNode;
	}

	Edge->EndNode = EndNode->RuntimeNode;

	if (Edge->StartNode)
	{
		Edge->StartNode->Edges.Add(Edge->EndNode, Edge);
	}
}

void UComboGraphEdGraph::RebuildGraphForNodeBase(UComboGraph* OwningGraph, UComboGraphEdNodeBase* NodeBase)
{
	UComboGraphEdNode* Node = Cast<UComboGraphEdNode>(NodeBase);
	UComboGraphEdNodeConduit* Conduit = Cast<UComboGraphEdNodeConduit>(NodeBase);

	UComboGraphNodeBase* RuntimeNode = Node ? Node->RuntimeNode
		: Conduit ? Conduit->RuntimeNode
		: nullptr;

	if (!RuntimeNode)
	{
		return;
	}

	NodeMap.Add(RuntimeNode, NodeBase);

	OwningGraph->AllNodes.Add(RuntimeNode);

	for (UEdGraphPin* Pin : NodeBase->Pins)
	{
		if (Pin->Direction != EGPD_Output)
		{
			continue;
		}

		for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
		{
			UComboGraphNodeBase* ChildNode = nullptr;

			// Try to determine child node
			if (const UComboGraphEdNode* OwningNode = Cast<UComboGraphEdNode>(LinkedTo->GetOwningNode()))
			{
				ChildNode = OwningNode->RuntimeNode;
			}
			else if (const UComboGraphEdNodeEdge* OwningEdge = Cast<UComboGraphEdNodeEdge>(LinkedTo->GetOwningNode()))
			{
				UComboGraphEdNode* EndNode = OwningEdge->GetEndNode();
				if (EndNode)
				{
					ChildNode = EndNode->RuntimeNode;
				}
			}

			// Update child / parent nodes for both node and containing ability graph
			if (ChildNode)
			{
				RuntimeNode->ChildrenNodes.Add(ChildNode);
				ChildNode->ParentNodes.Add(RuntimeNode);
			}
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphEdGraph::RebuildGraphForEntry(UComboGraph* OwningGraph, UComboGraphEdNodeEntry* NodeEntry)
{
	check(NodeEntry);
	check(OwningGraph);

	UComboGraphEdNode* Node = Cast<UComboGraphEdNode>(NodeEntry->GetOutputNode());
	UComboGraphEdNodeConduit* Conduit = Cast<UComboGraphEdNodeConduit>(NodeEntry->GetOutputNode());

	CG_EDITOR_LOG(Verbose, TEXT("UComboGraphEdGraph::RebuildGraphForEntry ... Node: %s"), Node ? *Node->GetName() : TEXT("NONE"))
	CG_EDITOR_LOG(Verbose, TEXT("UComboGraphEdGraph::RebuildGraphForEntry ... Conduit: %s"), Conduit ? *Conduit->GetName() : TEXT("NONE"))

	// Setup Entry node reference (created by schema when first creating the graph)
	OwningGraph->EntryNode = NodeEntry->RuntimeNode;

	// Setup reference to first root combo node, which is the runtime node from either direct combo node or conduit one
	OwningGraph->FirstNode = Node ? Node->RuntimeNode
		: Conduit ? Conduit->RuntimeNode
		: nullptr;
}

void UComboGraphEdGraph::ValidateNodes(FCompilerResultsLog* LogResults)
{
	for (const UEdGraphNode* Node : Nodes)
	{
		if (const UComboGraphEdNode* GraphNode = Cast<UComboGraphEdNode>(Node))
		{
			Node->ValidateNodeDuringCompilation(*LogResults);
		}
	}
}

bool UComboGraphEdGraph::Modify(const bool bAlwaysMarkDirty)
{
	const bool bWasSaved = Super::Modify(bAlwaysMarkDirty);

	UComboGraph* ComboGraphModel = GetComboGraphModel();
	if (ComboGraphModel)
	{
		ComboGraphModel->Modify();
	}

	for (UEdGraphNode* Node : Nodes)
	{
		Node->Modify();
	}

	return bWasSaved;
}

void UComboGraphEdGraph::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
}

TArray<UComboGraphEdNode*> UComboGraphEdGraph::GetAllNodes() const
{
	TArray<UComboGraphEdNode*> OutNodes;
	for (UEdGraphNode* EdNode : Nodes)
	{
		if (UComboGraphEdNode* Node = Cast<UComboGraphEdNode>(EdNode))
		{
			OutNodes.Add(Node);
		}
	}

	return OutNodes;
}

namespace ACEAutoArrangeHelpers
{
	struct FNodeBoundsInfo
	{
		FVector2D SubGraphBBox;
		TArray<FNodeBoundsInfo> Children;
	};

	UEdGraphPin* FindGraphNodePin(UEdGraphNode* Node, const EEdGraphPinDirection Dir)
	{
		UEdGraphPin* Pin = nullptr;
		for (int32 Idx = 0; Idx < Node->Pins.Num(); Idx++)
		{
			if (Node->Pins[Idx]->Direction == Dir)
			{
				Pin = Node->Pins[Idx];
				break;
			}
		}

		return Pin;
	}

	void AutoArrangeNodesVertically(UComboGraphEdNodeBase* ParentNode, FNodeBoundsInfo& BBoxTree, float PosX, float PosY)
	{
		int32 BBoxIndex = 0;

		UEdGraphPin* Pin = FindGraphNodePin(ParentNode, EGPD_Output);
		if (Pin)
		{
			SGraphNode::FNodeSet NodeFilter;
			TArray<UEdGraphPin*> TempLinkedTo = Pin->LinkedTo;
			for (int32 Idx = 0; Idx < TempLinkedTo.Num(); Idx++)
			{
				UComboGraphEdNodeBase* GraphNode = Cast<UComboGraphEdNodeBase>(TempLinkedTo[Idx]->GetOwningNode());
				if (GraphNode && BBoxTree.Children.Num() > 0)
				{
					AutoArrangeNodesVertically(GraphNode, BBoxTree.Children[BBoxIndex], PosX, PosY + GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
					GraphNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FVector2D(BBoxTree.Children[BBoxIndex].SubGraphBBox.X / 2 - GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X / 2 + PosX, PosY), NodeFilter);
					PosX += BBoxTree.Children[BBoxIndex].SubGraphBBox.X + 20;
					BBoxIndex++;
				}

			}
		}
	}

	void AutoArrangeNodesHorizontally(UComboGraphEdNodeBase* ParentNode, FNodeBoundsInfo& BBoxTree, float PosX, float PosY)
	{
		int32 BBoxIndex = 0;

		UEdGraphPin* Pin = FindGraphNodePin(ParentNode, EGPD_Output);
		if (Pin)
		{
			SGraphNode::FNodeSet NodeFilter;
			TArray<UEdGraphPin*> TempLinkedTo = Pin->LinkedTo;
			for (int32 Idx = 0; Idx < TempLinkedTo.Num(); Idx++)
			{
				UComboGraphEdNodeBase* GraphNode = Cast<UComboGraphEdNodeBase>(TempLinkedTo[Idx]->GetOwningNode());
				if (GraphNode && BBoxTree.Children.Num() > 0)
				{
					// AutoArrangeNodesVertically(GraphNode, BBoxTree.Children[BBoxIndex], PosX, PosY + GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
					AutoArrangeNodesHorizontally(GraphNode, BBoxTree.Children[BBoxIndex], PosX + GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X * 2.5f, PosY);

					// GraphNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FVector2D(BBoxTree.Children[BBoxIndex].SubGraphBBox.X / 2 - GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X / 2 + PosX, PosY), NodeFilter);
					GraphNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FVector2D(PosX, BBoxTree.Children[BBoxIndex].SubGraphBBox.Y / 2 - GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y / 2 + PosY), NodeFilter);

					// PosX += BBoxTree.Children[BBoxIndex].SubGraphBBox.X + 20;
					PosY += BBoxTree.Children[BBoxIndex].SubGraphBBox.Y + 20;
					BBoxIndex++;
				}

			}
		}
	}

	void GetNodeSizeInfo(UComboGraphEdNodeBase* ParentNode, FNodeBoundsInfo& BBoxTree, TArray<UComboGraphEdNodeBase*>& VisitedNodes)
	{
		BBoxTree.SubGraphBBox = ParentNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize();
		float LevelWidth = 0;
		float LevelHeight = 0;

		UEdGraphPin* Pin = FindGraphNodePin(ParentNode, EGPD_Output);
		if (Pin)
		{
			Pin->LinkedTo.Sort(FComboGraphCompareNodeXLocation());
			for (int32 Idx = 0; Idx < Pin->LinkedTo.Num(); Idx++)
			{
				UComboGraphEdNodeBase* GraphNode = Cast<UComboGraphEdNodeBase>(Pin->LinkedTo[Idx]->GetOwningNode());
				if (GraphNode)
				{
					const int32 ChildIdx = BBoxTree.Children.Add(FNodeBoundsInfo());
					FNodeBoundsInfo& ChildBounds = BBoxTree.Children[ChildIdx];

					if (!VisitedNodes.Contains(GraphNode))
					{
						VisitedNodes.AddUnique(GraphNode);
						GetNodeSizeInfo(GraphNode, ChildBounds, VisitedNodes);
					}

					LevelWidth += ChildBounds.SubGraphBBox.X + 20;
					if (ChildBounds.SubGraphBBox.Y > LevelHeight)
					{
						LevelHeight = ChildBounds.SubGraphBBox.Y;
					}
				}
			}

			if (LevelWidth > BBoxTree.SubGraphBBox.X)
			{
				BBoxTree.SubGraphBBox.X = LevelWidth;
			}

			BBoxTree.SubGraphBBox.Y += LevelHeight;
		}
	}
}


void UComboGraphEdGraph::AutoArrange(const bool bVertical)
{
	UComboGraphEdNodeBase* RootNode = nullptr;
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		RootNode = Cast<UComboGraphEdNodeEntry>(Nodes[Idx]);
		if (RootNode)
		{
			break;
		}
	}

	if (!RootNode)
	{
		return;
	}

	CG_EDITOR_LOG(Verbose, TEXT("UComboGraphEdGraph::AutoArrange Strategy: %s"), bVertical ? TEXT("Vertical") : TEXT("Horizontal"))
	const FScopedTransaction Transaction(NSLOCTEXT("ACEGraph", "CombatGraphEditorAutoArrange", "Combat Graph Editor: Auto Arrange"));

	ACEAutoArrangeHelpers::FNodeBoundsInfo BBoxTree;
	TArray<UComboGraphEdNodeBase*> VisitedNodes;
	ACEAutoArrangeHelpers::GetNodeSizeInfo(RootNode, BBoxTree, VisitedNodes);

	SGraphNode::FNodeSet NodeFilter;
	if (bVertical)
	{
		ACEAutoArrangeHelpers::AutoArrangeNodesVertically(RootNode, BBoxTree, 0, RootNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);

		const float NewRootPosX = BBoxTree.SubGraphBBox.X / 2 - RootNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X / 2;
		RootNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FVector2D(NewRootPosX, 0), NodeFilter);
	}
	else
	{
		ACEAutoArrangeHelpers::AutoArrangeNodesHorizontally(RootNode, BBoxTree, RootNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X * 2.5f, 0);

		const float NewRootPosY = BBoxTree.SubGraphBBox.Y / 2 - RootNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y / 2;
		RootNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FVector2D(0, NewRootPosY), NodeFilter);
	}

	RootNode->DEPRECATED_NodeWidget.Pin()->GetOwnerPanel()->ZoomToFit(/*bOnlySelection=*/ false);
}

void UComboGraphEdGraph::Clear()
{
	UComboGraph* ComboGraphModel = GetComboGraphModel();
	if (ComboGraphModel)
	{
		ComboGraphModel->ClearGraph();
	}

	NodeMap.Reset();
	EdgeMap.Reset();

	for (UEdGraphNode* Node : Nodes)
	{
		if (const UComboGraphEdNode* EdGraphNode = Cast<UComboGraphEdNode>(Node))
		{
			UComboGraphNodeBase* GraphNode = EdGraphNode->RuntimeNode;
			if (GraphNode)
			{
				GraphNode->ParentNodes.Reset();
				GraphNode->ChildrenNodes.Reset();
				GraphNode->Edges.Reset();
			}
		}
	}
}

void UComboGraphEdGraph::SortNodes(UComboGraphNodeBase* RootNode)
{
	TArray<UComboGraphNodeBase*> CurrLevelNodes = { RootNode };
	TArray<UComboGraphNodeBase*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (UComboGraphNodeBase* Node : CurrLevelNodes)
		{
			auto Comp = [&](const UComboGraphNodeBase& L, const UComboGraphNodeBase& R)
			{
				const UComboGraphEdNodeBase* EdNode_LNode = NodeMap[&L];
				const UComboGraphEdNodeBase* EdNode_RNode = NodeMap[&R];
				return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
			};

			Node->ChildrenNodes.Sort(Comp);
			Node->ParentNodes.Sort(Comp);

			NextLevelNodes.Append(Node->ChildrenNodes);
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
	}
}
