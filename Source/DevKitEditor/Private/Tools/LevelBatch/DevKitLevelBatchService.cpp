#include "Tools/LevelBatch/DevKitLevelBatchService.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Tools/EnvBatchSourceTagRules.h"

#define LOCTEXT_NAMESPACE "DevKitLevelBatchService"

namespace
{
	const TCHAR* StatusFileName = TEXT("EnvBatchStatus.json");
	const TCHAR* GeneratedActorTagPrefix = TEXT("EnvBatch.Generated.");

	FString NormalizePackageFolder(FString PackageFolder)
	{
		PackageFolder.TrimStartAndEndInline();
		while (PackageFolder.EndsWith(TEXT("/")))
		{
			PackageFolder.LeftChopInline(1);
		}
		return PackageFolder;
	}

	FString PackageFolderToContentPath(const FString& PackageFolder)
	{
		FString RelativePath = PackageFolder;
		if (RelativePath.StartsWith(TEXT("/Game/")))
		{
			RelativePath.RightChopInline(6);
		}
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), RelativePath));
	}

	FString BuildObjectPathFromPackagePath(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
	}

	FString GetEditorCmdPath()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries/Win64/UnrealEditor-Cmd.exe")));
	}

	FString GetProjectFilePath()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	}

	bool ReadStatusToken(const FString& StatusFilePath, FString& OutStatusToken)
	{
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *StatusFilePath))
		{
			return false;
		}

		TSharedPtr<FJsonObject> RootObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
		{
			return false;
		}

		return RootObject->TryGetStringField(TEXT("status"), OutStatusToken);
	}

	EDevKitLevelBatchReviewStatus StatusFromToken(const FString& Token)
	{
		if (Token == TEXT("PendingReview"))
		{
			return EDevKitLevelBatchReviewStatus::PendingReview;
		}
		if (Token == TEXT("ArtReviewed"))
		{
			return EDevKitLevelBatchReviewStatus::ArtReviewed;
		}
		if (Token == TEXT("PerformanceApproved"))
		{
			return EDevKitLevelBatchReviewStatus::PerformanceApproved;
		}
		return EDevKitLevelBatchReviewStatus::NotBatched;
	}

	bool ActorHasGeneratedBatchTag(const AActor* Actor, const FString& LevelName)
	{
		if (!Actor)
		{
			return false;
		}

		const FString Prefix = FString::Printf(TEXT("%s%s"), GeneratedActorTagPrefix, *LevelName);
		for (const FName& Tag : Actor->Tags)
		{
			if (Tag.ToString().StartsWith(Prefix))
			{
				return true;
			}
		}
		return false;
	}

	bool ActorHasSourceTagForLevel(const AActor* Actor, const FString& LevelName)
	{
		if (!Actor)
		{
			return false;
		}

		for (const FName& Tag : Actor->Tags)
		{
			FEnvBatchSourceTagSpec Spec;
			if (ParseEnvBatchSourceTag(Tag.ToString(), Spec) && Spec.LevelName == LevelName)
			{
				return true;
			}
		}
		return false;
	}
}

FString FDevKitLevelBatchService::GetDefaultMapDataRoot()
{
	return TEXT("/Game/Art/Map/Map_Data");
}

FDevKitLevelBatchPaths FDevKitLevelBatchService::BuildPaths(const FString& LevelFolder)
{
	FDevKitLevelBatchPaths Paths;
	Paths.LevelFolder = NormalizePackageFolder(LevelFolder);
	Paths.LevelName = FPackageName::GetLongPackageAssetName(Paths.LevelFolder);
	Paths.LevelAssetFolder = Paths.LevelFolder / TEXT("LevelAsset");
	Paths.LevelMaterialFolder = Paths.LevelFolder / TEXT("LevelMaterial");
	Paths.BatchedAssetFolder = Paths.LevelFolder / TEXT("BatchedAsset");
	Paths.BakeInfoFolder = Paths.LevelFolder / TEXT("BakeInfo");
	Paths.PersistentMapPackage = Paths.LevelAssetFolder / Paths.LevelName;
	Paths.BatchedMapPackage = Paths.LevelAssetFolder / FString::Printf(TEXT("%s_Batched"), *Paths.LevelName);
	Paths.StatusFilePath = FPaths::Combine(PackageFolderToContentPath(Paths.BatchedAssetFolder), StatusFileName);
	return Paths;
}

