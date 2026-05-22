#include "AssetGraphSchema_GameplayAbilityComboGraph.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include "AssetRegistry/AssetData.h"
#include "AssetToolsModule.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Factories/AnimMontageFactory.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphEdge.h"
#include "GenericGraphAssetEditor/ConnectionDrawingPolicy_GenericGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "GenericGraph.h"
#include "GenericGraphEdge.h"
#include "GenericGraphNode.h"
#include "GraphEditorActions.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "AssetGraphSchema_GameplayAbilityComboGraph"

int32 UAssetGraphSchema_GameplayAbilityComboGraph::CurrentCacheRefreshID = 0;

namespace
{
	class FYogComboGraphCycleChecker
	{
	public:
		bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
		{
			VisitedNodes.Add(StartNode);
			return TraverseNodes(EndNode);
		}

	private:
		bool TraverseNodes(UEdGraphNode* Node)
		{
			VisitedNodes.Add(Node);

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->Direction != EGPD_Output)
				{
					continue;
				}

				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					UEdGraphNode* OtherNode = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
					if (!OtherNode)
					{
						continue;
					}

					if (VisitedNodes.Contains(OtherNode))
					{
						return false;
					}

					if (!FinishedNodes.Contains(OtherNode) && !TraverseNodes(OtherNode))
					{
						return false;
					}
				}
			}

			VisitedNodes.Remove(Node);
			FinishedNodes.Add(Node);
			return true;
		}

		TSet<UEdGraphNode*> VisitedNodes;
		TSet<UEdGraphNode*> FinishedNodes;
	};

	FName MakeNodeIdFromAnimation(const UAnimationAsset* AnimationAsset)
	{
		if (!AnimationAsset)
		{
			return NAME_None;
		}

		FString NodeName = AnimationAsset->GetName();
		NodeName.RemoveFromEnd(TEXT("_Montage"));
		return FName(*NodeName);
	}

	UAnimMontage* CreateMontageFromSequence(UAnimSequence* Sequence)
	{
		if (!Sequence)
		{
			return nullptr;
		}

		const FString SourcePackageName = Sequence->GetOutermost()->GetName();
		FString PackagePath;
		FString SourceAssetName;
		SourcePackageName.Split(TEXT("/"), &PackagePath, &SourceAssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);

		if (PackagePath.IsEmpty())
		{
			PackagePath = TEXT("/Game");
		}

		FString AssetName = Sequence->GetName();
		AssetName.RemoveFromEnd(TEXT("_Seq"));
		AssetName += TEXT("_Montage");

		const FString BasePackageName = PackagePath / AssetName;
		if (UAnimMontage* ExistingMontage = LoadObject<UAnimMontage>(nullptr, *BasePackageName))
		{
			return ExistingMontage;
		}

		FString UniquePackageName;
		FString UniqueAssetName;
		IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
		AssetTools.CreateUniqueAssetName(BasePackageName, FString(), UniquePackageName, UniqueAssetName);

		UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
		Factory->SourceAnimation = Sequence;

		UAnimMontage* Montage = Cast<UAnimMontage>(AssetTools.CreateAsset(
			UniqueAssetName,
			FPackageName::GetLongPackagePath(UniquePackageName),
			UAnimMontage::StaticClass(),
			Factory,
			TEXT("YogComboGraphSequenceDrop")));

		if (!Montage)
		{
			return nullptr;
		}

		Montage->MarkPackageDirty();

		return Montage;
	}

	UAnimMontage* ResolveMontageForAsset(UAnimationAsset* AnimationAsset)
	{
		if (UAnimMontage* Montage = Cast<UAnimMontage>(AnimationAsset))
		{
			return Montage;
		}

		return CreateMontageFromSequence(Cast<UAnimSequence>(AnimationAsset));
	}

	UGameplayAbilityComboGraph* GetGameplayAbilityComboGraph(const UEdGraph* Graph)
	{
		return Graph ? Cast<UGameplayAbilityComboGraph>(Graph->GetOuter()) : nullptr;
	}

	UEdGraphPin* GetInputPin(const UEdNode_GenericGraphNode* Node)
	{
		return Node && Node->Pins.Num() > 0 ? Node->Pins[0] : nullptr;
	}

	UEdGraphPin* GetOutputPin(const UEdNode_GenericGraphNode* Node)
	{
		return Node && Node->Pins.Num() > 1 ? Node->Pins[1] : nullptr;
	}

	UEdGraphPin* GetEdgeInputPin(const UEdNode_GenericGraphEdge* EdgeNode)
	{
		return EdgeNode && EdgeNode->Pins.Num() > 0 ? EdgeNode->Pins[0] : nullptr;
	}

	UEdGraphPin* GetEdgeOutputPin(const UEdNode_GenericGraphEdge* EdgeNode)
	{
		return EdgeNode && EdgeNode->Pins.Num() > 1 ? EdgeNode->Pins[1] : nullptr;
	}

	void AllocateGenericNodePins(UEdNode_GenericGraphNode* EdNode)
	{
		if (!EdNode || EdNode->Pins.Num() >= 2)
		{
			return;
		}

		EdNode->Pins.Reset();
		EdNode->CreatePin(EGPD_Input, TEXT("MultipleNodes"), FName(), TEXT("In"));
		EdNode->CreatePin(EGPD_Output, TEXT("MultipleNodes"), FName(), TEXT("Out"));
	}

	void AllocateGenericEdgePins(UEdNode_GenericGraphEdge* EdgeNode)
	{
		if (!EdgeNode || EdgeNode->Pins.Num() >= 2)
		{
			return;
		}

		EdgeNode->Pins.Reset();
		UEdGraphPin* InputPin = EdgeNode->CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
		InputPin->bHidden = true;

		UEdGraphPin* OutputPin = EdgeNode->CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
		OutputPin->bHidden = true;
	}

	UGameplayAbilityComboGraphNode* CreateRuntimeNodeForMontage(UEdNode_GenericGraphNode* EdNode, UGameplayAbilityComboGraph* ComboGraph, UAnimMontage* Montage)
	{
		if (!EdNode || !ComboGraph || !Montage || !ComboGraph->NodeType || !ComboGraph->NodeType->IsChildOf(UGameplayAbilityComboGraphNode::StaticClass()))
		{
			return nullptr;
		}

		UGameplayAbilityComboGraphNode* ComboNode = NewObject<UGameplayAbilityComboGraphNode>(EdNode, ComboGraph->NodeType);
		if (!ComboNode)
		{
			return nullptr;
		}

		ComboNode->Graph = ComboGraph;
		ComboNode->Montage = Montage;
		ComboNode->NodeId = MakeNodeIdFromAnimation(Montage);
		ComboNode->NodeTitle = FText::FromName(ComboNode->NodeId);
		EdNode->GenericGraphNode = ComboNode;

		return ComboNode;
	}

	UEdGraphNode* PerformNewNodeAction(UEdNode_GenericGraphNode* NodeTemplate, UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D& Location, bool bSelectNewNode)
	{
		if (!NodeTemplate || !ParentGraph)
		{
			return nullptr;
		}

		const FScopedTransaction Transaction(LOCTEXT("YogComboGraphEditorNewNode", "Yog Combo Graph Editor: New Node"));
		ParentGraph->Modify();
		if (FromPin)
		{
			FromPin->Modify();
		}

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		AllocateGenericNodePins(NodeTemplate);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		if (NodeTemplate->GenericGraphNode)
		{
			NodeTemplate->GenericGraphNode->SetFlags(RF_Transactional);
		}
		NodeTemplate->SetFlags(RF_Transactional);

		if (FromPin)
		{
			UEdGraphPin* TargetPin = FromPin->Direction == EGPD_Input ? GetOutputPin(NodeTemplate) : GetInputPin(NodeTemplate);
			if (TargetPin && ParentGraph->GetSchema()->TryCreateConnection(FromPin, TargetPin))
			{
				FromPin->GetOwningNode()->NodeConnectionListChanged();
			}
		}

		return NodeTemplate;
	}

	struct FYogComboGraphSchemaAction_NewNode : public FEdGraphSchemaAction
	{
		FYogComboGraphSchemaAction_NewNode()
			: FEdGraphSchemaAction()
			, NodeTemplate(nullptr)
		{
		}

		FYogComboGraphSchemaAction_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
			: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
			, NodeTemplate(nullptr)
		{
		}

		virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override
		{
			return PerformNewNodeAction(NodeTemplate, ParentGraph, FromPin, Location, bSelectNewNode);
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			FEdGraphSchemaAction::AddReferencedObjects(Collector);
			Collector.AddReferencedObject(NodeTemplate);
		}

		UEdNode_GenericGraphNode* NodeTemplate;
	};

	UEdNode_GenericGraphNode* GetEndNodeFromEdge(const UEdNode_GenericGraphEdge* EdgeNode)
	{
		const UEdGraphPin* OutputPin = GetEdgeOutputPin(EdgeNode);
		if (!OutputPin || OutputPin->LinkedTo.IsEmpty())
		{
			return nullptr;
		}

		return Cast<UEdNode_GenericGraphNode>(OutputPin->LinkedTo[0]->GetOwningNode());
	}

	void CreateEdgeConnections(UEdNode_GenericGraphEdge* EdgeNode, UEdNode_GenericGraphNode* Start, UEdNode_GenericGraphNode* End)
	{
		UEdGraphPin* EdgeInputPin = GetEdgeInputPin(EdgeNode);
		UEdGraphPin* EdgeOutputPin = GetEdgeOutputPin(EdgeNode);
		UEdGraphPin* StartOutputPin = GetOutputPin(Start);
		UEdGraphPin* EndInputPin = GetInputPin(End);
		if (!EdgeInputPin || !EdgeOutputPin || !StartOutputPin || !EndInputPin)
		{
			return;
		}

		EdgeInputPin->Modify();
		EdgeInputPin->LinkedTo.Empty();

		StartOutputPin->Modify();
		EdgeInputPin->MakeLinkTo(StartOutputPin);

		EdgeOutputPin->Modify();
		EdgeOutputPin->LinkedTo.Empty();

		EndInputPin->Modify();
		EdgeOutputPin->MakeLinkTo(EndInputPin);
	}

	UEdNode_GenericGraphEdge* CreateEdgeNode(UEdGraph* ParentGraph, UGenericGraph* GenericGraph, UEdNode_GenericGraphNode* Start, UEdNode_GenericGraphNode* End, const FVector2D& Location)
	{
		if (!ParentGraph || !GenericGraph || !GenericGraph->EdgeType || !Start || !End)
		{
			return nullptr;
		}

		const FScopedTransaction Transaction(LOCTEXT("YogComboGraphEditorNewEdge", "Yog Combo Graph Editor: New Edge"));
		ParentGraph->Modify();

		UEdNode_GenericGraphEdge* EdgeNode = NewObject<UEdNode_GenericGraphEdge>(ParentGraph);
		EdgeNode->GenericGraphEdge = NewObject<UGenericGraphEdge>(EdgeNode, GenericGraph->EdgeType);
		EdgeNode->Graph = ParentGraph;

		if (EdgeNode->GenericGraphEdge)
		{
			EdgeNode->GenericGraphEdge->Graph = GenericGraph;
			EdgeNode->GenericGraphEdge->StartNode = Start->GenericGraphNode;
			EdgeNode->GenericGraphEdge->EndNode = End->GenericGraphNode;
		}

		ParentGraph->AddNode(EdgeNode, true, false);
		EdgeNode->CreateNewGuid();
		EdgeNode->PostPlacedNewNode();
		AllocateGenericEdgePins(EdgeNode);

		EdgeNode->NodePosX = Location.X;
		EdgeNode->NodePosY = Location.Y;

		if (EdgeNode->GenericGraphEdge)
		{
			EdgeNode->GenericGraphEdge->SetFlags(RF_Transactional);
		}
		EdgeNode->SetFlags(RF_Transactional);

		CreateEdgeConnections(EdgeNode, Start, End);
		return EdgeNode;
	}

	void SpawnNodeFromAnimationAsset(UAnimationAsset* AnimationAsset, const FVector2D& GraphPosition, UEdGraph* Graph, UEdGraphPin* FromPin)
	{
		if (!Graph)
		{
			return;
		}

		UGameplayAbilityComboGraph* ComboGraph = GetGameplayAbilityComboGraph(Graph);
		UAnimMontage* Montage = ResolveMontageForAsset(AnimationAsset);
		if (!ComboGraph || !Montage)
		{
			return;
		}

		UEdNode_GenericGraphNode* NodeTemplate = NewObject<UEdNode_GenericGraphNode>(Graph);
		CreateRuntimeNodeForMontage(NodeTemplate, ComboGraph, Montage);
		if (NodeTemplate->GenericGraphNode)
		{
			UEdGraphPin* AutowirePin = FromPin && FromPin->Direction == EGPD_Output ? FromPin : nullptr;
			UEdNode_GenericGraphNode* NewNode = Cast<UEdNode_GenericGraphNode>(PerformNewNodeAction(NodeTemplate, Graph, AutowirePin, GraphPosition, true));
			if (NewNode && FromPin && FromPin->Direction == EGPD_Input)
			{
				Graph->GetSchema()->TryCreateConnection(GetOutputPin(NewNode), FromPin);
			}
		}
	}

	void UpdateNodeFromAnimationAsset(UAnimationAsset* AnimationAsset, UEdGraphNode* Node)
	{
		UEdNode_GenericGraphNode* EdNode = Cast<UEdNode_GenericGraphNode>(Node);
		UGameplayAbilityComboGraphNode* ComboNode = EdNode ? Cast<UGameplayAbilityComboGraphNode>(EdNode->GenericGraphNode) : nullptr;
		UAnimMontage* Montage = ResolveMontageForAsset(AnimationAsset);
		if (!EdNode || !ComboNode || !Montage)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("UpdateNodeWithAnimationAsset", "Update Combo Node Animation"));
		EdNode->Modify();
		ComboNode->Modify();

		ComboNode->Montage = Montage;
		ComboNode->NodeId = MakeNodeIdFromAnimation(Montage);
		ComboNode->NodeTitle = FText::FromName(ComboNode->NodeId);
		EdNode->ReconstructNode();
		EdNode->GetGraph()->GetSchema()->ForceVisualizationCacheClear();
		EdNode->GetGraph()->MarkPackageDirty();
	}
}

