// Copyright 2021, Project Zero, All Rights Reserved.

#include "TopDownPhysicsCamera.h"

#define LOCTEXT_NAMESPACE "FTopDownPhysicsCameraModule"

void FTopDownPhysicsCameraModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTopDownPhysicsCameraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTopDownPhysicsCameraModule, TopDownPhysicsCamera)