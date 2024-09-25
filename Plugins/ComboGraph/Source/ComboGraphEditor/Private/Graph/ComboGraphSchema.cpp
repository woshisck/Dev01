// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraphSchema.h"

#include "ComboGraphEditorLog.h"
#include "ComboGraphEditorTypes.h"
#include "EdGraphNode_Comment.h"
#include "InputAction.h"
#include "Animation/AnimMontage.h"
#include "Graph/ComboGraphEdge.h"
#include "Graph/ComboGraphNodeBase.h"
#include "Graph/ComboGraphNodeMontage.h"
#include "Graph/ComboGraphNodeSequence.h"
#include "Graph/ComboGraphConnectionDrawingPolicy.h"
#include "Graph/ComboGraphEdGraph.h"
#include "Graph/ComboGraphNodeConduit.h"
#include "Graph/ComboGraphNodeEntry.h"
#include "Graph/Nodes/ComboGraphEdNodeEdge.h"
#include "Graph/Nodes/ComboGraphEdNode.h"
#include "Graph/Nodes/ComboGraphEdNodeConduit.h"
#include "Graph/Nodes/ComboGraphEdNodeEntry.h"
#include "Graph/Slate/SComboGraphNode.h"

#define LOCTEXT_NAMESPACE "CombatAbilityGraphSchema"

int32 UComboGraphSchema::CurrentCacheRefreshID = 0;

class FACENodeVisitorCycleChecker
{
public:
	/** Check whether a loop in the graph would be caused by linking the passed-in nodes */
	bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
	{
		VisitedNodes.Add(StartNode);
		return TraverseNodes(EndNode);
	}

private:
	TSet<UEdGraphNode*> VisitedNodes;
	TSet<UEdGraphNode*> FinishedNodes;

	bool TraverseNodes(UEdGraphNode* Node)
	{
		VisitedNodes.Add(Node);

		for (UEdGraphPin* MyPin : Node->Pins)
		{
			if (MyPin->Direction == EGPD_Output)
			{
				for (const UEdGraphPin* OtherPin : MyPin->LinkedTo)
				{
					UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
					if (VisitedNodes.Contains(OtherNode))
					{
						// Only  an issue if this is a back-edge
						return false;
					}

					if (!FinishedNodes.Contains(OtherNode) && !TraverseNodes(OtherNode))
					{
						// Only should traverse if this node hasn't been traversed
						return false;
					}
				}
			}
		}

		VisitedNodes.Remove(Node);
		FinishedNodes.Add(Node);
		return true;
	};
};

/////////////////////////////////////////////////////
// FComboGraphSchemaAction_AddComment

UEdGraphNode* FComboGraphSchemaAction_AddComment::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode_Comment* const CommentTemplate = NewObject<UEdGraphNode_Comment>();

	FVector2D SpawnLocation = Location;
	FSlateRect Bounds;

	const TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(ParentGraph);
	if (GraphEditorPtr.IsValid() && GraphEditorPtr->GetBoundsForSelectedNodes(/*out*/ Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation.X = CommentTemplate->NodePosX;
		SpawnLocation.Y = CommentTemplate->NodePosY;
	}

	UEdGraphNode* const NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation, bSelectNewNode);

	return NewNode;
}

/////////////////////////////////////////////////////
// FComboGraphSchemaAction_NewNode

UEdGraphNode* FComboGraphSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, const bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("CombatGraphEditorNewNode", "Combat Graph Editor: New Node"));
		ParentGraph->Modify();
		if (FromPin)
		{
			FromPin->Modify();
		}

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		NodeTemplate->RuntimeNode->SetFlags(RF_Transactional);
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

void FComboGraphSchemaAction_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

/////////////////////////////////////////////////////
// FComboGraphSchemaAction_NewEdge

UEdGraphNode* FComboGraphSchemaAction_NewEdge::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("CombatGraphEditorNewEdge", "Combat Graph Editor: New Edge"));
		ParentGraph->Modify();
		if (FromPin)
		{
			FromPin->Modify();
		}

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		NodeTemplate->RuntimeEdge->SetFlags(RF_Transactional);
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

void FComboGraphSchemaAction_NewEdge::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

/////////////////////////////////////////////////////
// FComboGraphSchemaAction_NewNodeConduit