UAnimationAsset* UAssetGraphSchema_GameplayAbilityComboGraph::GetFirstSupportedAnimationAsset(const TArray<FAssetData>& Assets)
{
	if (UAnimMontage* Montage = FAssetData::GetFirstAsset<UAnimMontage>(Assets))
	{
		return Montage;
	}

	if (UAnimSequence* Sequence = FAssetData::GetFirstAsset<UAnimSequence>(Assets))
	{
		return Sequence;
	}

	return nullptr;
}

EGraphType UAssetGraphSchema_GameplayAbilityComboGraph::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_StateMachine;
}

void UAssetGraphSchema_GameplayAbilityComboGraph::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UGameplayAbilityComboGraph* Graph = Cast<UGameplayAbilityComboGraph>(ContextMenuBuilder.CurrentGraph ? ContextMenuBuilder.CurrentGraph->GetOuter() : nullptr);
	if (!Graph || !Graph->NodeType)
	{
		return;
	}

	FText Description = Graph->NodeType.GetDefaultObject()->ContextMenuName;
	if (Description.IsEmpty())
	{
		FString Title = Graph->NodeType->GetName();
		Title.RemoveFromEnd(TEXT("_C"));
		Description = FText::FromString(Title);
	}

	TSharedPtr<FYogComboGraphSchemaAction_NewNode> Action(new FYogComboGraphSchemaAction_NewNode(
		LOCTEXT("YogComboGraphNodeAction", "Combo Graph Node"),
		Description,
		LOCTEXT("YogComboGraphNewNodeTooltip", "Add combo node here"),
		0));
	Action->NodeTemplate = NewObject<UEdNode_GenericGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
	Action->NodeTemplate->GenericGraphNode = NewObject<UGenericGraphNode>(Action->NodeTemplate, Graph->NodeType);
	Action->NodeTemplate->GenericGraphNode->Graph = Graph;
	ContextMenuBuilder.AddAction(Action);
}

