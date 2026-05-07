#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"

namespace DevKitEditorCommandletReports
{
	FString GetSavedReportPath(const FString& ReportFileName);
	FString GetSharedReportPath(const FString& ReportFileName);
	bool SaveReportString(const FString& ReportFileName, const FString& ReportText, FString& OutSavedReportPath, FString& OutSharedReportPath);
	bool SaveReportLines(const FString& ReportFileName, const TArray<FString>& ReportLines, FString& OutSavedReportPath, FString& OutSharedReportPath);
}