UEdGraphNode* FComboGraphSchemaAction_NewNodeConduit::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	// If there is a template, we actually use it
	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("CombatGraphEditorNewConduit", "Combat Graph Editor: New Conduit"));
		ParentGraph->Modify();
		if (FromPin)
		{
			FromPin->Modify();
		}

		// set outer to be the graph so it doesn't go away
		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		ResultNode = NodeTemplate;

		ResultNode->SetFlags(RF_Transactional);
	}

	return ResultNode;
}

void FComboGraphSchemaAction_NewNodeConduit::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

/////////////////////////////////////////////////////
// FComboGraphSchemaAction_AutoArrangeVertical

UEdGraphNode* FComboGraphSchemaAction_AutoArrangeVertical::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UComboGraphEdGraph* Graph = Cast<UComboGraphEdGraph>(ParentGraph);
	if (Graph)
	{
		CG_EDITOR_LOG(Verbose, TEXT("FComboGraphSchemaAction_AutoArrangeVertical::PerformAction"))
		Graph->AutoArrange(true);
	}

	return nullptr;
}

/////////////////////////////////////////////////////
// FComboGraphSchemaAction_AutoArrangeHorizontal

UEdGraphNode* FComboGraphSchemaAction_AutoArrangeHorizontal::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UComboGraphEdGraph* Graph = Cast<UComboGraphEdGraph>(ParentGraph);
	if (Graph)
	{
		CG_EDITOR_LOG(Verbose, TEXT("FComboGraphSchemaAction_AutoArrangeVertical::PerformAction Horizontal"))
		Graph->AutoArrange(false);
	}

	return nullptr;
}


/////////////////////////////////////////////////////
// UComboGraphSchema

void UComboGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UComboGraph* ComboGraph = Cast<UComboGraph>(Graph.GetOuter());
	CG_EDITOR_LOG(Verbose, TEXT("CreateDefaultNodesForGraph - Graph, Outer Combo Graph: %s"), *GetNameSafe(ComboGraph))

	// Create the entry/exit tunnels
	FGraphNodeCreator<UComboGraphEdNodeEntry> NodeCreator(Graph);
	UComboGraphEdNodeEntry* EntryNode = NodeCreator.CreateNode();
	NodeCreator.Finalize();
	SetNodeMetaData(EntryNode, FNodeMetadata::DefaultGraphNode);

	// Create runtime node for this editor node. Entry nodes gets a bare bone anim base one as well with blank anim related info.
	EntryNode->RuntimeNode = NewObject<UComboGraphNodeEntry>(ComboGraph, UComboGraphNodeEntry::StaticClass());

	if (UComboGraphEdGraph* EdComboGraph = CastChecked<UComboGraphEdGraph>(&Graph))
	{
		EdComboGraph->EntryNode = EntryNode;
	}
}

EGraphType UComboGraphSchema::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_StateMachine;
}

void UComboGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	Super::GetGraphContextActions(ContextMenuBuilder);
	const bool bHasNoParent = (ContextMenuBuilder.FromPin == nullptr);

	// First build up non anim related actions
	if (bHasNoParent)
	{
		const TSharedPtr<FComboGraphSchemaAction_AutoArrangeVertical> ActionAutoArrangeVertical(new FComboGraphSchemaAction_AutoArrangeVertical(
			LOCTEXT("AutoArrangeNodeCategory", "Auto Arrange"),
			LOCTEXT("AutoArrangeVertical", "Auto Arrange (Vertically)"),
			LOCTEXT("AutoArrangeVertically_ToolTip", "Re-arrange graph layout vertically (from top to bottom). Similar to Behavior Trees."),
			0
		));

		const TSharedPtr<FComboGraphSchemaAction_AutoArrangeHorizontal> ActionAutoArrangeHorizontal(new FComboGraphSchemaAction_AutoArrangeHorizontal(
			LOCTEXT("AutoArrangeNodeCategory", "Auto Arrange"),
			LOCTEXT("AutoArrangeHorizontal", "Auto Arrange (Horizontally)"),
			LOCTEXT("AutoArrangeHorizontally_ToolTip", "Re-arrange graph layout horizontally (from left to right). This is far from perfect, but still handy. Will be revisited and improved upon in a future update."),
			0
		));

		ContextMenuBuilder.AddAction(ActionAutoArrangeVertical);
		ContextMenuBuilder.AddAction(ActionAutoArrangeHorizontal);
	}
	else if (const UComboGraphEdNodeEntry* EntryNode = Cast<UComboGraphEdNodeEntry>(ContextMenuBuilder.FromPin->GetOwningNode()))
	{
		// Add conduit only if it is directly after entry node
		CG_EDITOR_LOG(Verbose, TEXT("GetGraphContextActions -- ContextMenuBuilder.FromPin is Entry Node, add Conduit to actions (%s)"), *EntryNode->GetName())

		const TSharedPtr<FComboGraphSchemaAction_NewNodeConduit> Action(new FComboGraphSchemaAction_NewNodeConduit(
			LOCTEXT("CombatGraphNodeAction", "Combat Graph Node"),
			 LOCTEXT("AddConduit", "Add Conduit..."),
			LOCTEXT("AddConduit", "A new conduit state (suitable to use to branch out combos after entry point based on an initial Transition Input)"),
			1
		));
		Action->NodeTemplate = NewObject<UComboGraphEdNodeConduit>(ContextMenuBuilder.OwnerOfTemporaries);
		Action->NodeTemplate->RuntimeNode = NewObject<UComboGraphNodeConduit>(Action->NodeTemplate);
		ContextMenuBuilder.AddAction(Action);
	}

	UComboGraph* ComboGraph = CastChecked<UComboGraph>(ContextMenuBuilder.CurrentGraph->GetOuter());
	// Now, build up actions for anim nodes
	if (!ComboGraph || ComboGraph->NodeType == nullptr)
	{
		return;
	}

	TSet<TSubclassOf<UComboGraphNodeBase>> Visited;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(ComboGraph->NodeType) && !It->HasAnyClassFlags(CLASS_Abstract) && !Visited.Contains(*It))
		{
			TSubclassOf<UComboGraphNodeBase> NodeType = *It;
			if (It->GetName().StartsWith("REINST") || It->GetName().StartsWith("SKEL"))
			{
				continue;
			}

			if (NodeType.GetDefaultObject()->ContextMenuName.IsEmpty())
			{
				continue;
			}

			CG_EDITOR_LOG(Verbose, TEXT("GetGraphContextActions - Create action from %s"), *NodeType->GetName())
			CreateAndAddActionToContextMenu(ContextMenuBuilder, NodeType);
			Visited.Add(NodeType);
		}
	}
}

void UComboGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetContextMenuActions(Menu, Context);
}

FLinearColor UComboGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	// light grey
	return FLinearColor(0.101961f, 0.101961f, 0.101961f, 1.0f);
}