void UAssetGraphSchema_GameplayAbilityComboGraph::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Context && Context->Pin)
	{
		FToolMenuSection& Section = Menu->AddSection("YogComboGraphPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
		if (Context->Pin->LinkedTo.Num() > 0)
		{
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);
		}
	}
	else if (Context && Context->Node)
	{
		FToolMenuSection& Section = Menu->AddSection("YogComboGraphNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
		Section.AddMenuEntry(FGenericCommands::Get().Delete);
		Section.AddMenuEntry(FGenericCommands::Get().Cut);
		Section.AddMenuEntry(FGenericCommands::Get().Copy);
		Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
		Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
	}

	UEdGraphSchema::GetContextMenuActions(Menu, Context);
}

const FPinConnectionResponse UAssetGraphSchema_GameplayAbilityComboGraph::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorMissingPin", "Missing pin"));
	}

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Can't connect node to itself"));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameDirection", "Can't connect pins with the same direction"));
	}

	UEdNode_GenericGraphNode* NodeA = Cast<UEdNode_GenericGraphNode>(A->GetOwningNode());
	UEdNode_GenericGraphNode* NodeB = Cast<UEdNode_GenericGraphNode>(B->GetOwningNode());
	if (!NodeA || !NodeB || !NodeA->GenericGraphNode || !NodeB->GenericGraphNode)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorInvalidNode", "Not a valid combo graph node"));
	}

	UEdNode_GenericGraphNode* OutNode = A->Direction == EGPD_Output ? NodeA : NodeB;
	UEdNode_GenericGraphNode* InNode = A->Direction == EGPD_Output ? NodeB : NodeA;

	bool bAllowCycles = false;
	if (const UGenericGraph* GenericGraph = OutNode->GetGraph() ? Cast<UGenericGraph>(OutNode->GetGraph()->GetOuter()) : nullptr)
	{
		bAllowCycles = GenericGraph->bCanBeCyclical;
	}

	FYogComboGraphCycleChecker CycleChecker;
	if (!bAllowCycles && !CycleChecker.CheckForLoop(OutNode, InNode))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorCycle", "Can't create a graph cycle"));
	}

	FText ErrorMessage;
	UEdGraphPin* OutPin = GetOutputPin(OutNode);
	UEdGraphPin* InPin = GetInputPin(InNode);
	if (!OutPin || !InPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorMissingComboPin", "Missing combo graph pin"));
	}

	if (!OutNode->GenericGraphNode->CanCreateConnectionTo(InNode->GenericGraphNode, OutPin->LinkedTo.Num(), ErrorMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
	}

	if (!InNode->GenericGraphNode->CanCreateConnectionFrom(OutNode->GenericGraphNode, InPin->LinkedTo.Num(), ErrorMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
	}

	return OutNode->GenericGraphNode->GetGraph() && OutNode->GenericGraphNode->GetGraph()->bEdgeEnabled
		? FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnectWithEdge", "Connect nodes with edge"))
		: FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
}

bool UAssetGraphSchema_GameplayAbilityComboGraph::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	if (!A || !B || A->Direction == B->Direction)
	{
		return false;
	}

	UEdNode_GenericGraphNode* NodeA = Cast<UEdNode_GenericGraphNode>(A->GetOwningNode());
	UEdNode_GenericGraphNode* NodeB = Cast<UEdNode_GenericGraphNode>(B->GetOwningNode());
	UEdNode_GenericGraphNode* OutNode = A->Direction == EGPD_Output ? NodeA : NodeB;
	UEdNode_GenericGraphNode* InNode = A->Direction == EGPD_Output ? NodeB : NodeA;
	UEdGraphPin* OutPin = GetOutputPin(OutNode);
	UEdGraphPin* InPin = GetInputPin(InNode);
	if (!OutNode || !InNode || !OutPin || !InPin)
	{
		return false;
	}

	for (UEdGraphPin* TestPin : OutPin->LinkedTo)
	{
		UEdGraphNode* ChildNode = TestPin ? TestPin->GetOwningNode() : nullptr;
		if (const UEdNode_GenericGraphEdge* EdgeNode = Cast<UEdNode_GenericGraphEdge>(ChildNode))
		{
			ChildNode = GetEndNodeFromEdge(EdgeNode);
		}

		if (ChildNode == InNode)
		{
			return false;
		}
	}

	UEdGraphSchema::TryCreateConnection(OutPin, InPin);
	return true;
}

