// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

// Intended categories:
//	Log - This happened. What gameplay programmers may care about to debug
//	Verbose - This is why this happened. What you may turn on to debug the ability system code.

GASCOMPANION_API DECLARE_LOG_CATEGORY_EXTERN(LogAbilitySystemCompanion, Display, All);
GASCOMPANION_API DECLARE_LOG_CATEGORY_EXTERN(LogAbilitySystemCompanionUI, Display, All);

namespace UE::GASCompanion::Log
{
	static FString GetWorldLogPrefix(const UWorld* InWorld)
	{
		FString WorldName;

		if (InWorld == nullptr)
		{
			WorldName = NSLOCTEXT("GSCLog", "PlayWorldBeingCreated", "(World Being Created)").ToString();
		}
		else
		{
			const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(InWorld);

			switch (InWorld->GetNetMode())
			{
			case NM_Standalone:
				WorldName = NSLOCTEXT("GSCLog", "PlayWorldIsStandalone", "Standalone").ToString();
				break;

			case NM_ListenServer:
				WorldName = NSLOCTEXT("GSCLog", "PlayWorldIsListenServer", "Listen Server").ToString();
				break;

			case NM_DedicatedServer:
				WorldName = NSLOCTEXT("GSCLog", "PlayWorldIsDedicatedServer", "Dedicated Server").ToString();
				break;

			case NM_Client:
				// 0 is always the server, use PIEInstance so it matches the in-editor UI
				WorldName = FText::Format(NSLOCTEXT("GSCLog", "PlayWorldIsClient", "Client {0}"), FText::AsNumber(WorldContext ? WorldContext->PIEInstance : 0)).ToString();
				break;

			default:
				break;
			};

			if ((WorldContext != nullptr) && !WorldContext->CustomDescription.IsEmpty())
			{
				WorldName += TEXT(" ") + WorldContext->CustomDescription;
			}
		}

		return WorldName;
	}

	static FString GetBoolText(const bool bCondition)
	{
		// Could use LexToString(bool) instead
		return bCondition ? TEXT("true") : TEXT("false");
	}

	static FColor GetOnScreenVerbosityColor(const ELogVerbosity::Type Verbosity)
	{
		return
			Verbosity == ELogVerbosity::Fatal || Verbosity == ELogVerbosity::Error ? FColor::Red :
			Verbosity == ELogVerbosity::Warning ? FColor::Yellow :
			Verbosity == ELogVerbosity::Display || Verbosity == ELogVerbosity::Log ? FColor::Cyan :
			Verbosity == ELogVerbosity::Verbose || Verbosity == ELogVerbosity::VeryVerbose ? FColor::Orange :
			FColor::Cyan;
	}

	static void AddOnScreenDebugMessage(const ELogVerbosity::Type Verbosity, const FString& Message)
	{
		if (GEngine)
		{
			const FColor Color = GetOnScreenVerbosityColor(Verbosity);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, Color, Message);
		}
	}
};

#define GSC_LOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogAbilitySystemCompanion, Verbosity, Format, ##__VA_ARGS__); \
}

#define GSC_WLOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogAbilitySystemCompanion, Verbosity, TEXT("[%s] %s - %s"), *UE::GASCompanion::Log::GetWorldLogPrefix(GetWorld()), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
}

#define GSC_PLOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogAbilitySystemCompanion, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
}

#define GSC_UI_LOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogAbilitySystemCompanionUI, Verbosity, Format, ##__VA_ARGS__); \
}

#define GSC_SLOG(Verbosity, Format, ...) \
{ \
	UE::GASCompanion::Log::AddOnScreenDebugMessage(ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
	UE_LOG(LogAbilitySystemCompanion, Verbosity, Format, ##__VA_ARGS__); \
}
