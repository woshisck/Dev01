#pragma once

#include "Modules/ModuleManager.h"

class FSceneViewExtensionBase;

class FCelesLightRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void ApplySettingsAfterEngineInit();

	FDelegateHandle PostEngineInitHandle;
	TSharedPtr<FSceneViewExtensionBase, ESPMode::ThreadSafe> NativeRenderStyleExtension;
};