bool UAssetGraphSchema_GameplayAbilityComboGraph::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const
{
	if (!A || !B || A->Direction == B->Direction)
	{
		return false;
	}

	UEdNode_GenericGraphNode* NodeA = Cast<UEdNode_GenericGraphNode>(A->GetOwningNode());
	UEdNode_GenericGraphNode* NodeB = Cast<UEdNode_GenericGraphNode>(B->GetOwningNode());
	UEdNode_GenericGraphNode* OutNode = A->Direction == EGPD_Output ? NodeA : NodeB;
	UEdNode_GenericGraphNode* InNode = A->Direction == EGPD_Output ? NodeB : NodeA;
	if (!OutNode || !InNode || !OutNode->GenericGraphNode || !OutNode->GenericGraphNode->GetGraph() || !OutNode->GenericGraphNode->GetGraph()->EdgeType)
	{
		return false;
	}

	const FVector2D InitPos((OutNode->NodePosX + InNode->NodePosX) / 2, (OutNode->NodePosY + InNode->NodePosY) / 2);
	UEdNode_GenericGraphEdge* EdgeNode = CreateEdgeNode(OutNode->GetGraph(), OutNode->GenericGraphNode->GetGraph(), OutNode, InNode, InitPos);
	return EdgeNode != nullptr;
}

