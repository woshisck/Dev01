#include "MaterialBatch/MaterialBatchMaterialAuditCommandlet.h"

#include "Commandlets/CommandletReportUtils.h"
#include "MaterialBatch/MaterialBatchMaterialAudit.h"
#include "Misc/Parse.h"

namespace
{
const TCHAR* MaterialBatchMaterialAuditReportFileName = TEXT("MaterialBatchMaterialAuditReport.md");

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

UMaterialBatchMaterialAuditCommandlet::UMaterialBatchMaterialAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMaterialBatchMaterialAuditCommandlet::Main(const FString& Params)
{
	const FString MaterialPath = GetParamValue(
		Params,
		TEXT("Material="),
		TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building"));

	FMaterialBatchMaterialAuditResult Result = FMaterialBatchMaterialAuditBuilder::AuditMaterial(MaterialPath);
	TArray<FString> ReportLines = FMaterialBatchMaterialAuditBuilder::BuildMarkdownReport(Result);

	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Usage"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("```text"));
	ReportLines.Add(TEXT("UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchMaterialAudit -Material=/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building"));
	ReportLines.Add(TEXT("```"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("- Mode: report-only; no assets are modified or generated."));

	FString SavedReportPath;
	FString SharedReportPath;
	const bool bSaved = DevKitEditorCommandletReports::SaveReportLines(
		MaterialBatchMaterialAuditReportFileName,
		ReportLines,
		SavedReportPath,
		SharedReportPath);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchMaterialAudit could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("MaterialBatchMaterialAudit wrote: %s"), *SavedReportPath);
	UE_LOG(LogTemp, Display, TEXT("MaterialBatchMaterialAudit wrote shared report: %s"), *SharedReportPath);
	return Result.bLoaded ? 0 : 1;
}
