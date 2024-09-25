// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Core/Common/GSCAttributesWizardCommands.h"

#define LOCTEXT_NAMESPACE "FGASCompanionEditorModule"

void FGSCAttributesWizardCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "GAS Companion", "Bring up GAS Companion New AttributeSet Class Dialog", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