TArray<FDevKitLevelBatchStatus> FDevKitLevelBatchService::ScanLevelFolders(const FString& RootPackagePath)
{
	TArray<FDevKitLevelBatchStatus> Result;

	const FString RootFolder = NormalizePackageFolder(RootPackagePath);
	const FString RootContentPath = PackageFolderToContentPath(RootFolder);

	TArray<FString> ChildDirectoryNames;
	IFileManager::Get().FindFiles(ChildDirectoryNames, *(RootContentPath / TEXT("*")), false, true);
	ChildDirectoryNames.Sort();

	for (const FString& ChildDirectoryName : ChildDirectoryNames)
	{
		const FDevKitLevelBatchPaths Paths = BuildPaths(RootFolder / ChildDirectoryName);
		FDevKitLevelBatchStatus Status;
		Status.Paths = Paths;
		Status.bHasLevelAssetFolder = IFileManager::Get().DirectoryExists(*PackageFolderToContentPath(Paths.LevelAssetFolder));
		Status.bHasBatchedAssetFolder = IFileManager::Get().DirectoryExists(*PackageFolderToContentPath(Paths.BatchedAssetFolder));
		Status.bHasPersistentMap = FPackageName::DoesPackageExist(Paths.PersistentMapPackage);
		Status.bHasBatchedMap = FPackageName::DoesPackageExist(Paths.BatchedMapPackage);

		if (Status.bHasBatchedAssetFolder)
		{
			TArray<FString> GeneratedFiles;
			IFileManager::Get().FindFilesRecursive(
				GeneratedFiles,
				*PackageFolderToContentPath(Paths.BatchedAssetFolder),
				TEXT("*.*"),
				true,
				false);

			for (const FString& GeneratedFile : GeneratedFiles)
			{
				if (!GeneratedFile.EndsWith(StatusFileName))
				{
					++Status.GeneratedAssetCount;
				}
			}
		}

		Status.ReviewStatus = DetermineReviewStatus(
			Paths,
			Status.bHasBatchedMap,
			Status.GeneratedAssetCount,
			Status.bHasStatusFile);
		Result.Add(Status);
	}

	return Result;
}

TOptional<FDevKitLevelBatchStatus> FDevKitLevelBatchService::FindLevelForMapPackage(const FString& MapPackagePath, const FString& RootPackagePath)
{
	FString NormalizedMapPackagePath = MapPackagePath;
	NormalizedMapPackagePath.TrimStartAndEndInline();
	int32 ObjectNameSeparatorIndex = INDEX_NONE;
	if (NormalizedMapPackagePath.FindChar(TEXT('.'), ObjectNameSeparatorIndex))
	{
		NormalizedMapPackagePath.LeftInline(ObjectNameSeparatorIndex);
	}

	for (const FDevKitLevelBatchStatus& Status : ScanLevelFolders(RootPackagePath))
	{
		if (NormalizedMapPackagePath == Status.Paths.PersistentMapPackage ||
			NormalizedMapPackagePath == Status.Paths.BatchedMapPackage ||
			NormalizedMapPackagePath.StartsWith(Status.Paths.LevelAssetFolder / TEXT("")))
		{
			return Status;
		}
	}

	return TOptional<FDevKitLevelBatchStatus>();
}

FDevKitLevelBatchCommand FDevKitLevelBatchService::BuildPartialApplyCommand(const FDevKitLevelBatchPaths& Paths, const FString& TierName)
{
	FDevKitLevelBatchCommand Command;
	Command.EditorCmdPath = GetEditorCmdPath();
	Command.OutputRoot = Paths.BatchedAssetFolder;
	Command.RequireTagPrefix = FString::Printf(TEXT("EnvBatch.Source.%s."), *Paths.LevelName);

	const FString SanitizedTierName = TierName.IsEmpty() ? TEXT("Mid") : TierName;
	Command.Arguments = FString::Printf(
		TEXT("\"%s\" -run=MaterialBatchBuild -Map=%s -Cluster=%s -Tier=%s -TextureBackend=VTAtlas -RequireTag=%s -OutputRoot=%s -ApplyVTAtlasOnly -ApplyMappingOnly -ApplyPropertyTextureOnly -ApplyProxyMeshOnly -ApplyBatchMaterialOnly -unattended -nopause -NoSound -NullRHI"),
		*GetProjectFilePath(),
		*Paths.PersistentMapPackage,
		*Paths.LevelName,
		*SanitizedTierName,
		*Command.RequireTagPrefix,
		*Command.OutputRoot);
	Command.PowerShellCommand = FString::Printf(TEXT("& \"%s\" %s"), *Command.EditorCmdPath, *Command.Arguments);
	return Command;
}

bool FDevKitLevelBatchService::LaunchPartialApplyCommand(const FDevKitLevelBatchPaths& Paths, const FString& TierName, FString& OutMessage)
{
	IFileManager::Get().MakeDirectory(*PackageFolderToContentPath(Paths.BatchedAssetFolder), true);

	const FDevKitLevelBatchCommand Command = BuildPartialApplyCommand(Paths, TierName);
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*Command.EditorCmdPath,
		*Command.Arguments,
		true,
		false,
		false,
		nullptr,
		0,
		nullptr,
		nullptr);

	if (!ProcHandle.IsValid())
	{
		OutMessage = FString::Printf(TEXT("Failed to launch MaterialBatchBuild for %s."), *Paths.LevelName);
		return false;
	}

	FPlatformProcess::CloseProc(ProcHandle);
	OutMessage = FString::Printf(TEXT("Started MaterialBatchBuild for %s. OutputRoot=%s"), *Paths.LevelName, *Paths.BatchedAssetFolder);
	return true;
}

