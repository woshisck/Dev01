#include "Data/GameplayAbilityComboGraph.h"

#include "Animation/AnimMontage.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraph"

namespace
{
	EYogComboGraphInputAction NormalizeInputAction(EYogComboGraphInputAction InputAction)
	{
		if (InputAction == EYogComboGraphInputAction::LegacyWeaponSkill)
		{
			return EYogComboGraphInputAction::WeaponSkill;
		}
		return InputAction;
	}

	bool IsKnownInputAction(EYogComboGraphInputAction InputAction)
	{
		switch (NormalizeInputAction(InputAction))
		{
		case EYogComboGraphInputAction::Attack:
		case EYogComboGraphInputAction::WeaponSkill:
		case EYogComboGraphInputAction::Dash:
		case EYogComboGraphInputAction::Special:
		case EYogComboGraphInputAction::Any:
			return true;
		default:
			return false;
		}
	}

	bool DoesInputMatch(EYogComboGraphInputAction Required, EYogComboGraphInputAction Input)
	{
		Required = NormalizeInputAction(Required);
		Input = NormalizeInputAction(Input);
		if (!IsKnownInputAction(Required) || !IsKnownInputAction(Input) ||
			Required == EYogComboGraphInputAction::Any || Input == EYogComboGraphInputAction::Any)
		{
			return false;
		}
		return Required == Input;
	}

	bool DoesEdgeStateMatch(const FGameplayTagQuery& StateRequirement, const FGameplayTagContainer* OwnedTags)
	{
		if (StateRequirement.IsEmpty())
		{
			return true;
		}
		return OwnedTags != nullptr && StateRequirement.Matches(*OwnedTags);
	}

	FName MakeStableNodeId(const UObject* Object)
	{
		return Object ? FName(*Object->GetName()) : NAME_None;
	}

	FName GetRuntimeNodeId(const UGameplayAbilityComboGraphNode* Node)
	{
		return Node && !Node->NodeId.IsNone()
			? Node->NodeId
			: MakeStableNodeId(Node);
	}

	FString InputActionToString(EYogComboGraphInputAction InputAction)
	{
		InputAction = NormalizeInputAction(InputAction);
		if (!IsKnownInputAction(InputAction))
		{
			return FString(TEXT("Invalid"));
		}

		const UEnum* Enum = StaticEnum<EYogComboGraphInputAction>();
		return Enum
			? Enum->GetDisplayNameTextByValue(static_cast<int64>(InputAction)).ToString()
			: FString(TEXT("Any"));
	}
}

UGameplayAbilityComboGraphNode::UGameplayAbilityComboGraphNode()
{
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UGameplayAbilityComboGraph::StaticClass();
	ContextMenuName = LOCTEXT("ComboNodeContextName", "Combo Ability Node");
	BackgroundColor = FLinearColor(0.13f, 0.18f, 0.22f, 1.f);
#endif
}

FText UGameplayAbilityComboGraphNode::GetDescription_Implementation() const
{
	const FName RuntimeNodeId = GetRuntimeNodeId(this);
	const FString MontageName = GetNameSafe(Montage);
	return FText::FromString(FString::Printf(
		TEXT("Node=%s\nRootInput=%s\nMontage=%s\nComboWindow=%s [%d-%d / %d]\nCardSlot=%s\nCardRole=%s"),
		*RuntimeNodeId.ToString(),
		*InputActionToString(RootInputAction),
		Montage ? *MontageName : TEXT("None"),
		bUseNodeComboWindow ? TEXT("Node") : TEXT("Montage Notify"),
		ComboWindowStartFrame,
		ComboWindowEndFrame,
		TotalFrames,
		CombatDeckActionSlotTag.IsValid() ? *CombatDeckActionSlotTag.ToString() : TEXT("RuntimeDefault"),
		CombatDeckFlowRoleTag.IsValid() ? *CombatDeckFlowRoleTag.ToString() : TEXT("RuntimeDefault")));
}

#if WITH_EDITOR
FLinearColor UGameplayAbilityComboGraphNode::GetBackgroundColor() const
{
	if (bDebugActive)
	{
		return FLinearColor(0.05f, 0.85f, 0.2f, 1.f);
	}
	return BackgroundColor;
}

