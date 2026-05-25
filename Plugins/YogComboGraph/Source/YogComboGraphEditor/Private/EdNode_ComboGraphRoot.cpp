#include "EdNode_ComboGraphRoot.h"

#define LOCTEXT_NAMESPACE "EdNode_ComboGraphRoot"

void UEdNode_ComboGraphRoot::AllocateDefaultPins()
{
	// Input pin is hidden — connections to the root are blocked by the schema.
	// It must still exist at Pins[0] so the shared GetInputPin/GetOutputPin
	// helpers (which assume Input=Pins[0], Output=Pins[1]) work correctly.
	UEdGraphPin* InPin = CreatePin(EGPD_Input, TEXT("MultipleNodes"), FName(), TEXT("In"));
	if (InPin)
	{
		InPin->bHidden = true;
	}
	CreatePin(EGPD_Output, TEXT("MultipleNodes"), FName(), TEXT("Out"));
}

FText UEdNode_ComboGraphRoot::GetNodeTitle(ENodeTitleType::Type /*TitleType*/) const
{
	return LOCTEXT("RootNodeTitle", "Root");
}

FLinearColor UEdNode_ComboGraphRoot::GetBackgroundColor() const
{
	return FLinearColor(0.08f, 0.08f, 0.08f, 1.f);
}

#undef LOCTEXT_NAMESPACE
