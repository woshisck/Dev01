// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Slate/SComboGraphNodeEntry.h"
#include "Widgets/SBoxPanel.h"
#include "SGraphPin.h"
#include "Graph/Nodes/ComboGraphEdNodeEntry.h"

/////////////////////////////////////////////////////
// SComboGraphNodeEntry

void SComboGraphNodeEntry::Construct(const FArguments& InArgs, UComboGraphEdNodeEntry* InNode)
{
	GraphNode = InNode;

	SetCursor(EMouseCursor::CardinalCross);

	UpdateGraphNode();
}

void SComboGraphNodeEntry::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{

}

FSlateColor SComboGraphNodeEntry::GetBorderBackgroundColor() const
{
	constexpr FLinearColor InactiveStateColor(0.08f, 0.08f, 0.08f);
	return InactiveStateColor;
}

void SComboGraphNodeEntry::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();


	ContentScale.Bind(this, &SGraphNode::GetContentScale);
	GetOrAddSlot(ENodeZone::Center)
	    .HAlign(HAlign_Center)
	    .VAlign(VAlign_Center)
		[
			SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
#else
			.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
#endif
			.Padding(0)
			.BorderBackgroundColor(this, &SComboGraphNodeEntry::GetBorderBackgroundColor)
			[
				SNew(SOverlay)

				// PIN AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(10.0f)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
			]
		];

	CreatePinWidgets();
}

void SComboGraphNodeEntry::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));
	RightNodeBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillHeight(1.0f)
		[
			PinToAdd
		];
	OutputPins.Add(PinToAdd);
}

FText SComboGraphNodeEntry::GetPreviewCornerText() const
{
	return NSLOCTEXT("SComboGraphNodeEntry", "CornerTextDescription", "Entry point for state machine");
}
