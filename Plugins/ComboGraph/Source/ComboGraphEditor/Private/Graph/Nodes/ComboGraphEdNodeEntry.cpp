// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Nodes/ComboGraphEdNodeEntry.h"

#include "ComboGraphEditorTypes.h"

#define LOCTEXT_NAMESPACE "ComboGraphEdNodeEntry"

UComboGraphEdNodeEntry::UComboGraphEdNodeEntry()
{
	bCanRenameNode = false;
}

void UComboGraphEdNodeEntry::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, UComboGraphPinNames::PinCategory_EntryOut, TEXT("Entry"));
}

FText UComboGraphEdNodeEntry::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UEdGraph* Graph = GetGraph();
	return FText::FromString(Graph->GetName());
}

FText UComboGraphEdNodeEntry::GetTooltipText() const
{
	return LOCTEXT("StateEntryNodeTooltip", "Entry point for state machine");
}

UEdGraphNode* UComboGraphEdNodeEntry::GetOutputNode()
{
	if(Pins.Num() > 0 && Pins[0] != nullptr)
	{
		check(Pins[0]->LinkedTo.Num() <= 1);
		if(Pins[0]->LinkedTo.Num() > 0 && Pins[0]->LinkedTo[0]->GetOwningNode() != nullptr)
		{
			return Pins[0]->LinkedTo[0]->GetOwningNode();
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
