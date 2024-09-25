// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Slate/SComboGraphConduitOutputPin.h"

void SComboGraphConduitOutputPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	SetCursor(EMouseCursor::Default);

	bShowLabel = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	// Set up a hover for pins that is tinted the color of the pin.
	SBorder::Construct(
		SBorder::FArguments()
	   .BorderImage(this, &SComboGraphConduitOutputPin::GetPinBorder)
	   .BorderBackgroundColor(this, &SComboGraphConduitOutputPin::GetPinColor)
	   .OnMouseButtonDown(this, &SComboGraphConduitOutputPin::OnPinMouseDown)
	   .Cursor(this, &SComboGraphConduitOutputPin::GetPinCursor)
	);
}

TSharedRef<SWidget> SComboGraphConduitOutputPin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SComboGraphConduitOutputPin::GetPinBorder() const
{
	return IsHovered()
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	   ? FAppStyle::GetBrush(TEXT("Graph.StateNode.Pin.BackgroundHovered"))
	   : FAppStyle::GetBrush(TEXT("Graph.StateNode.Pin.Background"));
#else
	   ? FEditorStyle::GetBrush(TEXT("Graph.StateNode.Pin.BackgroundHovered"))
	   : FEditorStyle::GetBrush(TEXT("Graph.StateNode.Pin.Background"));
#endif
}
