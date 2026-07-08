#pragma once

#include "CoreMinimal.h"

class AActor;
class ARuntimeVirtualTextureVolume;
class UPrimitiveComponent;
class URuntimeVirtualTexture;
class UWorld;

struct FDevKitLevelRVTRequest
{
	FString BakeInfoFolder;
	FString RuntimeVirtualTextureName;
};

struct FDevKitLevelRVTPaths
{
	FString BakeInfoFolder;
	FString RuntimeVirtualTextureName;
	FString RuntimeVirtualTexturePackage;
	FString VolumeActorName;
};

struct FDevKitLevelRVTResult
{
	bool bSuccess = false;
	FText Message;
	FDevKitLevelRVTPaths Paths;
	int32 ActorCount = 0;
	int32 PrimitiveComponentCount = 0;
	TWeakObjectPtr<URuntimeVirtualTexture> RuntimeVirtualTexture;
	TWeakObjectPtr<ARuntimeVirtualTextureVolume> Volume;
};

class FDevKitLevelRVTService
{
public:
	static FString InferBakeInfoFolderFromWorldPackage(const FString& WorldPackagePath);
	static FString BuildDefaultGroundRVTNameFromWorldPackage(const FString& WorldPackagePath);
	static TOptional<FDevKitLevelRVTPaths> BuildPaths(const FDevKitLevelRVTRequest& Request, FText& OutError);

	static TArray<AActor*> GetSelectedActors();
	static FDevKitLevelRVTResult CreateGroundRVTForSelection(UWorld* World, const FDevKitLevelRVTRequest& Request);

private:
	static bool IsValidNameToken(const FString& Token);
	static FString NormalizeRootFolder(FString RootFolder);
	static TArray<UPrimitiveComponent*> CollectPrimitiveComponents(const TArray<AActor*>& Actors);
	static URuntimeVirtualTexture* CreateOrLoadRuntimeVirtualTexture(const FDevKitLevelRVTPaths& Paths, FText& OutError);
};
