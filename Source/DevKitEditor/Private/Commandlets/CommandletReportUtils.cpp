#include "Commandlets/CommandletReportUtils.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DevKitEditorCommandletReports
{
	namespace
	{
		const TCHAR* SharedReportDirectory = TEXT("Docs/GeneratedReports/CommandletReports");

		bool EnsureDirectoryForFile(const FString& FilePath)
		{
			const FString Directory = FPaths::GetPath(FilePath);
			return IFileManager::Get().MakeDirectory(*Directory, true);
		}
	}

	FString GetSavedReportPath(const FString& ReportFileName)
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), ReportFileName);
	}

	FString GetSharedReportPath(const FString& ReportFileName)
	{
		return FPaths::Combine(FPaths::ProjectDir(), SharedReportDirectory, ReportFileName);
	}

	bool SaveReportString(const FString& ReportFileName, const FString& ReportText, FString& OutSavedReportPath, FString& OutSharedReportPath)
	{
		OutSavedReportPath = GetSavedReportPath(ReportFileName);
		OutSharedReportPath = GetSharedReportPath(ReportFileName);

		EnsureDirectoryForFile(OutSavedReportPath);
		EnsureDirectoryForFile(OutSharedReportPath);

		const bool bSavedLocal = FFileHelper::SaveStringToFile(
			ReportText,
			*OutSavedReportPath,
			FFileHelper::EEncodingOptions::ForceUTF8);
		const bool bSavedShared = FFileHelper::SaveStringToFile(
			ReportText,
			*OutSharedReportPath,
			FFileHelper::EEncodingOptions::ForceUTF8);

		return bSavedLocal && bSavedShared;
	}

	bool SaveReportLines(const FString& ReportFileName, const TArray<FString>& ReportLines, FString& OutSavedReportPath, FString& OutSharedReportPath)
	{
		return SaveReportString(ReportFileName, FString::Join(ReportLines, LINE_TERMINATOR), OutSavedReportPath, OutSharedReportPath);
	}
}
