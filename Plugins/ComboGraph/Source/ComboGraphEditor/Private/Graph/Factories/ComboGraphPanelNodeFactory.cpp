// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Factories/ComboGraphPanelNodeFactory.h"

#include "ComboGraphEditorLog.h"
#include "Graph/Nodes/ComboGraphEdNodeEdge.h"
#include "Graph/Nodes/ComboGraphEdNode.h"
#include "Graph/Nodes/ComboGraphEdNodeConduit.h"
#include "Graph/Nodes/ComboGraphEdNodeEntry.h"
#include "Graph/Slate/SComboGraphNode.h"
#include "Graph/Slate/SComboGraphEdge.h"
#include "Graph/Slate/SComboGraphNodeConduit.h"
#include "Graph/Slate/SComboGraphNodeEntry.h"
#include "Utils/ComboGraphUtils.h"

TSharedPtr<SGraphNode> FComboGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UComboGraphEdNodeEdge* GraphEdge = Cast<UComboGraphEdNodeEdge>(Node))
	{
		return SNew(SComboGraphEdge, GraphEdge);
	}

	if (UComboGraphEdNode* GraphNode = Cast<UComboGraphEdNode>(Node))
	{
		const UComboGraphProjectSettings* Settings = FComboGraphUtils::GetPluginProjectSettings();

		// const bool bDrawVerticalPins = Settings->DrawPinTypes == EComboGraphDrawPinTypes::Both || Settings->DrawPinTypes == EComboGraphDrawPinTypes::Vertical;
		// const bool bDrawHorizontalPins = Settings->DrawPinTypes == EComboGraphDrawPinTypes::Both || Settings->DrawPinTypes == EComboGraphDrawPinTypes::Horizontal;

		return SNew(SComboGraphNode, GraphNode)
			.PinSize(Settings->PinSize)
			.PinPadding(Settings->PinPadding)
			.DrawVerticalPins(true)
			.DrawHorizontalPins(true)
			.ContentPadding(Settings->ContentMargin)
			.ContentInternalPadding(Settings->ContentInternalPadding);
	}

	if (UComboGraphEdNodeEntry* EntryNode = Cast<UComboGraphEdNodeEntry>(Node))
	{
		return SNew(SComboGraphNodeEntry, EntryNode);
	}

	if (UComboGraphEdNodeConduit* ConduitNode = Cast<UComboGraphEdNodeConduit>(Node))
	{
		return SNew(SComboGraphNodeConduit, ConduitNode);
	}

	return nullptr;
}