FConnectionDrawingPolicy* UAssetGraphSchema_GameplayAbilityComboGraph::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FConnectionDrawingPolicy_GenericGraph(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UAssetGraphSchema_GameplayAbilityComboGraph::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

void UAssetGraphSchema_GameplayAbilityComboGraph::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));
	UEdGraphSchema::BreakNodeLinks(TargetNode);
}

void UAssetGraphSchema_GameplayAbilityComboGraph::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));
	UEdGraphSchema::BreakPinLinks(TargetPin, bSendsNodeNotification);
}

void UAssetGraphSchema_GameplayAbilityComboGraph::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));
	UEdGraphSchema::BreakSinglePinLink(SourcePin, TargetPin);
}

UEdGraphPin* UAssetGraphSchema_GameplayAbilityComboGraph::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	UEdNode_GenericGraphNode* EdNode = Cast<UEdNode_GenericGraphNode>(InTargetNode);
	if (!EdNode)
	{
		return nullptr;
	}

	return InSourcePinDirection == EGPD_Input ? GetOutputPin(EdNode) : GetInputPin(EdNode);
}

bool UAssetGraphSchema_GameplayAbilityComboGraph::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	return Cast<UEdNode_GenericGraphNode>(InTargetNode) != nullptr;
}

bool UAssetGraphSchema_GameplayAbilityComboGraph::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UAssetGraphSchema_GameplayAbilityComboGraph::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UAssetGraphSchema_GameplayAbilityComboGraph::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
}

