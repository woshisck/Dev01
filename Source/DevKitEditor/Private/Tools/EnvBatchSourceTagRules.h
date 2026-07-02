#pragma once

#include "CoreMinimal.h"

struct FEnvBatchSourceTagSpec
{
	FString LevelName;
	FString ActorKind = TEXT("Prop");
	FString ProcessingMode = TEXT("Batched");
	FString VTCGroup;
	int32 SerialNumber = 1;
	bool bHasExplicitVTCGroup = false;
};

enum class EEnvBatchVTCChannel : uint8
{
	BaseColor,
	Normal,
	ORM,
	MaterialLight
};

FString GetDefaultEnvBatchVTCGroup();
FString GetEnvBatchVTCChannelName(EEnvBatchVTCChannel Channel);
FString SanitizeEnvBatchTagToken(FString Value, const FString& Fallback);
bool IsEnvBatchSourceTagString(const FString& TagString);
bool ParseEnvBatchSourceTag(const FString& SourceTag, FEnvBatchSourceTagSpec& OutSpec);
FString BuildEnvBatchSourceTag(const FEnvBatchSourceTagSpec& Spec);
FString BuildEnvBatchSourceTagPrefix(const FEnvBatchSourceTagSpec& Spec);
FString BuildEnvBatchVTCGroupKey(const FEnvBatchSourceTagSpec& Spec);
FString BuildEnvBatchSharedPropVTCCollectionName(const FEnvBatchSourceTagSpec& Spec, EEnvBatchVTCChannel Channel);