bool FDevKitLevelBatchService::WriteReviewStatus(const FDevKitLevelBatchPaths& Paths, EDevKitLevelBatchReviewStatus Status, FString& OutError)
{
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Paths.StatusFilePath), true);

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("levelName"), Paths.LevelName);
	RootObject->SetStringField(TEXT("levelFolder"), Paths.LevelFolder);
	RootObject->SetStringField(TEXT("status"), StatusToToken(Status));
	RootObject->SetStringField(TEXT("updatedAtUtc"), FDateTime::UtcNow().ToIso8601());
	RootObject->SetStringField(TEXT("cleanupPolicy"), TEXT("Restore source actors only. Generated actors and assets are intentionally kept."));

	FString JsonText;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonText);
	if (!FJsonSerializer::Serialize(RootObject, Writer))
	{
		OutError = TEXT("Could not serialize EnvBatch status JSON.");
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonText, *Paths.StatusFilePath))
	{
		OutError = FString::Printf(TEXT("Could not save %s."), *Paths.StatusFilePath);
		return false;
	}

	OutError.Reset();
	return true;
}

bool FDevKitLevelBatchService::RestoreSourceActorsForLoadedWorld(const FString& LevelName, int32& OutRestoredActorCount, FString& OutMessage)
{
	return SetSourceActorsHiddenForLoadedWorld(LevelName, false, OutRestoredActorCount, OutMessage);
}

bool FDevKitLevelBatchService::HideSourceActorsForLoadedWorld(const FString& LevelName, int32& OutHiddenActorCount, FString& OutMessage)
{
	return SetSourceActorsHiddenForLoadedWorld(LevelName, true, OutHiddenActorCount, OutMessage);
}

FDevKitLevelBatchCleanupPolicy FDevKitLevelBatchService::GetCleanupPolicy()
{
	return FDevKitLevelBatchCleanupPolicy();
}

FString FDevKitLevelBatchService::StatusToToken(EDevKitLevelBatchReviewStatus Status)
{
	switch (Status)
	{
	case EDevKitLevelBatchReviewStatus::PendingReview:
		return TEXT("PendingReview");
	case EDevKitLevelBatchReviewStatus::ArtReviewed:
		return TEXT("ArtReviewed");
	case EDevKitLevelBatchReviewStatus::PerformanceApproved:
		return TEXT("PerformanceApproved");
	default:
		return TEXT("NotBatched");
	}
}

FText FDevKitLevelBatchService::StatusToDisplayText(EDevKitLevelBatchReviewStatus Status)
{
	switch (Status)
	{
	case EDevKitLevelBatchReviewStatus::PendingReview:
		return LOCTEXT("PendingReview", "待审查");
	case EDevKitLevelBatchReviewStatus::ArtReviewed:
		return LOCTEXT("ArtReviewed", "已审查");
	case EDevKitLevelBatchReviewStatus::PerformanceApproved:
		return LOCTEXT("PerformanceApproved", "已通过");
	default:
		return LOCTEXT("NotBatched", "未合批");
	}
}

EDevKitLevelBatchReviewStatus FDevKitLevelBatchService::DetermineReviewStatus(
	const FDevKitLevelBatchPaths& Paths,
	bool bHasBatchedMap,
	int32 GeneratedAssetCount,
	bool& bOutHasStatusFile)
{
	bOutHasStatusFile = IFileManager::Get().FileExists(*Paths.StatusFilePath);
	if (bOutHasStatusFile)
	{
		FString StatusToken;
		if (ReadStatusToken(Paths.StatusFilePath, StatusToken))
		{
			return StatusFromToken(StatusToken);
		}
	}

	return (bHasBatchedMap || GeneratedAssetCount > 0)
		? EDevKitLevelBatchReviewStatus::PendingReview
		: EDevKitLevelBatchReviewStatus::NotBatched;
}

bool FDevKitLevelBatchService::SetSourceActorsHiddenForLoadedWorld(
	const FString& LevelName,
	bool bHidden,
	int32& OutChangedActorCount,
	FString& OutMessage)
{
	OutChangedActorCount = 0;
	if (!GEditor)
	{
		OutMessage = TEXT("Editor is not available.");
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		OutMessage = TEXT("No loaded editor world.");
		return false;
	}

	int32 GeneratedActorCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		if (ActorHasGeneratedBatchTag(Actor, LevelName))
		{
			++GeneratedActorCount;
			continue;
		}

		if (!ActorHasSourceTagForLevel(Actor, LevelName))
		{
			continue;
		}

		Actor->Modify();
		Actor->bHiddenEd = bHidden;
		Actor->SetIsTemporarilyHiddenInEditor(bHidden);
		Actor->SetActorHiddenInGame(bHidden);
		Actor->MarkPackageDirty();
		++OutChangedActorCount;
	}

	OutMessage = bHidden
		? FString::Printf(TEXT("Hidden %d source actors for %s. Generated actors were not touched."), OutChangedActorCount, *LevelName)
		: FString::Printf(TEXT("Restored %d source actors for %s. Kept %d generated actors/assets."), OutChangedActorCount, *LevelName, GeneratedActorCount);
	return true;
}

#undef LOCTEXT_NAMESPACE