void UAssetGraphSchema_GameplayAbilityComboGraph::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	if (UAnimationAsset* AnimationAsset = GetFirstSupportedAnimationAsset(Assets))
	{
		SpawnNodeFromAnimationAsset(AnimationAsset, GraphPosition, Graph, nullptr);
	}
}

void UAssetGraphSchema_GameplayAbilityComboGraph::DroppedAssetsOnNode(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphNode* Node) const
{
	if (UAnimationAsset* AnimationAsset = GetFirstSupportedAnimationAsset(Assets))
	{
		UpdateNodeFromAnimationAsset(AnimationAsset, Node);
	}
}

void UAssetGraphSchema_GameplayAbilityComboGraph::DroppedAssetsOnPin(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphPin* Pin) const
{
	if (UAnimationAsset* AnimationAsset = GetFirstSupportedAnimationAsset(Assets))
	{
		const FVector2D Offset = Pin && Pin->Direction == EGPD_Input ? FVector2D(-220.f, 0.f) : FVector2D(220.f, 0.f);
		SpawnNodeFromAnimationAsset(AnimationAsset, GraphPosition + Offset, Pin ? Pin->GetOwningNode()->GetGraph() : nullptr, Pin);
	}
}

void UAssetGraphSchema_GameplayAbilityComboGraph::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& bOutOkIcon) const
{
	OutTooltipText = FString();
	bOutOkIcon = false;

	if (UAnimationAsset* AnimationAsset = GetFirstSupportedAnimationAsset(Assets))
	{
		bOutOkIcon = true;
		OutTooltipText = AnimationAsset->IsA<UAnimMontage>()
			? LOCTEXT("DropMontageOnGraph", "Drop Montage here to create a combo node.").ToString()
			: LOCTEXT("DropSequenceOnGraph", "Drop Sequence here to create a combo node and generated montage.").ToString();
	}
}

