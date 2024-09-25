// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FComboGraphBlueprintEditorCommands  : public TCommands<FComboGraphBlueprintEditorCommands>
{
public:
	FComboGraphBlueprintEditorCommands();

	TSharedPtr<FUICommandInfo> AutoArrange;
	TSharedPtr<FUICommandInfo> AutoArrangeVertical;
	TSharedPtr<FUICommandInfo> AutoArrangeHorizontal;

	// TCommands<> interface
	virtual void RegisterCommands() override;
	// End of TCommands<> interface
};
