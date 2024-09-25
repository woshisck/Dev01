// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Intended categories:
//	Log - This happened. What gameplay programmers may care about to debug
//	Verbose - This is why this happened. What you may turn on to debug the ability system code.
//

GASCOMPANIONEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogGASCompanionEditor, Display, All);
GASCOMPANIONEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(VLogGASCompanionEditor, Display, All);

#if NO_LOGGING || !PLATFORM_DESKTOP

// Without logging enabled we pass ability system through to UE_LOG which only handles Fatal verbosity in NO_LOGGING
#define EDITOR_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
}

#define EDITOR_VLOG(Actor, Verbosity, Format, ...) \
{ \
    UE_LOG(VLogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
    UE_VLOG(Actor, VLogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
}

#else

#define EDITOR_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
}

#define EDITOR_VLOG(Actor, Verbosity, Format, ...) \
{ \
    UE_LOG(VLogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
    UE_VLOG(Actor, VLogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
}

#endif //NO_LOGGING
