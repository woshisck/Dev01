#include "Tools/RVTMeshDecal/DevKitRVTMeshDecalService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "FoliageType.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "UObject/SavePackage.h"
#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureEnum.h"

#define LOCTEXT_NAMESPACE "DevKitRVTMeshDecalService"

namespace
{
	const FString LevelAssetFolderName = TEXT("LevelAsset");
	const FString BakeInfoFolderName = TEXT("BakeInfo");
	const FString RVTDecalFoliageFolderName = TEXT("RVTDecalFoliage");
	const TCHAR* DefaultPlaneMeshObjectPath = TEXT("/Engine/BasicShapes/Plane.Plane");

	bool EnsureContentFolder(const FString& LongPackagePath)
	{
		const FString FolderFilename = FPackageName::LongPackageNameToFilename(LongPackagePath);
		return IFileManager::Get().MakeDirectory(*FolderFilename, true);
	}
}

FString FDevKitRVTMeshDecalService::InferDefaultFoliageTypeFolderFromWorldPackage(const FString& WorldPackagePath)
{
	FString Normalized = NormalizeFolder(WorldPackagePath);
	const FString LevelAssetToken = FString::Printf(TEXT("/%s/"), *LevelAssetFolderName);
	const int32 LevelAssetIndex = Normalized.Find(LevelAssetToken, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (LevelAssetIndex != INDEX_NONE)
	{
		return Normalized.Left(LevelAssetIndex) / BakeInfoFolderName / RVTDecalFoliageFolderName;
	}

	int32 LastSlashIndex = INDEX_NONE;
	if (Normalized.FindLastChar(TEXT('/'), LastSlashIndex))
	{
		return Normalized.Left(LastSlashIndex) / BakeInfoFolderName / RVTDecalFoliageFolderName;
	}

	return FString(TEXT("/Game")) / BakeInfoFolderName / RVTDecalFoliageFolderName;
}

FString FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(const FString& MaterialObjectPath, int32 TranslucencySortPriority)
{
	FString MaterialName = FPackageName::ObjectPathToObjectName(NormalizeObjectPath(MaterialObjectPath));
	if (MaterialName.IsEmpty())
	{
		MaterialName = FPackageName::GetLongPackageAssetName(MaterialObjectPath);
	}
	MaterialName = SanitizeNameToken(MaterialName);
	if (MaterialName.IsEmpty())
	{
		MaterialName = TEXT("Material");
	}

	const FString PriorityToken = TranslucencySortPriority < 0
		? FString::Printf(TEXT("PM%d"), FMath::Abs(TranslucencySortPriority))
		: FString::Printf(TEXT("P%d"), TranslucencySortPriority);

	return FString::Printf(TEXT("FT_RVTDecal_%s_%s"), *MaterialName, *PriorityToken);
}

FString FDevKitRVTMeshDecalService::GetDefaultPlaneMeshObjectPath()
{
	return DefaultPlaneMeshObjectPath;
}

TOptional<FDevKitRVTMeshDecalPaths> FDevKitRVTMeshDecalService::BuildPaths(const FDevKitRVTMeshDecalRequest& Request, FText& OutError)
{
	FDevKitRVTMeshDecalPaths Paths;
	Paths.FoliageTypeFolder = NormalizeFolder(Request.FoliageTypeFolder);
	Paths.FoliageTypeName = Request.FoliageTypeNameOverride;
	Paths.FoliageTypeName.TrimStartAndEndInline();
	if (Paths.FoliageTypeName.IsEmpty())
	{
		Paths.FoliageTypeName = BuildDefaultFoliageTypeName(Request.MaterialObjectPath, Request.TranslucencySortPriority);
	}

	if (!FPackageName::IsValidLongPackageName(Paths.FoliageTypeFolder, true))
	{
		OutError = LOCTEXT("InvalidFoliageTypeFolder", "FoliageType 保存目录必须是有效内容路径，例如 /Game/Art/Map/Map_Data/LevelName/BakeInfo/RVTDecalFoliage。");
		return TOptional<FDevKitRVTMeshDecalPaths>();
	}

	if (!IsValidNameToken(Paths.FoliageTypeName))
	{
		OutError = LOCTEXT("InvalidFoliageTypeName", "FoliageType 名称只能使用字母、数字和下划线。");
		return TOptional<FDevKitRVTMeshDecalPaths>();
	}

	if (Request.MinScale <= 0.0f || Request.MaxScale <= 0.0f || Request.MinScale > Request.MaxScale)
	{
		OutError = LOCTEXT("InvalidScaleRange", "尺寸范围必须大于 0，且最小值不能大于最大值。");
		return TOptional<FDevKitRVTMeshDecalPaths>();
	}

	Paths.FoliageTypePackage = Paths.FoliageTypeFolder / Paths.FoliageTypeName;
	Paths.FoliageTypeObjectPath = FString::Printf(TEXT("%s.%s"), *Paths.FoliageTypePackage, *Paths.FoliageTypeName);
	OutError = FText::GetEmpty();
	return Paths;
}

FDevKitRVTMeshDecalResult FDevKitRVTMeshDecalService::CreateOrUpdateFoliageType(const FDevKitRVTMeshDecalRequest& Request)
{
	FText Error;
	TOptional<FDevKitRVTMeshDecalPaths> Paths = BuildPaths(Request, Error);
	if (!Paths.IsSet())
	{
		return FDevKitRVTMeshDecalResult{false, false, Error, FDevKitRVTMeshDecalPaths()};
	}

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, *NormalizeObjectPath(Request.PlaneMeshObjectPath));
	if (!PlaneMesh)
	{
		return FDevKitRVTMeshDecalResult{
			false,
			false,
			FText::Format(LOCTEXT("LoadPlaneFailed", "无法加载 Plane Mesh：{0}"), FText::FromString(Request.PlaneMeshObjectPath)),
			Paths.GetValue()
		};
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *NormalizeObjectPath(Request.MaterialObjectPath));
	if (!Material)
	{
		return FDevKitRVTMeshDecalResult{
			false,
			false,
			FText::Format(LOCTEXT("LoadMaterialFailed", "无法加载贴花材质：{0}"), FText::FromString(Request.MaterialObjectPath)),
			Paths.GetValue()
		};
	}

	URuntimeVirtualTexture* RuntimeVirtualTexture = LoadObject<URuntimeVirtualTexture>(nullptr, *NormalizeObjectPath(Request.RuntimeVirtualTextureObjectPath));
	if (!RuntimeVirtualTexture)
	{
		return FDevKitRVTMeshDecalResult{
			false,
			false,
			FText::Format(LOCTEXT("LoadRVTFailed", "无法加载目标 RVT：{0}"), FText::FromString(Request.RuntimeVirtualTextureObjectPath)),
			Paths.GetValue()
		};
	}

	if (!EnsureContentFolder(Paths->FoliageTypeFolder))
	{
		return FDevKitRVTMeshDecalResult{
			false,
			false,
			FText::Format(LOCTEXT("CreateFolderFailed", "创建 FoliageType 保存目录失败：{0}"), FText::FromString(Paths->FoliageTypeFolder)),
			Paths.GetValue()
		};
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateRVTMeshDecalFoliageTypeTransaction", "创建 RVT 网格贴花 FoliageType"));

	bool bCreatedNewAsset = false;
	UFoliageType_InstancedStaticMesh* FoliageType = LoadObject<UFoliageType_InstancedStaticMesh>(nullptr, *Paths->FoliageTypeObjectPath);
	if (!FoliageType)
	{
		UPackage* Package = CreatePackage(*Paths->FoliageTypePackage);
		if (!Package)
		{
			return FDevKitRVTMeshDecalResult{
				false,
				false,
				FText::Format(LOCTEXT("CreatePackageFailed", "创建 FoliageType 包失败：{0}"), FText::FromString(Paths->FoliageTypePackage)),
				Paths.GetValue()
			};
		}

		FoliageType = NewObject<UFoliageType_InstancedStaticMesh>(
			Package,
			UFoliageType_InstancedStaticMesh::StaticClass(),
			*Paths->FoliageTypeName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!FoliageType)
		{
			return FDevKitRVTMeshDecalResult{false, false, LOCTEXT("CreateFoliageTypeFailed", "创建 FoliageType 资产失败。"), Paths.GetValue()};
		}

		FAssetRegistryModule::AssetCreated(FoliageType);
		bCreatedNewAsset = true;
	}

	FoliageType->Modify();
	FoliageType->SetStaticMesh(PlaneMesh);
	FoliageType->OverrideMaterials.Reset();
	FoliageType->OverrideMaterials.Add(Material);
	FoliageType->RuntimeVirtualTextures.Reset();
	FoliageType->RuntimeVirtualTextures.Add(RuntimeVirtualTexture);
	FoliageType->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
	FoliageType->VirtualTextureCullMips = 0;
	FoliageType->TranslucencySortPriority = Request.TranslucencySortPriority;
	FoliageType->Scaling = EFoliageScaling::Uniform;
	FoliageType->ScaleX = FFloatInterval(Request.MinScale, Request.MaxScale);
	FoliageType->ScaleY = FFloatInterval(Request.MinScale, Request.MaxScale);
	FoliageType->ScaleZ = FFloatInterval(1.0f, 1.0f);
	FoliageType->AlignToNormal = Request.bAlignToNormal;
	FoliageType->AlignMaxAngle = 90.0f;
	FoliageType->RandomYaw = Request.bRandomYaw;
	FoliageType->RandomPitchAngle = 0.0f;
	FoliageType->ZOffset = FFloatInterval(0.5f, 0.5f);
	FoliageType->Radius = 0.0f;
	FoliageType->CollisionWithWorld = false;
	FoliageType->CastShadow = false;
	FoliageType->bCastDynamicShadow = false;
	FoliageType->bCastStaticShadow = false;
	FoliageType->bCastContactShadow = false;
	FoliageType->bReceivesDecals = false;
	FoliageType->bEnableDensityScaling = false;
	FoliageType->bIncludeInHLOD = false;
	FoliageType->Mobility = EComponentMobility::Static;
	FoliageType->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave{FoliageType->GetPackage()};
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	return FDevKitRVTMeshDecalResult{
		true,
		bCreatedNewAsset,
		FText::Format(
			bCreatedNewAsset ? LOCTEXT("CreateSuccess", "已创建 RVT 网格贴花 FoliageType：{0}") : LOCTEXT("UpdateSuccess", "已更新 RVT 网格贴花 FoliageType：{0}"),
			FText::FromString(Paths->FoliageTypeObjectPath)),
		Paths.GetValue(),
		FoliageType
	};
}

bool FDevKitRVTMeshDecalService::IsValidNameToken(const FString& Token)
{
	if (Token.IsEmpty())
	{
		return false;
	}

	for (const TCHAR Character : Token)
	{
		if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
		{
			return false;
		}
	}
	return true;
}

FString FDevKitRVTMeshDecalService::NormalizeFolder(FString Folder)
{
	Folder.TrimStartAndEndInline();
	while (Folder.EndsWith(TEXT("/")))
	{
		Folder.LeftChopInline(1);
	}
	return Folder;
}

FString FDevKitRVTMeshDecalService::NormalizeObjectPath(FString ObjectPath)
{
	ObjectPath.TrimStartAndEndInline();
	return ObjectPath;
}

FString FDevKitRVTMeshDecalService::SanitizeNameToken(FString Token)
{
	Token.TrimStartAndEndInline();
	for (TCHAR& Character : Token)
	{
		if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
		{
			Character = TEXT('_');
		}
	}
	return Token;
}

#undef LOCTEXT_NAMESPACE
