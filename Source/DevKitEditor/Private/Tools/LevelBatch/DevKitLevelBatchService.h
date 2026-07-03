#pragma once

#include "CoreMinimal.h"

enum class EDevKitLevelBatchReviewStatus : uint8
{
	NotBatched,
	PendingReview,
	ArtReviewed,
	PerformanceApproved
};

struct FDevKitLevelBatchPaths
{
	FString LevelName;
	FString LevelFolder;
	FString LevelAssetFolder;
	FString LevelMaterialFolder;
	FString BatchedAssetFolder;
	FString BakeInfoFolder;
	FString PersistentMapPackage;
	FString BatchedMapPackage;
	FString StatusFilePath;
};

struct FDevKitLevelBatchStatus
{
	FDevKitLevelBatchPaths Paths;
	EDevKitLevelBatchReviewStatus ReviewStatus = EDevKitLevelBatchReviewStatus::NotBatched;
	bool bHasLevelAssetFolder = false;
	bool bHasBatchedAssetFolder = false;
	bool bHasPersistentMap = false;
	bool bHasBatchedMap = false;
	bool bHasStatusFile = false;
	int32 GeneratedAssetCount = 0;
};

struct FDevKitLevelBatchCommand
{
	FString EditorCmdPath;
	FString Arguments;
	FString PowerShellCommand;
	FString OutputRoot;
	FString RequireTagPrefix;
};

struct FDevKitLevelBatchCleanupPolicy
{
	bool bRestoreSourceActors = true;
	bool bDeleteGeneratedActors = false;
	bool bDeleteGeneratedAssets = false;
};

class FDevKitLevelBatchService
{
public:
	static FString GetDefaultMapDataRoot();
	static FDevKitLevelBatchPaths BuildPaths(const FString& LevelFolder);
	static TArray<FDevKitLevelBatchStatus> ScanLevelFolders(const FString& RootPackagePath = GetDefaultMapDataRoot());
	static TOptional<FDevKitLevelBatchStatus> FindLevelForMapPackage(const FString& MapPackagePath, const FString& RootPackagePath = GetDefaultMapDataRoot());
	static FDevKitLevelBatchCommand BuildPartialApplyCommand(const FDevKitLevelBatchPaths& Paths, const FString& TierName);
	static bool LaunchPartialApplyCommand(const FDevKitLevelBatchPaths& Paths, const FString& TierName, FString& OutMessage);
	static bool WriteReviewStatus(const FDevKitLevelBatchPaths& Paths, EDevKitLevelBatchReviewStatus Status, FString& OutError);
	static bool RestoreSourceActorsForLoadedWorld(const FString& LevelName, int32& OutRestoredActorCount, FString& OutMessage);
	static bool HideSourceActorsForLoadedWorld(const FString& LevelName, int32& OutHiddenActorCount, FString& OutMessage);
	static FDevKitLevelBatchCleanupPolicy GetCleanupPolicy();
	static FString StatusToToken(EDevKitLevelBatchReviewStatus Status);
	static FText StatusToDisplayText(EDevKitLevelBatchReviewStatus Status);

private:
	static EDevKitLevelBatchReviewStatus DetermineReviewStatus(const FDevKitLevelBatchPaths& Paths, bool bHasBatchedMap, int32 GeneratedAssetCount, bool& bOutHasStatusFile);
	static bool SetSourceActorsHiddenForLoadedWorld(const FString& LevelName, bool bHidden, int32& OutChangedActorCount, FString& OutMessage);
};
