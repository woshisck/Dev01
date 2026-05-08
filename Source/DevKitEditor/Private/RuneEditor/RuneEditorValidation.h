#pragma once

#include "CoreMinimal.h"

class URuneDataAsset;

enum class ERuneEditorValidationSeverity : uint8
{
	Info,
	Warning,
	Error
};

struct FRuneEditorValidationIssue
{
	FRuneEditorValidationIssue() = default;
	FRuneEditorValidationIssue(ERuneEditorValidationSeverity InSeverity, const FText& InMessage)
		: Severity(InSeverity)
		, Message(InMessage)
	{
	}

	ERuneEditorValidationSeverity Severity = ERuneEditorValidationSeverity::Info;
	FText Message;
};

struct FRuneEditorValidationResult
{
	TArray<FRuneEditorValidationIssue> Issues;

	bool HasErrors() const;
	int32 CountSeverity(ERuneEditorValidationSeverity Severity) const;
	FText BuildSummaryText() const;
	FText BuildDetailsText() const;
};

class FRuneEditorValidation
{
public:
	static FRuneEditorValidationResult ValidateRuneGraph(URuneDataAsset* Rune);
};
