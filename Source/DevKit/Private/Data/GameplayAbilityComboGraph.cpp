#include "Data/GameplayAbilityComboGraph.h"

#include "Data/MontageConfigDA.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraph"

namespace
{
	bool DoesInputMatch(ECardRequiredAction RequiredAction, ECardRequiredAction InputAction)
	{
		return RequiredAction == ECardRequiredAction::Any || RequiredAction == InputAction;
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
}

UGameplayAbilityComboGraphNode::UGameplayAbilityComboGraphNode()
{
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UGameplayAbilityComboGraph::StaticClass();
	ContextMenuName = LOCTEXT("ComboNodeContextName", "Combo Ability Node");
	BackgroundColor = FLinearColor(0.13f, 0.18f, 0.22f, 1.f);
#endif
}

FWeaponComboNodeConfig UGameplayAbilityComboGraphNode::BuildRuntimeConfig(ECardRequiredAction InputAction) const
{
	FWeaponComboNodeConfig Config;
	Config.NodeId = GetRuntimeNodeId(this);
	Config.InputAction = InputAction;
	Config.MontageConfig = MontageConfig;
	Config.AttackDataOverride = AttackDataOverride;
	Config.bIsComboFinisher = bIsComboFinisher;
	Config.bAllowDashSave = bAllowDashSave;
	Config.bOverrideComboWindow = bUseNodeComboWindow;
	Config.ComboWindowStartFrame = ComboWindowStartFrame;
	Config.ComboWindowEndFrame = ComboWindowEndFrame;
	Config.ComboWindowTotalFrames = MontageConfig ? MontageConfig->TotalFrames : 30;
	Config.CardTriggerTiming = CardTriggerTiming;
	return Config;
}

FText UGameplayAbilityComboGraphNode::GetDescription_Implementation() const
{
	const FName RuntimeNodeId = GetRuntimeNodeId(this);
	const FString MontageName = GetNameSafe(MontageConfig);
	const int32 TotalFrames = MontageConfig ? MontageConfig->TotalFrames : 0;
	return FText::FromString(FString::Printf(
		TEXT("Node=%s\nMontage=%s\nComboWindow=%s [%d-%d / %d]"),
		*RuntimeNodeId.ToString(),
		MontageConfig ? *MontageName : TEXT("None"),
		bUseNodeComboWindow ? TEXT("Node") : TEXT("Montage Notify"),
		ComboWindowStartFrame,
		ComboWindowEndFrame,
		TotalFrames));
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

	return LOCTEXT("UntitledComboNode", "Combo Ability");
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
	return StaticEnum<ECardRequiredAction>()
		? StaticEnum<ECardRequiredAction>()->GetDisplayNameTextByValue(static_cast<int64>(InputAction))
		: FText::FromString(TEXT("Input"));
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

const UGameplayAbilityComboGraphNode* UGameplayAbilityComboGraph::FindRootComboNode(ECardRequiredAction InputAction) const
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

const UGameplayAbilityComboGraphNode* UGameplayAbilityComboGraph::FindChildComboNode(FName ParentNodeId, ECardRequiredAction InputAction) const
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
			if (ChildComboNode && Edge && DoesInputMatch(Edge->InputAction, InputAction))
			{
				return ChildComboNode;
			}
		}
	}

	return nullptr;
}

void UGameplayAbilityComboGraph::ValidateComboGraph(TArray<FText>& OutWarnings) const
{
	OutWarnings.Reset();

	TSet<FName> SeenNodeIds;
	TMap<uint8, FName> RootInputs;
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

		if (!ComboNode->MontageConfig)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has no MontageConfig."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->bUseNodeComboWindow && ComboNode->ComboWindowEndFrame < ComboNode->ComboWindowStartFrame)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has ComboWindowEndFrame before ComboWindowStartFrame."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->bUseNodeComboWindow && ComboNode->MontageConfig &&
			ComboNode->ComboWindowEndFrame > ComboNode->MontageConfig->TotalFrames)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has ComboWindowEndFrame after MontageConfig TotalFrames."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->ParentNodes.IsEmpty())
		{
			const uint8 RootInputKey = static_cast<uint8>(ComboNode->RootInputAction);
			if (const FName* ExistingRoot = RootInputs.Find(RootInputKey))
			{
				OutWarnings.Add(FText::FromString(FString::Printf(
					TEXT("Root nodes %s and %s use the same RootInputAction."),
					*ExistingRoot->ToString(),
					*RuntimeNodeId.ToString())));
			}
			else
			{
				RootInputs.Add(RootInputKey, RuntimeNodeId);
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

			const FString ChildInputKey = FString::Printf(TEXT("%s:%d"), *RuntimeNodeId.ToString(), static_cast<int32>(ComboEdge->InputAction));
			const FName ChildNodeId = GetRuntimeNodeId(ChildComboNode);
			if (const FName* ExistingChild = ChildInputsByParent.Find(ChildInputKey))
			{
				OutWarnings.Add(FText::FromString(FString::Printf(
					TEXT("Node %s has multiple children for input %s: %s and %s."),
					*RuntimeNodeId.ToString(),
					*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(ComboEdge->InputAction)),
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
