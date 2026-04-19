// Fill out your copyright notice in the Description page of Project Settings.

#include "DevKit.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDevKitModule, DevKit, "DevKit" );

void FDevKitModule::StartupModule()
{
    // 注册 /Project 虚拟路径 → [ProjectDir]/Shaders/
    // 使材质 Custom 节点可以 #include "/Project/MyFile.ush"
    FString ShaderDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDir);
}

// Logging definition
DEFINE_LOG_CATEGORY(DevKitGame);

DEFINE_LOG_CATEGORY(DevKitInit);

DEFINE_LOG_CATEGORY(DevKitAI);

DEFINE_LOG_CATEGORY(DevKitLevelSystem);

DEFINE_LOG_CATEGORY(DevKitCriticalErrors);
