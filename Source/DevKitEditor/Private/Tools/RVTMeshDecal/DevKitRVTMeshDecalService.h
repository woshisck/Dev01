#pragma once

#include "CoreMinimal.h"
#include "RVT/DevKitRVTSurfaceAsset.h"

class AInstancedFoliageActor;
class ADevKitRVTSurfaceInstanceActor;
class FLevelEditorViewportClient;
class UFoliageType_InstancedStaticMesh;
class UMaterialInterface;
class URuntimeVirtualTexture;
class UWorld;

enum class EDevKitRVTSurfaceItemMode : uint8
{
	Decal,
	VisibleGroundObject
};

struct FDevKitRVTMeshDecalRequest
{
	FString FoliageTypeFolder;
	FString MeshObjectPath;
	FString MaterialObjectPath;
	FString SurfaceRuntimeVirtualTextureObjectPath;
	FString HeightRuntimeVirtualTextureObjectPath;
	EDevKitRVTSurfaceItemMode ItemMode = EDevKitRVTSurfaceItemMode::Decal;
	int32 TranslucencySortPriority = 0;
	float Density = 100.0f;
	float Radius = 0.0f;
	float MinScale = 1.0f;
	float MaxScale = 1.0f;
	float ZOffset = 0.5f;
	bool bAlignToNormal = true;
	bool bRandomYaw = true;
	bool bBindWorldHeight = true;
	bool bPlaceInCurrentLevel = true;
	FString FoliageTypeNameOverride;
};

struct FDevKitRVTMeshDecalPaths
{
	FString FoliageTypeFolder;
	FString FoliageTypeName;
	FString FoliageTypePackage;
	FString FoliageTypeObjectPath;
};

struct FDevKitRVTMeshDecalResult
{
	bool bSuccess = false;
	bool bCreatedNewAsset = false;
	bool bMaterialWritesSurfaceRVT = false;
	bool bMaterialWritesWorldHeightRVT = false;
	FText Message;
	FDevKitRVTMeshDecalPaths Paths;
	TWeakObjectPtr<UFoliageType_InstancedStaticMesh> FoliageType;
};

struct FDevKitRVTAutoBindingResult
{
	bool bSurfaceResolved = false;
	bool bHeightResolved = false;
	FString SurfaceObjectPath;
	FString HeightObjectPath;
	FText Message;
};

struct FDevKitRVTPlacementResult
{
	bool bSuccess = false;
	FText Message;
	TWeakObjectPtr<AInstancedFoliageActor> InstancedFoliageActor;
};

/** Authoring request for one reusable, final RVT surface-library asset. */
struct FDevKitRVTSurfaceAssetRequest
{
	FString AssetFolder;
	FString AssetName;
	/** The exact selected asset allowed to be updated; null means this is a new-asset request. */
	TWeakObjectPtr<UDevKitRVTSurfaceAsset> ExpectedExistingAsset;
	FText DisplayName;
	FText Description;
	EDevKitRVTSurfaceAssetType AssetType = EDevKitRVTSurfaceAssetType::PlaneDecal;
	EDevKitRVTSurfaceGeometryPolicy GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault;
	FString MeshObjectPath;
	FString MaterialObjectPath;
	int32 Priority = 0;
	FVector DefaultScale = FVector::OneVector;
	float ZOffset = 0.5f;
	bool bAlignToNormal = true;
	bool bRandomYaw = true;
	bool bBindWorldHeight = false;
	bool bEnableCollision = false;
	bool bCastShadow = false;
};

struct FDevKitRVTSurfaceAssetResult
{
	bool bSuccess = false;
	bool bCreatedNewAsset = false;
	FText Message;
	FString ObjectPath;
	TWeakObjectPtr<UDevKitRVTSurfaceAsset> Asset;
};

struct FDevKitRVTSurfaceControllerResult
{
	bool bSuccess = false;
	bool bCreatedNewActor = false;
	int32 InstanceIndex = INDEX_NONE;
	FText Message;
	TWeakObjectPtr<ADevKitRVTSurfaceInstanceActor> Actor;
};