FText UGameplayAbilityComboGraphNode::GetNodeTitle() const
{
	if (!NodeId.IsNone())
	{
		return FText::FromName(NodeId);
	}

	if (!NodeTitle.IsEmpty())
	{
		return NodeTitle;
	}

	return LOCTEXT("UntitledComboNode", "Combo Montage");
}

bool UGameplayAbilityComboGraphNode::CanCreateConnectionTo(UGenericGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage)
{
	if (Other == this)
	{
		ErrorMessage = LOCTEXT("CannotConnectSelf", "A combo node cannot connect to itself.");
		return false;
	}

	return Super::CanCreateConnectionTo(Other, NumberOfChildrenNodes, ErrorMessage);
}
#endif

UGameplayAbilityComboGraphEdge::UGameplayAbilityComboGraphEdge()
{
#if WITH_EDITORONLY_DATA
	bShouldDrawTitle = true;
	EdgeColour = FLinearColor(0.75f, 0.92f, 1.f, 1.f);
#endif
}

#if WITH_EDITOR
FText UGameplayAbilityComboGraphEdge::GetNodeTitle() const
{
	return FText::FromString(InputActionToString(InputAction));
}
#endif

UGameplayAbilityComboGraph::UGameplayAbilityComboGraph()
{
	NodeType = UGameplayAbilityComboGraphNode::StaticClass();
	EdgeType = UGameplayAbilityComboGraphEdge::StaticClass();
	bEdgeEnabled = true;

#if WITH_EDITORONLY_DATA
	bCanRenameNode = true;
	bCanBeCyclical = false;
#endif
}

const UGameplayAbilityComboGraphNode* UGameplayAbilityComboGraph::FindRootComboNode(EYogComboGraphInputAction InputAction) const
{
	for (const UGenericGraphNode* RootNode : RootNodes)
	{
		const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(RootNode);
		if (ComboNode && DoesInputMatch(ComboNode->RootInputAction, InputAction))
		{
			return ComboNode;
		}
	}

	for (const UGenericGraphNode* Node : AllNodes)
	{
		const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
		if (ComboNode && ComboNode->ParentNodes.IsEmpty() && DoesInputMatch(ComboNode->RootInputAction, InputAction))
		{
			return ComboNode;
		}
	}

	return nullptr;
}

const UGameplayAbilityComboGraphNode* UGameplayAbilityComboGraph::FindChildComboNode(FName ParentNodeId, EYogComboGraphInputAction InputAction, const FGameplayTagContainer* OwnedTags) const
{
	if (ParentNodeId.IsNone())
	{
		return nullptr;
	}

	for (const UGenericGraphNode* Node : AllNodes)
	{
		const UGameplayAbilityComboGraphNode* ParentComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
		const FName CandidateId = GetRuntimeNodeId(ParentComboNode);
		if (!ParentComboNode || CandidateId != ParentNodeId)
		{
			continue;
		}

		for (UGenericGraphNode* ChildNode : ParentComboNode->ChildrenNodes)
		{
			UGenericGraphEdge* const* EdgePtr = ParentComboNode->Edges.Find(ChildNode);
			const UGameplayAbilityComboGraphEdge* Edge = EdgePtr ? Cast<UGameplayAbilityComboGraphEdge>(*EdgePtr) : nullptr;
			const UGameplayAbilityComboGraphNode* ChildComboNode = Cast<UGameplayAbilityComboGraphNode>(ChildNode);
			if (!ChildComboNode || !Edge)
			{
				continue;
			}
			if (!DoesInputMatch(Edge->InputAction, InputAction))
			{
				continue;
			}
			if (!DoesEdgeStateMatch(Edge->StateRequirement, OwnedTags))
			{
				continue;
			}
			return ChildComboNode;
		}
	}

	return nullptr;
}

TArray<TSubclassOf<UGameplayAbilityComboGraphNode>> UGameplayAbilityComboGraph::GetSupportedNodeClasses() const
{
	TArray<TSubclassOf<UGameplayAbilityComboGraphNode>> NodeClasses;
	if (NodeType && NodeType->IsChildOf(UGameplayAbilityComboGraphNode::StaticClass()))
	{
		NodeClasses.Add(TSubclassOf<UGameplayAbilityComboGraphNode>(NodeType.Get()));
	}
	return NodeClasses;
}

