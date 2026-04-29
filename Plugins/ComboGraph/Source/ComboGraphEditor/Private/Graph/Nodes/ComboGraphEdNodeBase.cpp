// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Nodes/ComboGraphEdNodeBase.h"

UEdGraphPin* UComboGraphEdNodeBase::GetInputPin() const
{
	if (Pins.Num() == 0 || Pins[0]->Direction == EGPD_Output)
	{
		return nullptr;
	}

	return Pins[0];
}

UEdGraphPin* UComboGraphEdNodeBase::GetOutputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			return Pin;
		}
	}

	return nullptr;
}