// ReSharper disable once CppConstValueFunctionReturnType
const FPinConnectionResponse UComboGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Either one or the other is invalid
	if (!PinA || !PinB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinInvalid", "Invalid pins"));
	}

	UEdGraphNode* OwningNodeA = PinA->GetOwningNode();
	UEdGraphNode* OwningNodeB = PinB->GetOwningNode();

	// Either one or the other is invalid
	if (!OwningNodeA || !OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinInvalidNodes", "Invalid nodes"));
	}

	// Make sure the pins are not on the same node
	if (OwningNodeA == OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Can't connect node to itself"));
	}


	UComboGraphEdNode* ComboGraphNodeIn = Cast<UComboGraphEdNode>(OwningNodeA);
	UComboGraphEdNode* ComboGraphNodeOut = Cast<UComboGraphEdNode>(OwningNodeB);

	// Allow wiring from Entry node to ACEGraphNode
	if (OwningNodeA->IsA(UComboGraphEdNodeEntry::StaticClass()) && ComboGraphNodeOut)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("PinConnectEntryToNode", "Connect entry node to combo node."));
	}

	// Allow wiring from Entry node to Conduit Node
	if (OwningNodeA->IsA(UComboGraphEdNodeEntry::StaticClass()) && OwningNodeB->IsA(UComboGraphEdNodeConduit::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("PinConnectEntryToConduit", "Connect entry node to conduit."));
	}

	// Allow wiring from Conduit node to combo nodes
	if (OwningNodeA->IsA(UComboGraphEdNodeConduit::StaticClass()) && ComboGraphNodeOut)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnectEntryToConduit", "Connect conduit node to combo node with Transition Input."));
	}

	// Disallow wiring onto an entry node
	if (OwningNodeB->IsA(UComboGraphEdNodeEntry::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorToEntryNode", "Cannot connect to an entry node (can only create connections **from** entry node)"));
	}

	if (ComboGraphNodeOut == nullptr || ComboGraphNodeIn == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError", "Not a valid Combo Graph Node"));
	}

	// Determine if we can have cycles or not
	bool bAllowCycles = false;
	if (const UComboGraph* ComboGraph = Cast<UComboGraph>(OwningNodeA->GetGraph()->GetOuter()))
	{
		bAllowCycles = ComboGraph->bCanBeCyclical;
	}

	// check for cycles
	FACENodeVisitorCycleChecker CycleChecker;
	if (!bAllowCycles && !CycleChecker.CheckForLoop(OwningNodeA, OwningNodeB))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorCycle", "Can't create a graph cycle"));
	}

	FText ErrorMessage;
	if (!ComboGraphNodeIn->RuntimeNode->CanCreateConnectionTo(ComboGraphNodeOut->RuntimeNode, ComboGraphNodeIn->GetOutputPin()->LinkedTo.Num(), ErrorMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
	}

	if (!ComboGraphNodeOut->RuntimeNode->CanCreateConnectionFrom(ComboGraphNodeIn->RuntimeNode, ComboGraphNodeOut->GetInputPin()->LinkedTo.Num(), ErrorMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnect", "Connect nodes with Transition Input"));

}

bool UComboGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	// return Super::TryCreateConnection(A, B);

	// We don't actually care about the pin, we want the node that is being dragged between
	UComboGraphEdNodeBase* NodeA = Cast<UComboGraphEdNodeBase>(A->GetOwningNode());
	UComboGraphEdNodeBase* NodeB = Cast<UComboGraphEdNodeBase>(B->GetOwningNode());

	// Check that this edge doesn't already exist
	for (const UEdGraphPin* TestPin : NodeA->GetOutputPin()->LinkedTo)
	{
		UEdGraphNode* ChildNode = TestPin->GetOwningNode();
		if (const UComboGraphEdNodeEdge* EdNode_Edge = Cast<UComboGraphEdNodeEdge>(ChildNode))
		{
			ChildNode = EdNode_Edge->GetEndNode();
		}

		if (ChildNode == NodeB)
		{
			return false;
		}
	}

	if (NodeA && NodeB)
	{
		// Always create connections from node A to B, don't allow adding in reverse
		const bool bModified = Super::TryCreateConnection(NodeA->GetOutputPin(), NodeB->GetInputPin());
		// if (bModified)
		// {
		// 	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(NodeA);
		// 	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		// }

		return bModified;
	}

	return false;
}

bool UComboGraphSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const
{
	UComboGraphEdNode* NodeA = Cast<UComboGraphEdNode>(A->GetOwningNode());
	UComboGraphEdNode* NodeB = Cast<UComboGraphEdNode>(B->GetOwningNode());

	// Handle conversion from conduit to Combo Graph Nodes
	if (const UComboGraphEdNodeConduit* ConduitNode = Cast<UComboGraphEdNodeConduit>(A->GetOwningNode()))
	{
		if (NodeB && NodeB->GetInputPin())
		{
			CG_EDITOR_LOG(Verbose, TEXT("CreateAutomaticConversionNodeAndConnections for conduit"));
			CreateEdgeConnection(A, B, ConduitNode, NodeB);
			return true;
		}
	}

	// Are nodes and pins all valid?
	if (!NodeA || !NodeA->GetOutputPin() || !NodeB || !NodeB->GetInputPin())
	{
		CG_EDITOR_LOG(Verbose, TEXT("CreateAutomaticConversionNodeAndConnections failed"));
		return false;
	}

	CG_EDITOR_LOG(Verbose, TEXT("CreateAutomaticConversionNodeAndConnections ok"));
	CreateEdgeConnection(A, B, NodeA, NodeB);

	return true;
}

FConnectionDrawingPolicy* UComboGraphSchema::CreateConnectionDrawingPolicy(const int32 InBackLayerID, const int32 InFrontLayerID, const float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FComboGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

void UComboGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));
	Super::BreakNodeLinks(TargetNode);

	// UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(&TargetNode);
	// FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UComboGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, const bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
	// UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(TargetPin.GetOwningNode());
	// FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UComboGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));
	Super::BreakSinglePinLink(SourcePin, TargetPin);

	// UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(TargetPin->GetOwningNode());
	// FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

