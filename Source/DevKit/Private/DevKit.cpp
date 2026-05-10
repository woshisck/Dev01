// Fill out your copyright notice in the Description page of Project Settings.

#include "DevKit.h"

#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDevKitModule, DevKit, "DevKit");

void FDevKitModule::StartupModule()
{
	FString ShaderDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDir);
}

DEFINE_LOG_CATEGORY(DevKitGame);
DEFINE_LOG_CATEGORY(DevKitInit);
DEFINE_LOG_CATEGORY(DevKitAI);
DEFINE_LOG_CATEGORY(DevKitLevelSystem);
DEFINE_LOG_CATEGORY(DevKitCriticalErrors);
