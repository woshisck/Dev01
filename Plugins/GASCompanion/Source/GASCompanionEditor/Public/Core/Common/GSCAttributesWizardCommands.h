// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "UI/Styling/GSCEditorStyle.h"

class FGSCAttributesWizardCommands : public TCommands<FGSCAttributesWizardCommands>
{
public:

	FGSCAttributesWizardCommands() : TCommands<FGSCAttributesWizardCommands>
	(
		TEXT("GASCompanionEditor"),
		NSLOCTEXT("Contexts", "GASCompanionEditor", "GASCompanionEditor Plugin"),
		NAME_None,
		FGSCEditorStyle::GetStyleSetName()
	)
	{}

	TSharedPtr< FUICommandInfo > OpenPluginWindow;

	// TCommands<> interface
	virtual void RegisterCommands() override;
};
