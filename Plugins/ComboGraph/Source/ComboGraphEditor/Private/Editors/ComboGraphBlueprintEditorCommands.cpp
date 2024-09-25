// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/ComboGraphBlueprintEditorCommands.h"

#define LOCTEXT_NAMESPACE "ACEBlueprintEditorCommands"

FComboGraphBlueprintEditorCommands::FComboGraphBlueprintEditorCommands() : TCommands<FComboGraphBlueprintEditorCommands>(
	"ACEBlueprintEditorCommands",
	NSLOCTEXT("Contexts", "ACEBlueprintEditorCommands", "Combo Graph Editor"),
	NAME_None,
	// FEditorStyle::GetStyleSetName())
	FAppStyle::GetAppStyleSetName())
{
}

void FComboGraphBlueprintEditorCommands::RegisterCommands()
{
	UI_COMMAND(AutoArrange, "Auto Arrange", "Auto Arrange (Horizontally or Vertically, depending on last selected mode in dropdown)", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AutoArrangeVertical, "Auto Arrange (Vertically)", "Auto Arrange (Vertically) - Re-arrange graph layout vertically (from top to bottom). Similar to Behavior Trees.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AutoArrangeHorizontal, "Auto Arrange (Horizontally)", "Auto Arrange (Horizontally) - Re-arrange graph layout horizontally (from left to right). This is far from perfect, but still handy. Will be revisited and improved upon in a future update.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
