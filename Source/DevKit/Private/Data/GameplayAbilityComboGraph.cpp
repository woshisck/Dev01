#include "Data/GameplayAbilityComboGraph.h"

#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Data/MontageConfigDA.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraph"

namespace
{
	bool DoesInputMatch(ECombatGraphInputAction RequiredAction, ECombatGraphInputAction InputAction)
	{
		return RequiredAction == ECombatGraphInputAction::Any || RequiredAction == InputAction;
	}

	ECardRequiredAction ToCardAction(ECombatGraphInputAction InputAction)
	{
		switch (InputAction)
		{
		case ECombatGraphInputAction::Light:
			return ECardRequiredAction::Light;
		case ECombatGraphInputAction::Heavy:
			return ECardRequiredAction::Heavy;
		case ECombatGraphInputAction::Dash:
		case ECombatGraphInputAction::Any:
		default:
			return ECardRequiredAction::Any;
		}
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

FWeaponComboNodeConfig UGameplayAbilityComboGraphNode::BuildRuntimeConfig(ECombatGraphInputAction InputAction) const
{
	FWeaponComboNodeConfig Config;
	Config.NodeId = GetRuntimeNodeId(this);
	Config.InputAction = ToCardAction(InputAction);
	Config.AbilityTag = ResolveAbilityTag();
	Config.GameplayAbilityClass = GameplayAbilityClass;
	Config.MontageConfig = MontageConfig;
	Config.AttackDataOverride = AttackDataOverride;
	Config.bIsComboFinisher = bIsComboFinisher;
	Config.bAllowDashSave = bAllowDashSave;
	Config.DashSaveMode = DashSaveMode;
	Config.DashSaveExpireSeconds = DashSaveExpireSeconds;
	Config.bSavePendingLinkContext = bSavePendingLinkContext;
	Config.bClearCombatTagsOnDashEnd = bClearCombatTagsOnDashEnd;
	Config.bBreakComboOnDashCancel = bBreakComboOnDashCancel;
	// 512 temporary simplification: combo windows are driven by montage notifies.
	// Keep node frame fields as display/tool data only.
	Config.bOverrideComboWindow = false;
	Config.ComboWindowStartFrame = ComboWindowStartFrame;
	Config.ComboWindowEndFrame = ComboWindowEndFrame;
	Config.ComboWindowTotalFrames = ComboWindowTotalFrames;
	Config.CardTriggerTiming = CardTriggerTiming;
	return Config;
}

FText UGameplayAbilityComboGraphNode::GetDescription_Implementation() const
{
	const FName RuntimeNodeId = GetRuntimeNodeId(this);
	const FGameplayTag AbilityTag = ResolveAbilityTag();
	const FString MontageName = GetNameSafe(MontageConfig);
	return FText::FromString(FString::Printf(
		TEXT("Node=%s\nAbility=%s\nMontage=%s\nComboWindow=Montage Notify [%d-%d / %d display only]"),
		*RuntimeNodeId.ToString(),
		AbilityTag.IsValid() ? *AbilityTag.ToString() : TEXT("None"),
		MontageConfig ? *MontageName : TEXT("None"),
		ComboWindowStartFrame,
		ComboWindowEndFrame,
		ComboWindowTotalFrames));
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
	return StaticEnum<ECombatGraphInputAction>()
		? StaticEnum<ECombatGraphInputAction>()->GetDisplayNameTextByValue(static_cast<int64>(InputAction))
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

const UGameplayAbilityComboGraphNode* UGameplayAbilityComboGraph::FindRootComboNode(ECombatGraphInputAction InputAction) const
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

const UGameplayAbilityComboGraphNode* UGameplayAbilityComboGraph::FindChildComboNode(FName ParentNodeId, ECombatGraphInputAction InputAction) const
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

		if (!ComboNode->GameplayAbilityClass && !ComboNode->AbilityTagOverride.IsValid())
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has no GameplayAbilityClass or AbilityTagOverride."), *RuntimeNodeId.ToString())));
		}

		if (!ComboNode->ResolveAbilityTag().IsValid())
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s cannot resolve an ability tag."), *RuntimeNodeId.ToString())));
		}

		if (ComboNode->RootInputAction != ECombatGraphInputAction::Dash && !ComboNode->MontageConfig)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Node %s has no MontageConfig."), *RuntimeNodeId.ToString())));
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
					*StaticEnum<ECombatGraphInputAction>()->GetNameStringByValue(static_cast<int64>(ComboEdge->InputAction)),
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