UEdGraphPin* UComboGraphSchema::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePin, EEdGraphPinDirection InSourcePinDirection) const
{
	const UComboGraphEdNode* EdGraphNode = Cast<UComboGraphEdNode>(InTargetNode);
	switch (InSourcePinDirection)
	{
	case EGPD_Input:
		return EdGraphNode->GetOutputPin();
	case EGPD_Output:
		return EdGraphNode->GetInputPin();
	default:
		return nullptr;
	}
}

bool UComboGraphSchema::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	return Cast<UComboGraphEdNode>(InTargetNode) != nullptr;
}

bool UComboGraphSchema::IsCacheVisualizationOutOfDate(const int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UComboGraphSchema::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UComboGraphSchema::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
}

void UComboGraphSchema::GetGraphDisplayInformation(const UEdGraph& Graph, FGraphDisplayInfo& DisplayInfo) const
{
	Super::GetGraphDisplayInformation(Graph, DisplayInfo);
	DisplayInfo.Tooltip = LOCTEXT("DisplayInfo_Tooltip", "Combo Graph");
}

void UComboGraphSchema::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	Super::DroppedAssetsOnGraph(Assets, GraphPosition, Graph);
	if (Graph != nullptr)
	{
		if (UAnimationAsset* AnimationAsset = FAssetData::GetFirstAsset<UAnimationAsset>(Assets))
		{
			SpawnNodeFromAsset(AnimationAsset, GraphPosition, Graph, nullptr);
		}
	}
}

void UComboGraphSchema::DroppedAssetsOnNode(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphNode* Node) const
{
	Super::DroppedAssetsOnNode(Assets, GraphPosition, Node);

	UAnimationAsset* Asset = FAssetData::GetFirstAsset<UAnimationAsset>(Assets);
	UComboGraphEdNode* GraphNode = Cast<UComboGraphEdNode>(Node);
	if (Asset && GraphNode && GraphNode->RuntimeNode)
	{
		const FScopedTransaction Transaction(LOCTEXT("UpdateNodeWithAsset", "Updating Node with Asset"));
		GraphNode->Modify();
		GraphNode->RuntimeNode->SetAnimationAsset(Asset);
		GraphNode->GetSchema()->ForceVisualizationCacheClear();
		GraphNode->ReconstructNode();
	}
}

void UComboGraphSchema::DroppedAssetsOnPin(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphPin* Pin) const
{
	UAnimationAsset* Asset = FAssetData::GetFirstAsset<UAnimationAsset>(Assets);

	if (Asset && Pin)
	{
		FVector2D FixedOffset = GetFixedOffsetFromPin(Pin);

		// if (const UComboGraphEdNode* EdGraphNode = Cast<UComboGraphEdNode>(Pin->GetOwningNode()))
		// {
		// 	if (EdGraphNode->SlateNode)
		// 	{
		// 		const FVector2D Size = EdGraphNode->SlateNode->ComputeDesiredSize(FSlateApplication::Get().GetApplicationScale());
		// 		FixedOffset.X = Size.X + 100.f;
		// 	}
		// }

		SpawnNodeFromAsset(Asset, GraphPosition + FixedOffset, Pin->GetOwningNode()->GetGraph(), Pin);
	}
}

void UComboGraphSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& bOutOkIcon) const
{
	Super::GetAssetsGraphHoverMessage(Assets, HoverGraph, OutTooltipText, bOutOkIcon);

	UAnimationAsset* AnimationAsset = FAssetData::GetFirstAsset<UAnimationAsset>(Assets);
	if (AnimationAsset)
	{
		if (UAnimMontage* Montage = FAssetData::GetFirstAsset<UAnimMontage>(Assets))
		{
			bOutOkIcon = true;
			OutTooltipText = LOCTEXT("MontagesInGraphs", "Drop Montage here to create a new node in the combo graph").ToString();
		}
		else if (UAnimSequence* Sequence = FAssetData::GetFirstAsset<UAnimSequence>(Assets))
		{
			bOutOkIcon = true;
			OutTooltipText = LOCTEXT("SequencesInGraphs", "Drop Sequence here to create a new node in the combo graph. Montage will be created dynamically from the Anim Sequence.").ToString();
		}
	}
}

