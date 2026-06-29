#pragma once

#include "CoreMinimal.h"

struct FUE58RuntimeProfilingScenario
{
	FString Name;
	FString Tier;
	FString Description;
	bool bRequiresBatchProxy = false;
	TArray<FString> CVars;
	TArray<FString> CaptureCommands;
};

struct FUE58RuntimeProfilingPlanOptions
{
	FString MapPath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	FString ClusterName = TEXT("Prison_S_01");
	FString CameraLabel = TEXT("RepresentativeCamera");
};

class FUE58RuntimeProfilingPlanBuilder
{
public:
	static TArray<FUE58RuntimeProfilingScenario> BuildDefaultScenarios();
	static TArray<FString> BuildMarkdownReport(const FUE58RuntimeProfilingPlanOptions& Options);
};
