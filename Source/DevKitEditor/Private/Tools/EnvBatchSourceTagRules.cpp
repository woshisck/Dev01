#include "Tools/EnvBatchSourceTagRules.h"

namespace
{
constexpr const TCHAR* EnvBatchSourcePrefix = TEXT("EnvBatch.Source.");
constexpr const TCHAR* DefaultVTCGroup = TEXT("TC-A");

bool IsValidActorKindToken(const FString& Token)
{
	return Token == TEXT("Prop") || Token == TEXT("Building") || Token == TEXT("Ground");
}

bool IsValidProcessingModeToken(const FString& Token)
{
	return Token == TEXT("Instance") || Token == TEXT("Batched");
}
}

FString GetDefaultEnvBatchVTCGroup()
{
	return FString(DefaultVTCGroup);
}

FString GetEnvBatchVTCChannelName(EEnvBatchVTCChannel Channel)
{
	switch (Channel)
	{
	case EEnvBatchVTCChannel::BaseColor:
		return TEXT("BaseColor");
	case EEnvBatchVTCChannel::Normal:
		return TEXT("Normal");
	case EEnvBatchVTCChannel::ORM:
		return TEXT("ORM");
	case EEnvBatchVTCChannel::MaterialLight:
		return TEXT("MaterialLight");
	default:
		return TEXT("Unknown");
	}
}

FString SanitizeEnvBatchTagToken(FString Value, const FString& Fallback)
{
	Value.TrimStartAndEndInline();

	FString Result;
	Result.Reserve(Value.Len());

	for (int32 Index = 0; Index < Value.Len(); ++Index)
	{
		const TCHAR Ch = Value[Index];
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-'))
		{
			Result.AppendChar(Ch);
		}
		else
		{
			Result.AppendChar(TEXT('_'));
		}
	}

	while (Result.ReplaceInline(TEXT("__"), TEXT("_")) > 0)
	{
	}

	return Result.IsEmpty() ? Fallback : Result;
}

bool IsEnvBatchSourceTagString(const FString& TagString)
{
	return TagString.StartsWith(EnvBatchSourcePrefix);
}

bool ParseEnvBatchSourceTag(const FString& SourceTag, FEnvBatchSourceTagSpec& OutSpec)
{
	TArray<FString> Parts;
	SourceTag.ParseIntoArray(Parts, TEXT("."), true);

	if ((Parts.Num() != 6 && Parts.Num() != 7) || Parts[0] != TEXT("EnvBatch") || Parts[1] != TEXT("Source"))
	{
		return false;
	}

	const FString& ActorKind = Parts[3];
	const FString& ProcessingMode = Parts[4];
	if (!IsValidActorKindToken(ActorKind) || !IsValidProcessingModeToken(ProcessingMode))
	{
		return false;
	}

	const bool bHasVTCGroup = Parts.Num() == 7;
	const FString& SerialPart = bHasVTCGroup ? Parts[6] : Parts[5];
	const int32 SerialNumber = FCString::Atoi(*SerialPart);
	if (SerialNumber <= 0)
	{
		return false;
	}

	OutSpec.LevelName = SanitizeEnvBatchTagToken(Parts[2], TEXT("Level"));
	OutSpec.ActorKind = ActorKind;
	OutSpec.ProcessingMode = ProcessingMode;
	OutSpec.VTCGroup = bHasVTCGroup
		? SanitizeEnvBatchTagToken(Parts[5], GetDefaultEnvBatchVTCGroup())
		: GetDefaultEnvBatchVTCGroup();
	OutSpec.SerialNumber = SerialNumber;
	OutSpec.bHasExplicitVTCGroup = bHasVTCGroup;
	return true;
}

FString BuildEnvBatchSourceTag(const FEnvBatchSourceTagSpec& Spec)
{
	if (!Spec.bHasExplicitVTCGroup)
	{
		return FString::Printf(
			TEXT("%s%s.%s.%s.%02d"),
			EnvBatchSourcePrefix,
			*SanitizeEnvBatchTagToken(Spec.LevelName, TEXT("Level")),
			*SanitizeEnvBatchTagToken(Spec.ActorKind, TEXT("Prop")),
			*SanitizeEnvBatchTagToken(Spec.ProcessingMode, TEXT("Batched")),
			FMath::Max(1, Spec.SerialNumber));
	}

	return FString::Printf(
		TEXT("%s%s.%s.%s.%s.%02d"),
		EnvBatchSourcePrefix,
		*SanitizeEnvBatchTagToken(Spec.LevelName, TEXT("Level")),
		*SanitizeEnvBatchTagToken(Spec.ActorKind, TEXT("Prop")),
		*SanitizeEnvBatchTagToken(Spec.ProcessingMode, TEXT("Batched")),
		*SanitizeEnvBatchTagToken(Spec.VTCGroup, GetDefaultEnvBatchVTCGroup()),
		FMath::Max(1, Spec.SerialNumber));
}

FString BuildEnvBatchSourceTagPrefix(const FEnvBatchSourceTagSpec& Spec)
{
	if (!Spec.bHasExplicitVTCGroup)
	{
		return FString::Printf(
			TEXT("%s%s.%s.%s."),
			EnvBatchSourcePrefix,
			*SanitizeEnvBatchTagToken(Spec.LevelName, TEXT("Level")),
			*SanitizeEnvBatchTagToken(Spec.ActorKind, TEXT("Prop")),
			*SanitizeEnvBatchTagToken(Spec.ProcessingMode, TEXT("Batched")));
	}

	return FString::Printf(TEXT("%s."), *BuildEnvBatchVTCGroupKey(Spec));
}

FString BuildEnvBatchVTCGroupKey(const FEnvBatchSourceTagSpec& Spec)
{
	return FString::Printf(
		TEXT("%s%s.%s.%s.%s"),
		EnvBatchSourcePrefix,
		*SanitizeEnvBatchTagToken(Spec.LevelName, TEXT("Level")),
		*SanitizeEnvBatchTagToken(Spec.ActorKind, TEXT("Prop")),
		*SanitizeEnvBatchTagToken(Spec.ProcessingMode, TEXT("Batched")),
		*SanitizeEnvBatchTagToken(Spec.VTCGroup, GetDefaultEnvBatchVTCGroup()));
}

FString BuildEnvBatchSharedPropVTCCollectionName(const FEnvBatchSourceTagSpec& Spec, EEnvBatchVTCChannel Channel)
{
	return FString::Printf(
		TEXT("TC_%s_%s_SharedProp_%s"),
		*SanitizeEnvBatchTagToken(Spec.LevelName, TEXT("Level")),
		*SanitizeEnvBatchTagToken(Spec.VTCGroup, GetDefaultEnvBatchVTCGroup()),
		*GetEnvBatchVTCChannelName(Channel));
}
