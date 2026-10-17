#include "DevKitLuaRuleVM.h"
#include "DevKitLuaSandbox.h"
#include "Containers/StringConv.h"
#include "Misc/ScopeLock.h"

namespace
{
    bool IsSafeText(const FString& Text, int32 MaxLength)
    {
        return Text.Len() <= MaxLength && FCString::Strlen(*Text) == Text.Len();
    }

    TArray<ANSICHAR> Encode(const FString& Text)
    {
        const FTCHARToUTF8 Utf8(*Text);
        TArray<ANSICHAR> Result;
        Result.Append(Utf8.Get(), Utf8.Length() + 1);
        return Result;
    }

    bool IsIdentifier(const FString& Text)
    {
        if (Text.IsEmpty() || !IsSafeText(Text, 64)) return false;
        for (int32 Index = 0; Index < Text.Len(); ++Index)
        {
            const TCHAR Char = Text[Index];
            if (!((Char >= TEXT('A') && Char <= TEXT('Z')) || (Char >= TEXT('a') && Char <= TEXT('z'))
                || Char == TEXT('_') || (Index > 0 && Char >= TEXT('0') && Char <= TEXT('9')))) return false;
        }
        return true;
    }
}

bool FDevKitLuaRuleVM::LoadSource(const FString& Source, FString& Error)
{
    Error.Reset();
    if (Source.IsEmpty() || !IsSafeText(Source, MaxSourceBytes))
    {
        Error = TEXT("Lua source must be nonempty text without embedded NUL (maximum 64 KiB UTF-8).");
        return false;
    }
    const FTCHARToUTF8 Utf8(*Source);
    if (Utf8.Length() > MaxSourceBytes)
    {
        Error = TEXT("Lua source exceeds the 64 KiB UTF-8 limit.");
        return false;
    }
    FScopeLock Guard(&Mutex);
    ANSICHAR ErrorBuffer[1024] = {};
    TArray<ANSICHAR> Names;
    Names.SetNumZeroed(256 * 65 + 1);
    if (!DevKitLuaSandbox_Validate(Utf8.Get(), Utf8.Length(), MaxMemoryBytes, MaxInstructions,
        Names.GetData(), Names.Num(), ErrorBuffer, UE_ARRAY_COUNT(ErrorBuffer)))
    {
        Error = UTF8_TO_TCHAR(ErrorBuffer);
        return false;
    }
    TArray<FString> Exports;
    FString(UTF8_TO_TCHAR(Names.GetData())).ParseIntoArrayLines(Exports, true);
    ValidatedConditions.Reset();
    for (const FString& Name : Exports) ValidatedConditions.Add(Name);
    ValidatedSource = Source;
    return true;
}

bool FDevKitLuaRuleVM::HasCondition(const FString& FunctionName) const
{
    FScopeLock Guard(&Mutex);
    return ValidatedConditions.Contains(FunctionName);
}

bool FDevKitLuaRuleVM::EvaluateCondition(const FString& FunctionName, const FDevKitLuaRuleContext& Context,
    bool& bMatch, FString& Error)
{
    bMatch = false;
    Error.Reset();
    if (!IsIdentifier(FunctionName))
    {
        Error = TEXT("Lua condition name must be an ASCII identifier of 1-64 characters.");
        return false;
    }
    if (Context.OwnedTags.Num() > 512 || Context.RequestTags.Num() > 512 || Context.Values.Num() > 128
        || !IsSafeText(Context.Subject, 128))
    {
        Error = TEXT("Lua context exceeds its bounded snapshot limits (512 tags/set, 128 values, subject 128 characters).");
        return false;
    }
    TArray<TArray<ANSICHAR>> OwnedStorage;
    TArray<TArray<ANSICHAR>> RequestStorage;
    TArray<TArray<ANSICHAR>> ValueStorage;
    TArray<const ANSICHAR*> OwnedPointers;
    TArray<const ANSICHAR*> RequestPointers;
    TArray<DevKitLuaNumber> Numbers;
    OwnedStorage.Reserve(Context.OwnedTags.Num());
    RequestStorage.Reserve(Context.RequestTags.Num());
    ValueStorage.Reserve(Context.Values.Num());
    for (const FString& Tag : Context.OwnedTags)
    {
        if (Tag.IsEmpty() || !IsSafeText(Tag, 512))
        {
            Error = TEXT("Owned tag is empty, too long or contains NUL.");
            return false;
        }
        OwnedStorage.Add(Encode(Tag));
        OwnedPointers.Add(OwnedStorage.Last().GetData());
    }
    for (const FString& Tag : Context.RequestTags)
    {
        if (Tag.IsEmpty() || !IsSafeText(Tag, 512))
        {
            Error = TEXT("Request tag is empty, too long or contains NUL.");
            return false;
        }
        RequestStorage.Add(Encode(Tag));
        RequestPointers.Add(RequestStorage.Last().GetData());
    }
    for (const TPair<FString, double>& Pair : Context.Values)
    {
        if (!IsIdentifier(Pair.Key) || !FMath::IsFinite(Pair.Value))
        {
            Error = TEXT("Context values require ASCII identifier keys and finite numbers.");
            return false;
        }
        ValueStorage.Add(Encode(Pair.Key));
        Numbers.Add({ ValueStorage.Last().GetData(), Pair.Value });
    }
    const TArray<ANSICHAR> Subject = Encode(Context.Subject);
    const TArray<ANSICHAR> Name = Encode(FunctionName);
    const DevKitLuaContext Snapshot = {
        OwnedPointers.GetData(), static_cast<size_t>(OwnedPointers.Num()),
        RequestPointers.GetData(), static_cast<size_t>(RequestPointers.Num()),
        Numbers.GetData(), static_cast<size_t>(Numbers.Num()), Subject.GetData()
    };
    FScopeLock Guard(&Mutex);
    if (ValidatedSource.IsEmpty())
    {
        Error = TEXT("No validated Lua rule source is loaded.");
        return false;
    }
    const FTCHARToUTF8 Utf8(*ValidatedSource);
    ANSICHAR ErrorBuffer[1024] = {};
    int Match = 0;
    if (!DevKitLuaSandbox_Run(Utf8.Get(), Utf8.Length(), Name.GetData(), &Snapshot,
        MaxMemoryBytes, MaxInstructions, &Match, ErrorBuffer, UE_ARRAY_COUNT(ErrorBuffer)))
    {
        Error = UTF8_TO_TCHAR(ErrorBuffer);
        return false;
    }
    bMatch = Match != 0;
    return true;
}