void UComboGraphSchema::GetAssetsPinHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphPin* HoverPin, FString& OutTooltipText, bool& bOutOkIcon) const
{
	Super::GetAssetsPinHoverMessage(Assets, HoverPin, OutTooltipText, bOutOkIcon);

	UAnimationAsset* Asset = FAssetData::GetFirstAsset<UAnimationAsset>(Assets);
	if (!Asset || !HoverPin)
	{
		OutTooltipText = TEXT("");
		bOutOkIcon = false;
		return;
	}

	// const bool bDirectionMatch = HoverPin->Direction == EGPD_Input;
	const bool bDirectionMatch = IsHoverPinMatching(HoverPin);

	if (bDirectionMatch)
	{
		UEdGraphNode* OwningNode = HoverPin->GetOwningNode();
		if (UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
		{
			bOutOkIcon = true;
			OutTooltipText = FText::Format(LOCTEXT("MontagesOnPin", "Connect Montage {0} in the combo graph to {1}"),
				FText::FromString(Asset->GetName()),
				OwningNode ? OwningNode->GetNodeTitle(ENodeTitleType::ListView) : FText::FromName(HoverPin->PinName)
			).ToString();
		}
		else if (UAnimSequence* Sequence = Cast<UAnimSequence>(Asset))
		{
			bOutOkIcon = true;
			OutTooltipText = FText::Format(LOCTEXT("SequencesOnPin", "Connect Sequence {0} in the combo graph to {1}"),
				FText::FromString(Asset->GetName()),
				OwningNode ? OwningNode->GetNodeTitle(ENodeTitleType::ListView) : FText::FromName(HoverPin->PinName)
			).ToString();
		}
	}
	else
	{
		bOutOkIcon = false;
		OutTooltipText = LOCTEXT("AssetPinHoverMessage_Fail", "Type or direction mismatch; must be wired to a combo input").ToString();
	}
}

void UComboGraphSchema::GetAssetsNodeHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphNode* HoverNode, FString& OutTooltipText, bool& bOutOkIcon) const
{
	Super::GetAssetsNodeHoverMessage(Assets, HoverNode, OutTooltipText, bOutOkIcon);

	UAnimationAsset* Asset = FAssetData::GetFirstAsset<UAnimationAsset>(Assets);
	if ((Asset == nullptr) || (HoverNode == nullptr) || !HoverNode->IsA(UEdGraphNode::StaticClass()))
	{
		OutTooltipText = TEXT("");
		bOutOkIcon = false;
		return;
	}

	const bool bCanPlayAsset = SupportNodeClassForAsset(Asset->GetClass(), HoverNode);
	if (bCanPlayAsset)
	{
		bOutOkIcon = true;
		OutTooltipText = FText::Format(LOCTEXT("AssetNodeHoverMessage_Success", "Change node to play '{0}'"), FText::FromString(Asset->GetName())).ToString();
	}
	else
	{
		bOutOkIcon = false;
		OutTooltipText = FText::Format(LOCTEXT("AssetNodeHoverMessage_Fail", "Cannot play '{0}' on this node type"),  FText::FromString(Asset->GetName())).ToString();
	}
}

bool UComboGraphSchema::CanDuplicateGraph(UEdGraph* InSourceGraph) const
{
	return false;
}

int32 UComboGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	if (Graph)
	{
		const TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(Graph);
		if (GraphEditorPtr.IsValid())
		{
			return GraphEditorPtr->GetNumberOfSelectedNodes();
		}
	}

	return 0;
}

TSharedPtr<FEdGraphSchemaAction> UComboGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FComboGraphSchemaAction_AddComment));
}

void UComboGraphSchema::SpawnNodeFromAsset(UAnimationAsset* Asset, const FVector2D& GraphPosition, UEdGraph* Graph, UEdGraphPin* PinIfAvailable)
{
	check(Graph);
	check(Graph->GetSchema()->IsA(UComboGraphSchema::StaticClass()));
	check(Asset);

	const bool bDirectionMatch = (PinIfAvailable == nullptr) || IsHoverPinMatching(PinIfAvailable);
	if (bDirectionMatch)
	{
		UClass* NewNodeClass = GetNodeClassForAnimAsset(Asset, Graph);
		UClass* NewRuntimeClass = GetRuntimeClassForAnimAsset(Asset, Graph);
		if (NewNodeClass && NewRuntimeClass)
		{
			CG_EDITOR_LOG(Verbose, TEXT("SpawnNodeFromAsset - NewNodeClass: %s, NewRuntimeClass: %s"), *NewNodeClass->GetName(), *NewRuntimeClass->GetName());
			UComboGraphEdNode* NewNode = NewObject<UComboGraphEdNode>(Graph, NewNodeClass);
			NewNode->RuntimeNode = NewObject<UComboGraphNodeAnimBase>(NewNode, NewRuntimeClass);
			NewNode->RuntimeNode->SetAnimationAsset(Asset);

			FComboGraphSchemaAction_NewNode Action;
			Action.NodeTemplate = NewNode;
			Action.PerformAction(Graph, PinIfAvailable, GraphPosition);
		}
	}
}

