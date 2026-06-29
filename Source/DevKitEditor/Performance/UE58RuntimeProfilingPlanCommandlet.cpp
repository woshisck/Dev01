#include "Performance/UE58RuntimeProfilingPlanCommandlet.h"

#include "Commandlets/CommandletReportUtils.h"
#include "Misc/Parse.h"
#include "Performance/UE58RuntimeProfilingPlan.h"

namespace
{
const TCHAR* UE58RuntimeProfilingPlanReportFileName = TEXT("UE58RuntimeProfilingPlanReport.md");

FString GetParamValue(const FString& Params, const TCHAR* Key, const FString& DefaultValue)
{
	FString Value;
	if (FParse::Value(*Params, Key, Value))
	{
		return Value;
	}
	return DefaultValue;
}
}

UUE58RuntimeProfilingPlanCommandlet::UUE58RuntimeProfilingPlanCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UUE58RuntimeProfilingPlanCommandlet::Main(const FString& Params)
{
	FUE58RuntimeProfilingPlanOptions Options;
	Options.MapPath = GetParamValue(
		Params,
		TEXT("Map="),
		TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01"));
	Options.ClusterName = GetParamValue(Params, TEXT("Cluster="), TEXT("Prison_S_01"));
	Options.CameraLabel = GetParamValue(Params, TEXT("Camera="), TEXT("RepresentativeCamera"));

	const TArray<FString> ReportLines = FUE58RuntimeProfilingPlanBuilder::BuildMarkdownReport(Options);

	FString SavedReportPath;
	FString SharedReportPath;
	if (!DevKitEditorCommandletReports::SaveReportLines(
		UE58RuntimeProfilingPlanReportFileName,
		ReportLines,
		SavedReportPath,
		SharedReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("UE58RuntimeProfilingPlan could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("UE58RuntimeProfilingPlan wrote: %s"), *SavedReportPath);
	UE_LOG(LogTemp, Display, TEXT("UE58RuntimeProfilingPlan wrote shared report: %s"), *SharedReportPath);
	return 0;
}
