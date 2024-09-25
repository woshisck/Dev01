// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FComboGraphModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	void SetupSettings();
	void ShutdownSettings();

	bool HandleSettingsSaved();

	void OnPostEngineInit();
};
