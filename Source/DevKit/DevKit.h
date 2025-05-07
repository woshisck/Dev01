// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "EngineMinimal.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "YogTypes.h"


//General Log
DEVKIT_API DECLARE_LOG_CATEGORY_EXTERN(LogGame, Log, All);
//Logging during game startup
DEVKIT_API DECLARE_LOG_CATEGORY_EXTERN(LogGameInit, Log, All);
//Logging for your AI system
DEVKIT_API DECLARE_LOG_CATEGORY_EXTERN(LogGameAI, Log, All);
//Logging for a that level system
DEVKIT_API DECLARE_LOG_CATEGORY_EXTERN(LogLevelSystem, Log, All);
//Logging for Critical Errors that must always be addressed
DEVKIT_API DECLARE_LOG_CATEGORY_EXTERN(LogCriticalErrors, Log, All);