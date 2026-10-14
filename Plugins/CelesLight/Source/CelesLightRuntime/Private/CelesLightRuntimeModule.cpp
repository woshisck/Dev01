#include "CelesLightRuntimeModule.h"
#include "StylizedLightingSettings.h"
#include "SceneViewExtension.h"
#include "Engine/Engine.h"

#include "Misc/CoreDelegates.h"

#define LOCTEXT_NAMESPACE "FCelesLightRuntimeModule"

void FCelesLightRuntimeModule::StartupModule()
{
	// Settings/CDOs are not ready during early startup. After initialization,
	// upload the published cloud lighting/profile contract; no newer display
	// grading or generic material-lighting extension is installed.
	if (GEngine && GEngine->IsInitialized())
	{
		// Also support module startup after the one-shot engine-init notification.
		ApplySettingsAfterEngineInit();
	}
	else
	{
		PostEngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FCelesLightRuntimeModule::ApplySettingsAfterEngineInit);
	}
}

void FCelesLightRuntimeModule::ShutdownModule()
{
	NativeRenderStyleExtension.Reset();
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}
}

void FCelesLightRuntimeModule::ApplySettingsAfterEngineInit()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}
	GetMutableDefault<UStylizedLightingSettings>()->ApplyToConsoleVariables();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCelesLightRuntimeModule, CelesLightRuntime)
