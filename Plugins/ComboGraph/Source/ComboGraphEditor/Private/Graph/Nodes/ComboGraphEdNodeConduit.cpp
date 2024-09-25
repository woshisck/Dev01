// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Nodes/ComboGraphEdNodeConduit.h"

#include "ComboGraphEditorTypes.h"

#define LOCTEXT_NAMESPACE "ComboGraphEdNodeConduit"

UComboGraphEdNodeConduit::UComboGraphEdNodeConduit()
{
	bCanRenameNode = false;
}

void UComboGraphEdNodeConduit::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UComboGraphPinNames::PinCategory_Transition, TEXT("In"));
	CreatePin(EGPD_Output, UComboGraphPinNames::PinCategory_Transition, TEXT("Out"));
}

void UComboGraphEdNodeConduit::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

FText UComboGraphEdNodeConduit::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("ConduitNodeTitle", "Conduit");
}

FText UComboGraphEdNodeConduit::GetTooltipText() const
{
	return LOCTEXT("ConduitNodeTooltip", "This is a conduit, which allows branching out beginning of combo graph based on an initial Transition Input");
}

void UComboGraphEdNodeConduit::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	// TODO: Consider validation error for this node
	Super::ValidateNodeDuringCompilation(MessageLog);
}

FString UComboGraphEdNodeConduit::GetDocumentationExcerptName() const
{
	// TODO: Figure out a way to do our own extensive documentation (when holding Ctrl+Alt when hovering)
	return TEXT("UAnimStateNode");
}

FString UComboGraphEdNodeConduit::GetDocumentationLink() const
{
	// TODO: Figure out a way to do our own extensive documentation (when holding Ctrl+Alt when hovering)
	return TEXT("Shared/GraphNodes/AnimationStateMachine");
}

#undef LOCTEXT_NAMESPACE
