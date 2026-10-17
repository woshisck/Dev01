#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"

/** Value snapshots only; Lua receives no UObject, filesystem, network or tag-write capabilities. */
struct DEVKITLUARULES_API FDevKitLuaRuleContext
{
    TArray<FString> OwnedTags;
    TArray<FString> RequestTags;
    TMap<FString, double> Values;
    FString Subject;
};

/**
 * A validated source holder, not a shared mutable Lua state. Every call uses a fresh bounded VM.
 * LoadSource failure preserves the last validated source. EvaluateCondition failure sets bMatch=false;
 * the caller must treat the false return as an evaluation error, never as permission to activate.
 */
class DEVKITLUARULES_API FDevKitLuaRuleVM final
{
public:
    static constexpr int32 MaxSourceBytes = 64 * 1024;
    static constexpr int32 MaxInstructions = 100000;
    static constexpr int32 MaxMemoryBytes = 4 * 1024 * 1024;

    bool LoadSource(const FString& Source, FString& Error);
    /** Queries exports from the last successful validation; never executes a predicate. */
    bool HasCondition(const FString& FunctionName) const;
    bool EvaluateCondition(const FString& FunctionName, const FDevKitLuaRuleContext& Context, bool& bMatch, FString& Error);

private:
    mutable FCriticalSection Mutex;
    FString ValidatedSource;
    TSet<FString> ValidatedConditions;
};
