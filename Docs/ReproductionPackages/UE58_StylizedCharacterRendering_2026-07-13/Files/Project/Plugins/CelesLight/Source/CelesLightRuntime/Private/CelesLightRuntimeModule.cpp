#include "CelesLightRuntimeModule.h"
#include "StylizedLightingSettings.h"

#include "Misc/CoreDelegates.h"

#define LOCTEXT_NAMESPACE "FCelesLightRuntimeModule"

void FCelesLightRuntimeModule::StartupModule()
{
	// Apply scalar settings immediately, then upload the profile and Ramp Atlas
	// again after renderer resources have been initialized.
	GetMutableDefault<UStylizedLightingSettings>()->ApplyToConsoleVariables();
	PostEngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FCelesLightRuntimeModule::ApplySettingsAfterEngineInit);
}

void FCelesLightRuntimeModule::ShutdownModule()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}
}

void FCelesLightRuntimeModule::ApplySettingsAfterEngineInit()
{
	GetMutableDefault<UStylizedLightingSettings>()->ApplyToConsoleVariables();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCelesLightRuntimeModule, CelesLightRuntime)
