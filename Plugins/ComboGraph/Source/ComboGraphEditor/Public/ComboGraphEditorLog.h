// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

COMBOGRAPHEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogComboGraphEditor, Display, All);

#define CG_EDITOR_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogComboGraphEditor, Verbosity, Format, ##__VA_ARGS__); \
}

FORCEINLINE FString GetBoolText(const bool bCondition)
{
	return bCondition ? TEXT("true") : TEXT("false");
}
