#include "Performance/UE58ScenePerformanceAuditCommandlet.h"

#include "Commandlets/CommandletReportUtils.h"
#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "MaterialBatch/MaterialBatchAuditHelpers.h"
#include "Misc/Parse.h"
#include "Performance/UE58ScenePerformanceAudit.h"

namespace
{
const TCHAR* UE58ScenePerformanceAuditReportFileName = TEXT("UE58ScenePerformanceAuditReport.md");

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

UUE58ScenePerformanceAuditCommandlet::UUE58ScenePerformanceAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UUE58ScenePerformanceAuditCommandlet::Main(const FString& Params)
{
	const FString MapPath = GetParamValue(
		Params,
		TEXT("Map="),
		TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01"));

	const FString MapFilename = FMaterialBatchAuditHelpers::ResolveMapFilename(MapPath);
	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);
	if (World)
	{
		for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
		{
			if (StreamingLevel)
			{
				StreamingLevel->SetShouldBeLoaded(true);
				StreamingLevel->SetShouldBeVisible(true);
			}
		}
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
	}

	const FUE58ScenePerformanceAuditResult Result =
		FUE58ScenePerformanceAuditBuilder::AuditWorld(World, MapPath);
	TArray<FString> ReportLines = FUE58ScenePerformanceAuditBuilder::BuildMarkdownReport(Result);
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Usage"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("```text"));
	ReportLines.Add(TEXT("UnrealEditor-Cmd.exe DevKit.uproject -run=UE58ScenePerformanceAudit -Map=/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01"));
	ReportLines.Add(TEXT("```"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("- Mode: report-only; no map or asset packages are saved."));

	FString SavedReportPath;
	FString SharedReportPath;
	if (!DevKitEditorCommandletReports::SaveReportLines(
		UE58ScenePerformanceAuditReportFileName,
		ReportLines,
		SavedReportPath,
		SharedReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("UE58ScenePerformanceAudit could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("UE58ScenePerformanceAudit wrote: %s"), *SavedReportPath);
	UE_LOG(LogTemp, Display, TEXT("UE58ScenePerformanceAudit wrote shared report: %s"), *SharedReportPath);
	return Result.bLoaded ? 0 : 1;
}