void UGameplayAbilityComboGraph::ValidateComboGraph(TArray<FText>& OutWarnings) const
{
	OutWarnings.Reset();

	TSet<FName> SeenNodeIds;
	TMap<EYogComboGraphInputAction, FName> RootInputs;
	TMap<FString, FName> ChildInputsByParent;
	for (const UGenericGraphNode* Node : AllNodes)
	{
		const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
		if (!ComboNode)
		{
			OutWarnings.Add(LOCTEXT("InvalidNodeClass", "Combo graph contains a non-combo node."));
			continue;
		}

		const FName RuntimeNodeId = GetRuntimeNodeId(ComboNode);
		if (RuntimeNodeId.IsNone())
		{
			OutWarnings.Add(LOCTEXT("MissingNodeId", "Combo node has no runtime node id."));
		}
		else if (SeenNodeIds.Contains(RuntimeNodeId))
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Duplicate combo node id: %s."), *RuntimeNodeId.ToString())));
		}
		SeenNodeIds.Add(RuntimeNodeId);

		if (!ComboNode->Montage)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has no Montage."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->bUseNodeComboWindow && ComboNode->ComboWindowEndFrame < ComboNode->ComboWindowStartFrame)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has ComboWindowEndFrame before ComboWindowStartFrame."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->bUseNodeComboWindow && ComboNode->TotalFrames > 0 &&
			ComboNode->ComboWindowEndFrame > ComboNode->TotalFrames)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has ComboWindowEndFrame after TotalFrames."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->ParentNodes.IsEmpty())
		{
			const EYogComboGraphInputAction RootInputAction = NormalizeInputAction(ComboNode->RootInputAction);
			if (!IsKnownInputAction(RootInputAction) || RootInputAction == EYogComboGraphInputAction::Any)
			{
				OutWarnings.Add(FText::FromString(FString::Printf(
					TEXT("Root node %s has an invalid root input."),
					*RuntimeNodeId.ToString())));
			}
			else if (const FName* ExistingRoot = RootInputs.Find(RootInputAction))
			{
				OutWarnings.Add(FText::FromString(FString::Printf(
					TEXT("Root nodes %s and %s use the same root input %s."),
					*ExistingRoot->ToString(),
					*RuntimeNodeId.ToString(),
					*InputActionToString(RootInputAction))));
			}
			else
			{
				RootInputs.Add(RootInputAction, RuntimeNodeId);
			}
		}

		for (const UGenericGraphNode* ChildNode : ComboNode->ChildrenNodes)
		{
			const UGameplayAbilityComboGraphNode* ChildComboNode = Cast<UGameplayAbilityComboGraphNode>(ChildNode);
			const UGenericGraphEdge* const* EdgePtr = ComboNode->Edges.Find(const_cast<UGenericGraphNode*>(ChildNode));
			const UGameplayAbilityComboGraphEdge* ComboEdge = EdgePtr ? Cast<UGameplayAbilityComboGraphEdge>(*EdgePtr) : nullptr;
			if (!ChildComboNode || !ComboEdge)
			{
				OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has a child connection without a combo edge."), *RuntimeNodeId.ToString())));
				continue;
			}

			const FName ChildNodeId = GetRuntimeNodeId(ChildComboNode);
			const EYogComboGraphInputAction EdgeInputAction = NormalizeInputAction(ComboEdge->InputAction);
			if (!IsKnownInputAction(EdgeInputAction) || EdgeInputAction == EYogComboGraphInputAction::Any)
			{
				OutWarnings.Add(FText::FromString(FString::Printf(
					TEXT("Edge from %s to %s has an invalid input."),
					*RuntimeNodeId.ToString(),
					*ChildNodeId.ToString())));
				continue;
			}

			const FString InputKeyPart = InputActionToString(EdgeInputAction);
			const FString ChildInputKey = FString::Printf(TEXT("%s:%s"), *RuntimeNodeId.ToString(), *InputKeyPart);
			if (const FName* ExistingChild = ChildInputsByParent.Find(ChildInputKey))
			{
				OutWarnings.Add(FText::FromString(FString::Printf(
					TEXT("Node %s has multiple children for input %s: %s and %s."),
					*RuntimeNodeId.ToString(),
					*InputKeyPart,
					*ExistingChild->ToString(),
					*ChildNodeId.ToString())));
			}
			else
			{
				ChildInputsByParent.Add(ChildInputKey, ChildNodeId);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