void UAssetGraphSchema_GameplayAbilityComboGraph::GetAssetsNodeHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphNode* HoverNode, FString& OutTooltipText, bool& bOutOkIcon) const
{
	OutTooltipText = FString();
	bOutOkIcon = false;

	if (UAnimationAsset* AnimationAsset = GetFirstSupportedAnimationAsset(Assets))
	{
		bOutOkIcon = Cast<UEdNode_GenericGraphNode>(HoverNode) != nullptr;
		OutTooltipText = bOutOkIcon
			? FText::Format(LOCTEXT("DropAnimationOnNode", "Set combo node animation to {0}."), FText::FromString(AnimationAsset->GetName())).ToString()
			: LOCTEXT("DropAnimationOnInvalidNode", "Drop on a combo node to assign this animation.").ToString();
	}
}

void UAssetGraphSchema_GameplayAbilityComboGraph::GetAssetsPinHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphPin* HoverPin, FString& OutTooltipText, bool& bOutOkIcon) const
{
	OutTooltipText = FString();
	bOutOkIcon = false;

	if (UAnimationAsset* AnimationAsset = GetFirstSupportedAnimationAsset(Assets))
	{
		bOutOkIcon = HoverPin != nullptr;
		OutTooltipText = bOutOkIcon
			? FText::Format(LOCTEXT("DropAnimationOnPin", "Create combo node for {0} and connect it."), FText::FromString(AnimationAsset->GetName())).ToString()
			: FString();
	}
}

#undef LOCTEXT_NAMESPACE
