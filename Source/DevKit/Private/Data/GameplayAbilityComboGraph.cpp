#include "Data/GameplayAbilityComboGraph.h"

#include "AbilitySystem/Abilities/YogGameplayAbility.h"
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
}

UGameplayAbilityComboGraphNode::UGameplayAbilityComboGraphNode()
{
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UGameplayAbilityComboGraph::StaticClass();
	ContextMenuName = LOCTEXT("ComboNodeContextName", "Combo Ability Node");
	BackgroundColor = FLinearColor(0.13f, 0.18f, 0.22f, 1.f);
#endif
}

FGameplayTag UGameplayAbilityComboGraphNode::ResolveAbilityTag() const
{
	if (AbilityTagOverride.IsValid())
	{
		return AbilityTagOverride;
	}

	if (!GameplayAbilityClass)
	{
		return FGameplayTag();
	}

	UYogGameplayAbility* AbilityCDO = Cast<UYogGameplayAbility>(GameplayAbilityClass->GetDefaultObject());
	if (!AbilityCDO)
	{
		return FGameplayTag();
	}

	return AbilityCDO->GetFirstTagFromContainer(AbilityCDO->GetAbilityTags());
}

FWeaponComboNodeConfig UGameplayAbilityComboGraphNode::BuildRuntimeConfig(ECardRequiredAction InputAction) const
{
	FWeaponComboNodeConfig Config;
	Config.NodeId = NodeId.IsNone() ? MakeStableNodeId(this) : NodeId;
	Config.InputAction = InputAction;
	Config.AbilityTag = ResolveAbilityTag();
	Config.MontageConfig = MontageConfig;
	Config.AttackDataOverride = AttackDataOverride;
	Config.bIsComboFinisher = bIsComboFinisher;
	Config.bAllowDashSave = bAllowDashSave;
	Config.bOverrideComboWindow = bUseNodeComboWindow;
	Config.ComboWindowStartFrame = ComboWindowStartFrame;
	Config.ComboWindowEndFrame = ComboWindowEndFrame;
	Config.ComboWindowTotalFrames = ComboWindowTotalFrames;
	Config.CardTriggerTiming = CardTriggerTiming;
	return Config;
}

#if WITH_EDITOR
FText UGameplayAbilityComboGraphNode::GetNodeTitle() const
{
	if (!NodeId.IsNone())
	{
		return FText::FromName(NodeId);
	}

	if (GameplayAbilityClass)
	{
		return GameplayAbilityClass->GetDisplayNameText();
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
		const FName CandidateId = ParentComboNode && !ParentComboNode->NodeId.IsNone()
			? ParentComboNode->NodeId
			: MakeStableNodeId(ParentComboNode);
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
	for (const UGenericGraphNode* Node : AllNodes)
	{
		const UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
		if (!ComboNode)
		{
			OutWarnings.Add(LOCTEXT("InvalidNodeClass", "Combo graph contains a non-combo node."));
			continue;
		}

		const FName RuntimeNodeId = ComboNode->NodeId.IsNone() ? MakeStableNodeId(ComboNode) : ComboNode->NodeId;
		if (RuntimeNodeId.IsNone())
		{
			OutWarnings.Add(LOCTEXT("MissingNodeId", "Combo node has no runtime node id."));
		}
		else if (SeenNodeIds.Contains(RuntimeNodeId))
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Duplicate combo node id: %s."), *RuntimeNodeId.ToString())));
		}
		SeenNodeIds.Add(RuntimeNodeId);

		if (!ComboNode->GameplayAbilityClass && !ComboNode->AbilityTagOverride.IsValid())
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has no GameplayAbilityClass or AbilityTagOverride."), *RuntimeNodeId.ToString())));
		}

		if (!ComboNode->ResolveAbilityTag().IsValid())
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s cannot resolve an ability tag."), *RuntimeNodeId.ToString())));
		}

		if (!ComboNode->MontageConfig)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has no MontageConfig."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->bUseNodeComboWindow && ComboNode->ComboWindowEndFrame < ComboNode->ComboWindowStartFrame)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has ComboWindowEndFrame before ComboWindowStartFrame."), *RuntimeNodeId.ToString())));
		}
	}
}

#undef LOCTEXT_NAMESPACE