class FDevKitRVTMeshDecalService
{
public:
	static FString InferDefaultSurfaceAssetFolderFromWorldPackage(const FString& WorldPackagePath);
	static FString BuildDefaultSurfaceAssetName(
		const FString& MeshObjectPath,
		const FString& MaterialObjectPath,
		EDevKitRVTSurfaceAssetType AssetType,
		int32 Priority);
	static FDevKitRVTSurfaceAssetResult CreateOrUpdateSurfaceAsset(
		const FDevKitRVTSurfaceAssetRequest& Request);
	static FDevKitRVTSurfaceControllerResult FindOrCreateSurfaceInstanceActor(
		UWorld* World,
		UDevKitRVTSurfaceAsset* SurfaceAsset,
		bool bPlaceInCurrentLevel = true);
	static FDevKitRVTSurfaceControllerResult PlaceSurfaceAssetInstanceAtViewportCursor(
		FLevelEditorViewportClient* ViewportClient,
		UDevKitRVTSurfaceAsset* SurfaceAsset,
		bool bPlaceInCurrentLevel = true);
	static FDevKitRVTSurfaceControllerResult PlaceSurfaceAssetInstanceAtViewportCenter(
		FLevelEditorViewportClient* ViewportClient,
		UDevKitRVTSurfaceAsset* SurfaceAsset,
		bool bPlaceInCurrentLevel = true);
	static FDevKitRVTSurfaceControllerResult SelectSurfaceInstanceActor(
		UWorld* World,
		UDevKitRVTSurfaceAsset* SurfaceAsset,
		bool bPlaceInCurrentLevel = true);

	// Legacy FoliageType compatibility API. New library/UI code should prefer the surface-asset API above.
	static FString InferDefaultFoliageTypeFolderFromWorldPackage(const FString& WorldPackagePath);
	static FString BuildDefaultFoliageTypeName(
		const FString& MaterialObjectPath,
		int32 TranslucencySortPriority,
		EDevKitRVTSurfaceItemMode ItemMode = EDevKitRVTSurfaceItemMode::Decal,
		const FString& MeshObjectPath = FString());
	static FString GetDefaultPlaneMeshObjectPath();
	static TOptional<FDevKitRVTMeshDecalPaths> BuildPaths(const FDevKitRVTMeshDecalRequest& Request, FText& OutError);
	static FDevKitRVTMeshDecalResult CreateOrUpdateFoliageType(const FDevKitRVTMeshDecalRequest& Request);

	static FDevKitRVTAutoBindingResult ResolveRuntimeVirtualTexturesForWorld(UWorld* World);
	static FDevKitRVTPlacementResult AddFoliageTypeToCurrentActor(
		UWorld* World,
		UFoliageType_InstancedStaticMesh* FoliageType,
		bool bPlaceInCurrentLevel);
	static FDevKitRVTPlacementResult PlaceSingleInstanceAtViewportCursor(
		FLevelEditorViewportClient* ViewportClient,
		UFoliageType_InstancedStaticMesh* FoliageType,
		bool bPlaceInCurrentLevel);
	static bool OpenFoliageMode(bool bSingleInstanceMode, bool bPlaceInCurrentLevel, FText& OutMessage);
	static bool MaterialWritesSurfaceToRuntimeVirtualTexture(const UMaterialInterface* Material);
	static bool MaterialWritesWorldHeightToRuntimeVirtualTexture(const UMaterialInterface* Material);
	static bool IsSurfaceRuntimeVirtualTexture(const URuntimeVirtualTexture* RuntimeVirtualTexture);
	static bool IsWorldHeightRuntimeVirtualTexture(const URuntimeVirtualTexture* RuntimeVirtualTexture);

private:
	static FDevKitRVTSurfaceControllerResult PlaceSurfaceAssetInstanceFromRay(
		FLevelEditorViewportClient* ViewportClient,
		UDevKitRVTSurfaceAsset* SurfaceAsset,
		bool bPlaceInCurrentLevel,
		const FVector& RayOrigin,
		const FVector& RayDirection);
	static bool IsValidNameToken(const FString& Token);
	static FString NormalizeFolder(FString Folder);
	static FString NormalizeObjectPath(FString ObjectPath);
	static FString SanitizeNameToken(FString Token);
};