UClass* UComboGraphSchema::GetNodeClassForAnimAsset(const UAnimationAsset* Asset, const UEdGraph* Graph)
{
	if (Asset->GetClass()->IsChildOf(UAnimMontage::StaticClass()))
	{
		return UComboGraphEdNode::StaticClass();
	}

	if (Asset->GetClass()->IsChildOf(UAnimSequence::StaticClass()))
	{
		return UComboGraphEdNode::StaticClass();
	}

	return nullptr;
}

TSubclassOf<UComboGraphNodeAnimBase> UComboGraphSchema::GetRuntimeClassForAnimAsset(const UAnimationAsset* Asset, const UEdGraph* Graph)
{
	const UComboGraph* ComboGraph = Cast<UComboGraph>(Graph->GetOuter());

	if (Asset->GetClass()->IsChildOf(UAnimMontage::StaticClass()))
	{
		if (!ComboGraph)
		{
			return UComboGraphNodeMontage::StaticClass();
		}

		return ComboGraph->DefaultNodeMontageType;
	}

	if (Asset->GetClass()->IsChildOf(UAnimSequence::StaticClass()))
	{
		if (!ComboGraph)
		{
			return UComboGraphNodeSequence::StaticClass();
		}

		return ComboGraph->DefaultNodeSequenceType;
	}

	return nullptr;
}

bool UComboGraphSchema::SupportNodeClassForAsset(UClass* AssetClass, const UEdGraphNode* InGraphNode)
{
	const UComboGraphEdNode* GraphNode = Cast<UComboGraphEdNode>(InGraphNode);
	if (GraphNode && GraphNode->RuntimeNode)
	{
		// See if this node supports this asset type (primary or not)
		return GraphNode->RuntimeNode->SupportsAssetClass(AssetClass);
	}

	return false;
}

void UComboGraphSchema::CreateEdgeConnection(UEdGraphPin* PinA, UEdGraphPin* PinB, const UComboGraphEdNodeBase* OwningNodeA, const UComboGraphEdNodeBase* OwningNodeB) const
{
	const FVector2D InitPos((OwningNodeA->NodePosX + OwningNodeB->NodePosX) / 2, (OwningNodeA->NodePosY + OwningNodeB->NodePosY) / 2);

	UInputAction* DefaultInput = nullptr;
	if (const UComboGraphEdGraph* ComboEdGraph = Cast<UComboGraphEdGraph>(OwningNodeA->GetGraph()))
	{
		if (const UComboGraph* ComboGraphModel = ComboEdGraph->GetComboGraphModel())
		{
			DefaultInput = ComboGraphModel->DefaultInputAction;
		}
	}

	FComboGraphSchemaAction_NewEdge Action;
	Action.NodeTemplate = NewObject<UComboGraphEdNodeEdge>(OwningNodeA->GetGraph());

	UComboGraphEdge* RuntimeEdge = NewObject<UComboGraphEdge>(Action.NodeTemplate, UComboGraphEdge::StaticClass());
	RuntimeEdge->TransitionInput = DefaultInput;
	Action.NodeTemplate->SetRuntimeEdge(RuntimeEdge);

	Action.PerformAction(OwningNodeA->GetGraph(), PinB, InitPos, true);
	// Always create connections from node A to B, don't allow adding in reverse
	Action.NodeTemplate->CreateConnections(OwningNodeA, OwningNodeB);
}

