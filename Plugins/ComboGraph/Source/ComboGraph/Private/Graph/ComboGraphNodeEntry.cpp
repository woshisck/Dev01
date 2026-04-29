// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraphNodeEntry.h"

#define LOCTEXT_NAMESPACE "ComboGraphNodeEntry"

FText UComboGraphNodeEntry::GetNodeTitle() const
{
	return LOCTEXT("DefaultNodeTitle", "Combat Graph Entry Node");
}

#undef LOCTEXT_NAMESPACE
