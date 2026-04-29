// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

COMBOGRAPH_API DECLARE_LOG_CATEGORY_EXTERN(LogComboGraph, Display, All);

class FComboGraphScreenLogger
{
public:

	static FColor GetOnScreenVerbosityColor(const ELogVerbosity::Type Verbosity)
	{
		return
			Verbosity == ELogVerbosity::Fatal || Verbosity == ELogVerbosity::Error ? FColor::Red :
			Verbosity == ELogVerbosity::Warning ? FColor::Yellow :
			Verbosity == ELogVerbosity::Display || Verbosity == ELogVerbosity::Log ? FColor::Cyan :
			Verbosity == ELogVerbosity::Verbose || Verbosity == ELogVerbosity::VeryVerbose ? FColor::Orange :
			FColor::Cyan;
	}

	static void AddOnScreenDebugMessage(const ELogVerbosity::Type Verbosity, const FString Message)
	{
		if (GEngine)
		{
			const FColor Color = GetOnScreenVerbosityColor(Verbosity);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, Color, Message);
		}
	}
};

#define CG_RUNTIME_LOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogComboGraph, Verbosity, Format, ##__VA_ARGS__); \
}

#define CG_RUNTIME_CLOG(Condition, Verbosity, Format, ...) \
{ \
	UE_CLOG(Condition, LogComboGraph, Verbosity, Format, ##__VA_ARGS__); \
}

#define CG_RUNTIME_SLOG(Verbosity, Format, ...) \
{ \
	FComboGraphScreenLogger::AddOnScreenDebugMessage(ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
	UE_LOG(LogComboGraph, Verbosity, Format, ##__VA_ARGS__); \
}

#define CG_RUNTIME_LOG_SCREEN(Verbosity, bPrintScreen, Format, ...) \
{ \
	if (bPrintScreen) \
	{ \
		FComboGraphScreenLogger::AddOnScreenDebugMessage(ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
	} \
	UE_LOG(LogComboGraph, Verbosity, Format, ##__VA_ARGS__); \
}

FORCEINLINE FString GetBoolText(const bool bCondition)
{
	return bCondition ? TEXT("true") : TEXT("false");
}