void UComboGraphSchema::CreateAndAddActionToContextMenu(FGraphContextMenuBuilder& ContextMenuBuilder, const TSubclassOf<UComboGraphNodeBase> NodeType) const
{
	if (!NodeType)
	{
		return;
	}


	UComboGraphNodeBase* NodeCDO = NodeType.GetDefaultObject();
	if (!NodeCDO)
	{
		return;
	}

	const FText AddToolTip = LOCTEXT("NewCombatGraphNodeTooltip", "Add node here");

	FText Description = NodeCDO->ContextMenuName;
	FString NodeName = NodeType->GetName();
	NodeName.RemoveFromEnd("_C");

	// Shouldn't happen. ContextMenuName if empty will prevent execution of CreateAndAddActionToContextMenu
	if (Description.IsEmpty())
	{
		Description = FText::FromString(NodeName);
	}

	// If BP node type, set category to something specific (can be further defined if Context Menu Name, with a "|" which will override the category)
	FText Category = NodeType->IsNative() ? LOCTEXT("CombatGraphNodeAction", "Combat Graph Node") : FText::FromString(TEXT("Custom"));

	// Split up ContextMenuName by "|" and create top category if there is more than one level
	const FString ContextMenu = NodeCDO->ContextMenuName.ToString();
	FString Left, Right;
	if (ContextMenu.Split(TEXT("|"), &Left, &Right))
	{
		Category = FText::FromString(Left.TrimStartAndEnd());
		Description = FText::FromString(Right.TrimStartAndEnd());
	}

	// If BP node, append class name
	if (!NodeType->IsNative() && NodeCDO->bIncludeClassNameInContextMenu)
	{
		Description = FText::FromString(FString::Printf(TEXT("%s (%s)"), *Description.ToString(), *NodeName));
	}

	const TSharedPtr<FComboGraphSchemaAction_NewNode> Action(new FComboGraphSchemaAction_NewNode(Category, Description, AddToolTip, 1));
	Action->NodeTemplate = NewObject<UComboGraphEdNode>(ContextMenuBuilder.OwnerOfTemporaries);
	Action->NodeTemplate->RuntimeNode = NewObject<UComboGraphNodeAnimBase>(Action->NodeTemplate, NodeType);
	ContextMenuBuilder.AddAction(Action);
}

bool UComboGraphSchema::IsHoverPinMatching(const UEdGraphPin* InHoverPin)
{
	check(InHoverPin);

	const bool bIsValidName = InHoverPin->PinName == UComboGraphPinNames::PinName_InLeft ||
		InHoverPin->PinName == UComboGraphPinNames::PinName_InTop ||
		InHoverPin->PinName == UComboGraphPinNames::PinName_OutRight ||
		InHoverPin->PinName == UComboGraphPinNames::PinName_OutBottom;

	return (InHoverPin->Direction == EGPD_Input || InHoverPin->Direction == EGPD_Output) && bIsValidName;
}

FVector2D UComboGraphSchema::GetFixedOffsetFromPin(const UEdGraphPin* InHoverPin)
{
	// If don't have access to bounding information for node, using fixed offset that should work for most cases.
	// Start at 450.f, and is further refined based on SlateNode bounding info is available
	FVector2D FixedOffset(450.0f, 0.f);
	FVector2D DesiredSize(0.f, 0.f);

	if (const UComboGraphEdNode* EdGraphNode = Cast<UComboGraphEdNode>(InHoverPin->GetOwningNode()))
	{
		if (EdGraphNode->SlateNode)
		{
			DesiredSize = EdGraphNode->SlateNode->ComputeDesiredSize(FSlateApplication::Get().GetApplicationScale());
		}
	}

	if (InHoverPin->PinName == UComboGraphPinNames::PinName_InLeft)
	{
		FixedOffset = FVector2D(-(DesiredSize.X * 2 + 100.f), -DesiredSize.Y);
	}
	else if (InHoverPin->PinName == UComboGraphPinNames::PinName_InTop)
	{
		FixedOffset = FVector2D(-DesiredSize.X, -(DesiredSize.Y * 2 + 100.f));
	}
	else if (InHoverPin->PinName == UComboGraphPinNames::PinName_OutRight)
	{
		FixedOffset = FVector2D(100.f, -DesiredSize.Y);
	}
	else if (InHoverPin->PinName == UComboGraphPinNames::PinName_OutBottom)
	{
		FixedOffset = FVector2D(-DesiredSize.X, 100.f);
	}


	return DesiredSize + FixedOffset;
}

#undef LOCTEXT_NAMESPACE
