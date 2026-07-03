#pragma once

#include "CoreMinimal.h"

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

private:
	static bool IsValidNameToken(const FString& Token);
	static FString NormalizeRootFolder(FString RootFolder);
};
