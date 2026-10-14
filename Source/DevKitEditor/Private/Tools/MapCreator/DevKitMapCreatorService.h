#pragma once

#include "CoreMinimal.h"

class UWorld;

struct FDevKitMapCreatorRequest
{
	FString RootFolder;
	FString DungeonLayer;
	FString LevelType;
	FString NameSuffix;
};

struct FDevKitMapCreatorPaths
{
	FString BaseName;
	FString TargetFolder;
	FString LevelAssetFolder;
	FString LevelMaterialFolder;
	FString BatchedAssetFolder;
	FString BakeInfoFolder;
	FString PersistentMap;
	FString MapDefinition;
	TArray<FString> Sublevels;
};

struct FDevKitMapCreatorResult
{
	bool bSuccess = false;
	FText Message;
	FDevKitMapCreatorPaths Paths;
};

/** Creates one conventional streaming sublevel for an already saved editor map. */
struct FDevKitSublevelRequest
{
	FString TargetFolder;
	FString Suffix;
	FString DungeonLayer;
};

struct FDevKitSublevelResult
{
	bool bSuccess = false;
	FText Message;
	FString PackagePath;
	// A failed parent save may leave the new file and an undoable, unsaved reference.
	bool bAssetCreated = false;
	bool bAttached = false;
	bool bParentSaved = false;
};

class FDevKitMapCreatorService
{
public:
	static const TArray<FString>& GetDungeonLayers();
	static const TArray<FString>& GetLevelTypes();
	static const TArray<FString>& GetSublevelSuffixes();
	static FString GetPersistentTemplateMapPath();
	static FString GetSublevelTemplateMapPath(const FString& SublevelSuffix, const FString& DungeonLayer);
	static FString GetDefaultRootFolder();

	static TOptional<FDevKitMapCreatorPaths> BuildPaths(const FDevKitMapCreatorRequest& Request, FText& OutError);
	static FDevKitMapCreatorResult CreateLevelStack(const FDevKitMapCreatorRequest& Request);
	/** Prompt only for the current editor world's map/external-actor packages. */
	static bool SaveCurrentEditorWorld();

	/** Pure path validation. Existing packages are checked separately at execution time. */
	static TOptional<FString> BuildSublevelPath(const FString& PersistentPackagePath,
		const FDevKitSublevelRequest& Request, FText& OutError);
	static FDevKitSublevelResult CreateAndAttachSublevel(UWorld* PersistentWorld,
		const FDevKitSublevelRequest& Request);
	static FDevKitSublevelResult AttachExistingSublevel(UWorld* PersistentWorld,
		const FString& PackagePath);

private:
	static bool IsValidNameToken(const FString& Token);
	static FString NormalizeRootFolder(FString RootFolder);
};
