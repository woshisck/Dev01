#pragma once

#include "CoreMinimal.h"

class AActor;
class ARuntimeVirtualTextureVolume;
class ULevel;
class UPrimitiveComponent;
class URuntimeVirtualTexture;
class UWorld;

enum class EDevKitLevelRVTLayout : uint8
{
	BaseColorNormalRoughness,
	Mask4,
	BaseColor,
	BaseColorNormalSpecular,
	YCoCgBaseColorNormalSpecular,
	YCoCgBaseColorNormalSpecularMask,
	WorldHeight,
	Displacement
};

struct FDevKitLevelRVTRequest
{
	FString BakeInfoFolder;
	FString RuntimeVirtualTextureName;
	FString GroundBatchSourceTag;
	TArray<EDevKitLevelRVTLayout> Layouts;
};

struct FDevKitLevelRVTPaths
{
	FString BakeInfoFolder;
	FString RuntimeVirtualTextureName;
	FString RuntimeVirtualTexturePackage;
	FString DataBakeLevelPackage;
	FString VolumeActorName;
	EDevKitLevelRVTLayout Layout = EDevKitLevelRVTLayout::BaseColorNormalRoughness;
};

struct FDevKitLevelRVTResult
{
	bool bSuccess = false;
	FText Message;
	FDevKitLevelRVTPaths Paths;
	int32 ActorCount = 0;
	int32 PrimitiveComponentCount = 0;
	int32 RuntimeVirtualTextureCount = 0;
	TWeakObjectPtr<URuntimeVirtualTexture> RuntimeVirtualTexture;
	TWeakObjectPtr<ARuntimeVirtualTextureVolume> Volume;
	FString GroundBatchSourceTag;
};

struct FDevKitLevelRVTBatchStats
{
	int32 ActorCount = 0;
	int32 PrimitiveComponentCount = 0;
	int32 BoundComponentCount = 0;
	int32 RuntimeVirtualTextureReferenceCount = 0;
};

struct FDevKitLevelRVTAssetState
{
	int32 RequestedLayoutCount = 0;
	int32 ExistingAssetCount = 0;
	int32 ExistingVolumeCount = 0;
	bool bDataBakeLevelLoaded = false;

	bool IsReady() const
	{
		return RequestedLayoutCount > 0
			&& ExistingAssetCount == RequestedLayoutCount
			&& ExistingVolumeCount == RequestedLayoutCount;
	}
};

class FDevKitLevelRVTService
{
public:
	static FString InferBakeInfoFolderFromWorldPackage(const FString& WorldPackagePath);
	static FString InferDataBakeLevelPackageFromWorldPackage(const FString& WorldPackagePath);
	static FString BuildDefaultGroundRVTNameFromWorldPackage(const FString& WorldPackagePath);
	static FString GetLayoutDisplayName(EDevKitLevelRVTLayout Layout);
	static FString BuildAssetNameForLayout(const FString& BaseName, EDevKitLevelRVTLayout Layout);
	static TOptional<FDevKitLevelRVTPaths> BuildPaths(const FDevKitLevelRVTRequest& Request, FText& OutError);
	static TOptional<FDevKitLevelRVTPaths> BuildPaths(const FDevKitLevelRVTRequest& Request, EDevKitLevelRVTLayout Layout, FText& OutError);

	static TArray<AActor*> GetSelectedActors();
	static TArray<AActor*> GetActorsWithGroundBatchSourceTag(UWorld* World, const FString& SourceTag);
	static FDevKitLevelRVTBatchStats GetGroundBatchStats(UWorld* World, const FString& SourceTag);
	static FDevKitLevelRVTAssetState GetRVTAssetState(UWorld* World, const FDevKitLevelRVTRequest& Request);
	static FDevKitLevelRVTResult AssignGroundBatchSourceTagToSelection(UWorld* World, const FString& SourceTag);
	static FDevKitLevelRVTResult RemoveGroundBatchSourceTagFromSelection(UWorld* World, const FString& SourceTag);
	static FDevKitLevelRVTResult CreateGroundRVTAssetsAndVolumes(UWorld* World, const FDevKitLevelRVTRequest& Request);
	static FDevKitLevelRVTResult AddGroundRVTBindings(UWorld* World, const FDevKitLevelRVTRequest& Request);
	static FDevKitLevelRVTResult RemoveGroundRVTBindings(UWorld* World, const FDevKitLevelRVTRequest& Request);
	static FDevKitLevelRVTResult ClearGroundRVTBindings(UWorld* World, const FDevKitLevelRVTRequest& Request);

private:
	static bool IsValidNameToken(const FString& Token);
	static FString NormalizeRootFolder(FString RootFolder);
	static TArray<UPrimitiveComponent*> CollectPrimitiveComponents(const TArray<AActor*>& Actors);
	static URuntimeVirtualTexture* CreateOrLoadRuntimeVirtualTexture(const FDevKitLevelRVTPaths& Paths, FText& OutError);
	static URuntimeVirtualTexture* LoadRuntimeVirtualTexture(const FDevKitLevelRVTPaths& Paths);
	static ARuntimeVirtualTextureVolume* FindOrCreateVolume(UWorld* World, ULevel* DataBakeLevel, const FDevKitLevelRVTPaths& Paths, URuntimeVirtualTexture* RuntimeVirtualTexture, const TArray<UPrimitiveComponent*>& BoundsComponents, FText& OutError);
};
