#pragma once

#include "CoreMinimal.h"

class UFoliageType_InstancedStaticMesh;

struct FDevKitRVTMeshDecalRequest
{
	FString FoliageTypeFolder;
	FString PlaneMeshObjectPath;
	FString MaterialObjectPath;
	FString RuntimeVirtualTextureObjectPath;
	int32 TranslucencySortPriority = 0;
	float MinScale = 1.0f;
	float MaxScale = 1.0f;
	bool bAlignToNormal = true;
	bool bRandomYaw = true;
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
	FText Message;
	FDevKitRVTMeshDecalPaths Paths;
	TWeakObjectPtr<UFoliageType_InstancedStaticMesh> FoliageType;
};

class FDevKitRVTMeshDecalService
{
public:
	static FString InferDefaultFoliageTypeFolderFromWorldPackage(const FString& WorldPackagePath);
	static FString BuildDefaultFoliageTypeName(const FString& MaterialObjectPath, int32 TranslucencySortPriority);
	static FString GetDefaultPlaneMeshObjectPath();
	static TOptional<FDevKitRVTMeshDecalPaths> BuildPaths(const FDevKitRVTMeshDecalRequest& Request, FText& OutError);
	static FDevKitRVTMeshDecalResult CreateOrUpdateFoliageType(const FDevKitRVTMeshDecalRequest& Request);

private:
	static bool IsValidNameToken(const FString& Token);
	static FString NormalizeFolder(FString Folder);
	static FString NormalizeObjectPath(FString ObjectPath);
	static FString SanitizeNameToken(FString Token);
};
