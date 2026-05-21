#include "Data/WeaponComboConfigDA.h"

#include "Data/MontageConfigDA.h"

namespace
{
	bool DoesComboInputMatch(ECardRequiredAction RequiredAction, ECardRequiredAction InputAction)
	{
		return RequiredAction == ECardRequiredAction::Any || RequiredAction == InputAction;
	}
}

FWeaponComboNodeConfig UWeaponComboConfigDA::FindNodeChecked(FName NodeId) const
{
	if (const FWeaponComboNodeConfig* Node = FindNode(NodeId))
	{
		return *Node;
	}

	return FWeaponComboNodeConfig();
}

const FWeaponComboNodeConfig* UWeaponComboConfigDA::FindNode(FName NodeId) const
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}

	return Nodes.FindByPredicate([NodeId](const FWeaponComboNodeConfig& Node)
	{
		return Node.NodeId == NodeId;
	});
}

const FWeaponComboNodeConfig* UWeaponComboConfigDA::FindRootNode(ECardRequiredAction InputAction) const
{
	for (const FName RootNodeId : RootNodes)
	{
		const FWeaponComboNodeConfig* Node = FindNode(RootNodeId);
		if (Node && DoesComboInputMatch(Node->InputAction, InputAction))
		{
			return Node;
		}
	}

	for (const FWeaponComboNodeConfig& Node : Nodes)
	{
		if (Node.ParentNodeId.IsNone() && DoesComboInputMatch(Node.InputAction, InputAction))
		{
			return &Node;
		}
	}

	return nullptr;
}

const FWeaponComboNodeConfig* UWeaponComboConfigDA::FindChildNode(FName ParentNodeId, ECardRequiredAction InputAction) const
{
	if (ParentNodeId.IsNone())
	{
		return nullptr;
	}

	return Nodes.FindByPredicate([ParentNodeId, InputAction](const FWeaponComboNodeConfig& Node)
	{
		return Node.ParentNodeId == ParentNodeId && DoesComboInputMatch(Node.InputAction, InputAction);
	});
}

void UWeaponComboConfigDA::ValidateConfig(TArray<FText>& OutWarnings) const
{
	OutWarnings.Reset();

	TSet<FName> SeenNodeIds;
	for (const FWeaponComboNodeConfig& Node : Nodes)
	{
		if (Node.NodeId.IsNone())
		{
			OutWarnings.Add(FText::FromString(TEXT("Combo node has empty NodeId.")));
			continue;
		}

		if (SeenNodeIds.Contains(Node.NodeId))
		{
			OutWarnings.Add(FText::FromString(FString::Printf(TEXT("Duplicate combo NodeId: %s."), *Node.NodeId.ToString())));
		}
		SeenNodeIds.Add(Node.NodeId);

		if (!Node.ParentNodeId.IsNone() && !FindNode(Node.ParentNodeId))
		{
			OutWarnings.Add(FText::FromString(FString::Printf(
				TEXT("Combo node %s references missing ParentNodeId %s."),
				*Node.NodeId.ToString(),
				*Node.ParentNodeId.ToString())));
		}

		if (!Node.MontageConfig)
		{
			OutWarnings.Add(FText::FromString(FString::Printf(
				TEXT("Combo node %s has no MontageConfig."),
				*Node.NodeId.ToString())));
		}
	}

	if (RootNodes.IsEmpty())
	{
		const bool bHasImplicitRoot = Nodes.ContainsByPredicate([](const FWeaponComboNodeConfig& Node)
		{
			return Node.ParentNodeId.IsNone();
		});

		if (!bHasImplicitRoot)
		{
			OutWarnings.Add(FText::FromString(TEXT("Combo config has no RootNodes and no implicit root node.")));
		}
	}

	for (const FName RootNodeId : RootNodes)
	{
		if (!FindNode(RootNodeId))
		{
			OutWarnings.Add(FText::FromString(FString::Printf(
				TEXT("RootNodes references missing node %s."),
				*RootNodeId.ToString())));
		}
	}
}
