#include "CelesLightRuntimeModule.h"
#include "StylizedLightingSettings.h"

#define LOCTEXT_NAMESPACE "FCelesLightRuntimeModule"

void FCelesLightRuntimeModule::StartupModule()
{
	GetMutableDefault<UStylizedLightingSettings>()->ApplyToConsoleVariables();
}

void FCelesLightRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCelesLightRuntimeModule, CelesLightRuntime)
