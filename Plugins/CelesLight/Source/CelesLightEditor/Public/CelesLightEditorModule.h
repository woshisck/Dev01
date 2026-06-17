#pragma once

#include "Modules/ModuleManager.h"

class UToolMenu;

class FCelesLightEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void FillCelesLightMenu(UToolMenu* Menu);
	void CreateCaptureBox();
	void ManualUpdateCelesLights();
};
