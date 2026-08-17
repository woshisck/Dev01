#include "Tools/RVTMeshDecal/DevKitRVTMeshDecalService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/BrushComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/ModelComponent.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorModes.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "FoliageEditModule.h"
#include "FoliageInstancedStaticMeshComponent.h"
#include "FoliageType.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "Factories/DataAssetFactory.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IAssetTools.h"
#include "InstancedFoliage.h"
#include "InstancedFoliageActor.h"
#include "LevelEditorViewport.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureOutput.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "RVT/DevKitRVTSurfaceInstanceActor.h"
#include "SceneView.h"
#include "Tools/LevelRVT/DevKitLevelRVTService.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureEnum.h"
#include "VT/RuntimeVirtualTextureVolume.h"

#define LOCTEXT_NAMESPACE "DevKitRVTMeshDecalService"

namespace
{
	const FString LevelAssetFolderName = TEXT("LevelAsset");
	const FString BakeInfoFolderName = TEXT("BakeInfo");
	const FString RVTDecalFoliageFolderName = TEXT("RVTDecalFoliage");
	const FString RVTSurfaceLibraryFolderName = TEXT("RVTSurfaceLibrary");
	const TCHAR* DefaultPlaneMeshObjectPath = TEXT(
		"/Game/Art/EnvironmentAsset/Decal/Mesh/SM_DecalPlane_01.SM_DecalPlane_01");

	bool IsEditableEditorWorld(const UWorld* World)
	{
		return GEditor
			&& World
			&& World->WorldType == EWorldType::Editor
			&& GEditor->PlayWorld == nullptr
			&& !GEditor->bIsSimulatingInEditor;
	}

	void SelectOnlyFoliageType(UWorld* World, UFoliageType* SelectedFoliageType)
	{
		if (!World || !SelectedFoliageType)
		{
			return;
		}

		for (TActorIterator<AInstancedFoliageActor> It(World); It; ++It)
		{
			for (const auto& Pair : It->GetFoliageInfos())
			{
				UFoliageType* FoliageType = Pair.Key;
				if (!FoliageType)
				{
					continue;
				}

				const bool bShouldBeSelected = FoliageType == SelectedFoliageType;
				if (FoliageType->IsSelected != bShouldBeSelected)
				{
					FoliageType->Modify(false);
					FoliageType->IsSelected = bShouldBeSelected;
				}
			}
		}
	}

	bool EnsureContentFolder(const FString& LongPackagePath)
	{
		const FString FolderFilename = FPackageName::LongPackageNameToFilename(LongPackagePath);
		return IFileManager::Get().MakeDirectory(*FolderFilename, true);
	}

	FString MakeObjectPath(const FString& Folder, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *Folder, *AssetName, *AssetName);
	}

	FString ToObjectPath(const UObject* Object)
	{
		return Object ? FSoftObjectPath(Object).ToString() : FString();
	}

	bool IsWorldHeightRVT(const URuntimeVirtualTexture* RuntimeVirtualTexture)
	{
		return RuntimeVirtualTexture
			&& RuntimeVirtualTexture->GetMaterialType() == ERuntimeVirtualTextureMaterialType::WorldHeight;
	}

	bool IsSurfaceRVT(const URuntimeVirtualTexture* RuntimeVirtualTexture)
	{
		if (!RuntimeVirtualTexture)
		{
			return false;
		}

		switch (RuntimeVirtualTexture->GetMaterialType())
		{
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular:
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Roughness:
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_YCoCg:
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_Mask_YCoCg:
		case ERuntimeVirtualTextureMaterialType::BaseColor:
			return true;
		default:
			return false;
		}
	}

	bool SupportsGameplayCollision(
		const EDevKitRVTSurfaceAssetType AssetType,
		const EDevKitRVTSurfaceGeometryPolicy GeometryPolicy)
	{
		return AssetType == EDevKitRVTSurfaceAssetType::VisibleObject
			&& (GeometryPolicy == EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible
				|| GeometryPolicy == EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault);
	}

	bool HasActiveRuntimeVirtualTextureVolume(
		UWorld* World,
		const URuntimeVirtualTexture* RuntimeVirtualTexture,
		const bool bRequireHidePrimitives = false)
	{
		if (!World || !RuntimeVirtualTexture)
		{
			return false;
		}

		for (TActorIterator<ARuntimeVirtualTextureVolume> It(World); It; ++It)
		{
			const URuntimeVirtualTextureComponent* Component = It->VirtualTextureComponent;
			if (Component
				&& Component->IsRegistered()
				&& Component->GetVirtualTexture() == RuntimeVirtualTexture)
			{
				if (bRequireHidePrimitives)
				{
					bool bHidePrimitivesInEditor = false;
					bool bHidePrimitivesInGame = false;
					Component->GetHidePrimitiveSettings(
						bHidePrimitivesInEditor,
						bHidePrimitivesInGame);
					if (!bHidePrimitivesInEditor || !bHidePrimitivesInGame)
					{
						continue;
					}
				}
				return true;
			}
		}
		return false;
	}

	int32 GetSurfaceRVTPriority(const URuntimeVirtualTexture* RuntimeVirtualTexture)
	{
		if (!RuntimeVirtualTexture)
		{
			return MAX_int32;
		}

		switch (RuntimeVirtualTexture->GetMaterialType())
		{
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular:
			return 0;
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Roughness:
			return 1;
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_YCoCg:
			return 2;
		case ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_Mask_YCoCg:
			return 3;
		case ERuntimeVirtualTextureMaterialType::BaseColor:
			return 4;
		default:
			return MAX_int32;
		}
	}

	const UMaterialExpressionRuntimeVirtualTextureOutput* FindRuntimeVirtualTextureOutput(
		const UMaterialInterface* Material)
	{
		const UMaterial* BaseMaterial = Material ? Material->GetMaterial() : nullptr;
		if (!BaseMaterial)
		{
			return nullptr;
		}

		for (const TObjectPtr<UMaterialExpression>& Expression : BaseMaterial->GetExpressions())
		{
			if (const UMaterialExpressionRuntimeVirtualTextureOutput* Output =
				Cast<UMaterialExpressionRuntimeVirtualTextureOutput>(Expression.Get()))
			{
				return Output;
			}
		}
		return nullptr;
	}

	bool IsLandscapeCollisionComponent(const UPrimitiveComponent* Component)
	{
		for (const UClass* ComponentClass = Component ? Component->GetClass() : nullptr;
			ComponentClass;
			ComponentClass = ComponentClass->GetSuperClass())
		{
			if (ComponentClass->GetFName() == TEXT("LandscapeHeightfieldCollisionComponent"))
			{
				return true;
			}
		}
		return false;
	}

	void RefreshFoliagePaletteIfOpen()
	{
		if (FModuleManager::Get().IsModuleLoaded(TEXT("FoliageEdit")))
		{
			FModuleManager::GetModuleChecked<IFoliageEditModule>(TEXT("FoliageEdit")).UpdateMeshList();
		}
	}

	void EnsureSMInstanceViewportSelectionEnabled()
	{
		if (IConsoleVariable* SelectionCVar = IConsoleManager::Get().FindConsoleVariable(
			TEXT("TypedElements.EnableViewportSMInstanceSelection")))
		{
			SelectionCVar->Set(1, ECVF_SetByCode);
		}
	}

	ULevel* ResolveSurfaceActorLevel(UWorld* World, const bool bPlaceInCurrentLevel)
	{
		if (!World)
		{
			return nullptr;
		}

		if (bPlaceInCurrentLevel && World->GetCurrentLevel())
		{
			return World->GetCurrentLevel();
		}

		return World->PersistentLevel.Get();
	}

	bool IsSurfacePlacementComponent(const UPrimitiveComponent* Component)
	{
		if (!Component)
		{
			return false;
		}

		if (IsLandscapeCollisionComponent(Component))
		{
			return true;
		}

		if (Component->IsA<UBrushComponent>() || Component->IsA<UModelComponent>())
		{
			return true;
		}

		if (!Component->IsA<UStaticMeshComponent>()
			|| Component->IsA<UFoliageInstancedStaticMeshComponent>())
		{
			return false;
		}

		// Do not stack a new surface item on an existing library controller instance.
		const AActor* Owner = Component->GetOwner();
		return !Owner || !Owner->IsA<ADevKitRVTSurfaceInstanceActor>();
	}

	void SelectOnlySurfaceActor(ADevKitRVTSurfaceInstanceActor* Actor)
	{
		if (!GEditor || !Actor)
		{
			return;
		}

		EnsureSMInstanceViewportSelectionEnabled();
		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(Actor, true, false, true);
		GEditor->NoteSelectionChange();
	}

	FDevKitRVTSurfaceControllerResult FindOrCreateSurfaceInstanceActorInternal(
		UWorld* World,
		UDevKitRVTSurfaceAsset* SurfaceAsset,
		const bool bPlaceInCurrentLevel)
	{
		FDevKitRVTSurfaceControllerResult Result;
		if (!World || !SurfaceAsset)
		{
			Result.Message = LOCTEXT("InvalidSurfaceControllerInput", "当前关卡或 RVT 地表资产无效。");
			return Result;
		}
		if (!IsEditableEditorWorld(World))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerNotEditorWorld",
				"只能在非 PIE/SIE 的编辑器关卡中创建或修改 RVT 地表控制 Actor。");
			return Result;
		}
		if (SurfaceAsset->bEnableCollision
			&& !SupportsGameplayCollision(SurfaceAsset->AssetType, SurfaceAsset->GeometryPolicy))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerCollisionPolicyInvalid",
				"无法创建或使用控制 Actor：该资产启用了玩法碰撞，但几何策略不是“始终保留模型”。请编辑资产并改为始终保留模型，或关闭碰撞；不会让碰撞随平台或画质变化。");
			return Result;
		}
		if (!SurfaceAsset->Mesh || !SurfaceAsset->Material)
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerAssetIncomplete",
				"所选 RVT 地表资产没有完整配置 Mesh 与 Material，请先在“编辑贴花资产”中补齐。");
			return Result;
		}
		if (!FDevKitRVTMeshDecalService::MaterialWritesSurfaceToRuntimeVirtualTexture(
			SurfaceAsset->Material))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerMaterialMissingSurfaceWriter",
				"无法创建或使用控制 Actor：该最终资产的材质没有检测到 Surface RVT Output。请返回编辑页补齐 RVT Writer 材质后再放置，避免 Exclusive 贴花不可见。");
			return Result;
		}
		if (SurfaceAsset->bBindWorldHeight
			&& !FDevKitRVTMeshDecalService::MaterialWritesWorldHeightToRuntimeVirtualTexture(
				SurfaceAsset->Material))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerMaterialMissingHeightWriter",
				"无法创建或使用控制 Actor：该最终资产要求 WorldHeight，但材质没有检测到 WorldHeight RVT Output。请返回编辑页补齐材质后再放置。");
			return Result;
		}

		ULevel* TargetLevel = ResolveSurfaceActorLevel(World, bPlaceInCurrentLevel);
		if (!TargetLevel)
		{
			Result.Message = LOCTEXT("NoSurfaceControllerLevel", "无法解析 RVT 地表控制 Actor 的目标 Level。");
			return Result;
		}

		const FDevKitRVTAutoBindingResult AutoBinding =
			FDevKitRVTMeshDecalService::ResolveRuntimeVirtualTexturesForWorld(World);
		URuntimeVirtualTexture* SurfaceRVT = AutoBinding.bSurfaceResolved
			? LoadObject<URuntimeVirtualTexture>(nullptr, *AutoBinding.SurfaceObjectPath)
			: nullptr;
		URuntimeVirtualTexture* HeightRVT = AutoBinding.bHeightResolved
			? LoadObject<URuntimeVirtualTexture>(nullptr, *AutoBinding.HeightObjectPath)
			: nullptr;
		if (!SurfaceRVT || !IsSurfaceRVT(SurfaceRVT))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerMissingSurfaceRVT",
				"当前关卡没有解析到可用的 Surface RVT；请先用“关卡 RVT 工具”创建并加载 Surface RVT/Volume。");
			return Result;
		}
		if (!HasActiveRuntimeVirtualTextureVolume(World, SurfaceRVT))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerMissingActiveSurfaceVolume",
				"当前关卡虽然存在 Surface RVT 资产，但没有已加载并注册、正在引用它的 Runtime Virtual Texture Volume。请先用“关卡 RVT 工具”创建/加载 Volume，避免 Exclusive 贴花消失。");
			return Result;
		}
		const bool bMayUseProjectionOnly =
			!ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
				SurfaceAsset->AssetType,
				SurfaceAsset->GeometryPolicy,
				true,
				true,
				SurfaceAsset->bEnableCollision);
		if (bMayUseProjectionOnly
			&& !HasActiveRuntimeVirtualTextureVolume(World, SurfaceRVT, true))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerVolumeDoesNotHidePrimitives",
				"当前 Surface RVT Volume 没有启用 Hide Primitives。该资产会使用 From Virtual Texture / Exclusive 投射；请在 Volume 勾选 Hide Primitives，或用“关卡 RVT 工具”更新 Volume，避免中低画质仍显示源模型。");
			return Result;
		}
		if (SurfaceAsset->bBindWorldHeight && (!HeightRVT || !IsWorldHeightRVT(HeightRVT)))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerMissingHeightRVT",
				"该资产要求绑定 WorldHeight，但当前关卡没有解析到可用的 WorldHeight RVT。");
			return Result;
		}
		if (SurfaceAsset->bBindWorldHeight && !HasActiveRuntimeVirtualTextureVolume(World, HeightRVT))
		{
			Result.Message = LOCTEXT(
				"SurfaceControllerMissingActiveHeightVolume",
				"该资产要求 WorldHeight，但当前关卡没有已加载并注册、正在引用目标 WorldHeight RVT 的 Volume。请先用“关卡 RVT 工具”创建/加载对应 Volume。");
			return Result;
		}

		ADevKitRVTSurfaceInstanceActor* SurfaceActor = nullptr;
		for (TActorIterator<ADevKitRVTSurfaceInstanceActor> It(World); It; ++It)
		{
			if (It->GetLevel() == TargetLevel && It->SurfaceAsset == SurfaceAsset)
			{
				SurfaceActor = *It;
				break;
			}
		}

		if (!SurfaceActor)
		{
			World->Modify();
			TargetLevel->Modify();

			const FString ActorBaseName = FString::Printf(TEXT("RVT_%s_Instances"), *SurfaceAsset->GetName());
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Name = MakeUniqueObjectName(
				TargetLevel,
				ADevKitRVTSurfaceInstanceActor::StaticClass(),
				FName(*ActorBaseName));
			SpawnParameters.OverrideLevel = TargetLevel;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParameters.ObjectFlags |= RF_Transactional;
			SurfaceActor = World->SpawnActor<ADevKitRVTSurfaceInstanceActor>(
				ADevKitRVTSurfaceInstanceActor::StaticClass(),
				FTransform::Identity,
				SpawnParameters);
			if (!SurfaceActor)
			{
				Result.Message = LOCTEXT(
					"CreateSurfaceControllerFailed",
					"创建 RVT 地表控制 Actor 失败。");
				return Result;
			}

			SurfaceActor->SetActorLabel(ActorBaseName);
			Result.bCreatedNewActor = true;
		}

		SurfaceActor->Modify();
		if (!SurfaceActor->InitializeSurfaceAsset(SurfaceAsset))
		{
			Result.Message = LOCTEXT(
				"InitializeSurfaceControllerFailed",
				"控制 Actor 已包含其他地表资产的实例，不能直接改绑；请新建或先显式迁移实例。");
			if (Result.bCreatedNewActor)
			{
				World->DestroyActor(SurfaceActor);
				Result.bCreatedNewActor = false;
			}
			return Result;
		}
		SurfaceActor->SurfaceRVT = SurfaceRVT;
		SurfaceActor->HeightRVT = HeightRVT;
		SurfaceActor->ApplySurfaceAsset();
		SurfaceActor->MarkPackageDirty();
		TargetLevel->MarkPackageDirty();
		EnsureSMInstanceViewportSelectionEnabled();

		Result.bSuccess = true;
		Result.Actor = SurfaceActor;
		Result.Message = FText::Format(
			Result.bCreatedNewActor
				? LOCTEXT(
					"CreatedSurfaceController",
					"已在 {0} 创建控制 Actor：{1}，并自动绑定当前关卡 RVT。")
				: LOCTEXT(
					"FoundSurfaceController",
					"已找到 {0} 中的控制 Actor：{1}，并刷新当前关卡 RVT 绑定。"),
			FText::FromString(TargetLevel->GetOutermost()->GetName()),
			FText::FromString(SurfaceActor->GetActorLabel()));
		return Result;
	}
}

FString FDevKitRVTMeshDecalService::InferDefaultSurfaceAssetFolderFromWorldPackage(
	const FString& WorldPackagePath)
{
	const FString Normalized = NormalizeFolder(WorldPackagePath);
	const FString LevelAssetToken = FString::Printf(TEXT("/%s/"), *LevelAssetFolderName);
	const int32 LevelAssetIndex = Normalized.Find(
		LevelAssetToken,
		ESearchCase::IgnoreCase,
		ESearchDir::FromStart);
	if (LevelAssetIndex != INDEX_NONE)
	{
		return Normalized.Left(LevelAssetIndex) / BakeInfoFolderName / RVTSurfaceLibraryFolderName;
	}

	int32 LastSlashIndex = INDEX_NONE;
	if (Normalized.FindLastChar(TEXT('/'), LastSlashIndex))
	{
		return Normalized.Left(LastSlashIndex) / BakeInfoFolderName / RVTSurfaceLibraryFolderName;
	}

	return FString(TEXT("/Game")) / BakeInfoFolderName / RVTSurfaceLibraryFolderName;
}

FString FDevKitRVTMeshDecalService::BuildDefaultSurfaceAssetName(
	const FString& MeshObjectPath,
	const FString& MaterialObjectPath,
	const EDevKitRVTSurfaceAssetType AssetType,
	const int32 Priority)
{
	FString MeshName = SanitizeNameToken(
		FPackageName::ObjectPathToObjectName(NormalizeObjectPath(MeshObjectPath)));
	if (MeshName.IsEmpty())
	{
		MeshName = TEXT("Mesh");
	}

	FString MaterialName = SanitizeNameToken(
		FPackageName::ObjectPathToObjectName(NormalizeObjectPath(MaterialObjectPath)));
	if (MaterialName.IsEmpty())
	{
		MaterialName = TEXT("Material");
	}

	const FString PriorityToken = Priority < 0
		? FString::Printf(TEXT("PM%d"), FMath::Abs(Priority))
		: FString::Printf(TEXT("P%d"), Priority);
	FString SourceIdentity = FString::Printf(
		TEXT("%d|%s|%s"),
		static_cast<int32>(AssetType),
		*NormalizeObjectPath(MeshObjectPath),
		*NormalizeObjectPath(MaterialObjectPath));
	SourceIdentity.ToLowerInline();
	const FString SourceHash = FString::Printf(TEXT("H%08X"), FCrc::StrCrc32(*SourceIdentity));

	if (AssetType == EDevKitRVTSurfaceAssetType::VisibleObject)
	{
		return FString::Printf(
			TEXT("DA_RVTObject_%s_%s_%s_%s"),
			*MeshName,
			*MaterialName,
			*PriorityToken,
			*SourceHash);
	}

	return FString::Printf(
		TEXT("DA_RVTDecal_%s_%s_%s"),
		*MaterialName,
		*PriorityToken,
		*SourceHash);
}

FDevKitRVTSurfaceAssetResult FDevKitRVTMeshDecalService::CreateOrUpdateSurfaceAsset(
	const FDevKitRVTSurfaceAssetRequest& Request)
{
	FDevKitRVTSurfaceAssetResult Result;
	const FString AssetFolder = NormalizeFolder(Request.AssetFolder);
	FString AssetName = Request.AssetName;
	AssetName.TrimStartAndEndInline();
	if (AssetName.IsEmpty())
	{
		AssetName = BuildDefaultSurfaceAssetName(
			Request.MeshObjectPath,
			Request.MaterialObjectPath,
			Request.AssetType,
			Request.Priority);
	}

	if (!FPackageName::IsValidLongPackageName(AssetFolder, true))
	{
		Result.Message = LOCTEXT(
			"InvalidSurfaceAssetFolder",
			"贴花资产目录必须是有效内容路径，例如 /Game/.../BakeInfo/RVTSurfaceLibrary。");
		return Result;
	}
	if (!IsValidNameToken(AssetName))
	{
		Result.Message = LOCTEXT("InvalidSurfaceAssetName", "贴花资产名称只能使用字母、数字和下划线。");
		return Result;
	}
	if (!FMath::IsFinite(Request.DefaultScale.X)
		|| !FMath::IsFinite(Request.DefaultScale.Y)
		|| !FMath::IsFinite(Request.DefaultScale.Z)
		|| FMath::IsNearlyZero(Request.DefaultScale.X)
		|| FMath::IsNearlyZero(Request.DefaultScale.Y)
		|| FMath::IsNearlyZero(Request.DefaultScale.Z))
	{
		Result.Message = LOCTEXT("InvalidSurfaceDefaultScale", "默认缩放的三个分量都必须是非零有限值。");
		return Result;
	}
	if (!FMath::IsFinite(Request.ZOffset))
	{
		Result.Message = LOCTEXT("InvalidSurfaceZOffset", "Z Offset 必须是有限值。");
		return Result;
	}

	if (Request.bEnableCollision
		&& !SupportsGameplayCollision(Request.AssetType, Request.GeometryPolicy))
	{
		Result.Message = LOCTEXT(
			"SurfaceCollisionRequiresAlwaysVisible",
			"保存失败：启用玩法碰撞的地表物件必须使用“始终保留模型”。Quality Scaled / 仅 RVT 投射会随平台或画质改变视觉几何，不能同时承载玩法碰撞；请改用始终保留模型，或关闭碰撞并使用独立碰撞体。");
		return Result;
	}

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *NormalizeObjectPath(Request.MeshObjectPath));
	if (!Mesh)
	{
		Result.Message = FText::Format(
			LOCTEXT("LoadSurfaceAssetMeshFailed", "无法加载贴花资产 Mesh：{0}"),
			FText::FromString(Request.MeshObjectPath));
		return Result;
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(
		nullptr,
		*NormalizeObjectPath(Request.MaterialObjectPath));
	if (!Material)
	{
		Result.Message = FText::Format(
			LOCTEXT("LoadSurfaceAssetMaterialFailed", "无法加载贴花资产 Material：{0}"),
			FText::FromString(Request.MaterialObjectPath));
		return Result;
	}

	if (!EnsureContentFolder(AssetFolder))
	{
		Result.Message = FText::Format(
			LOCTEXT("CreateSurfaceAssetFolderFailed", "创建贴花资产目录失败：{0}"),
			FText::FromString(AssetFolder));
		return Result;
	}

	const FString ObjectPath = MakeObjectPath(AssetFolder, AssetName);
	Result.ObjectPath = ObjectPath;
	UObject* ExistingObject = LoadObject<UObject>(nullptr, *ObjectPath);
	UDevKitRVTSurfaceAsset* SurfaceAsset = Cast<UDevKitRVTSurfaceAsset>(ExistingObject);
	const bool bEditingExistingAsset = !Request.ExpectedExistingAsset.IsExplicitlyNull();
	UDevKitRVTSurfaceAsset* ExpectedExistingAsset = Request.ExpectedExistingAsset.Get();
	if (bEditingExistingAsset && !ExpectedExistingAsset)
	{
		Result.Message = LOCTEXT(
			"ExpectedSurfaceAssetUnavailable",
			"编辑失败：原先选中的 RVT 地表资产已经失效或被删除，请刷新资产库后重新选择；不会创建或覆盖其他资产。");
		return Result;
	}
	if (bEditingExistingAsset && SurfaceAsset != ExpectedExistingAsset)
	{
		Result.Message = FText::Format(
			LOCTEXT(
				"EditedSurfaceAssetTargetChanged",
				"编辑失败：保存目录或资产名称已改变，目标不再是原先选中的资产。原资产：{0}；当前目标：{1}。请恢复原路径；编辑操作不会静默另存或覆盖。"),
			FText::FromString(ExpectedExistingAsset->GetPathName()),
			FText::FromString(ObjectPath));
		return Result;
	}
	if (!bEditingExistingAsset && ExistingObject)
	{
		Result.Message = FText::Format(
			LOCTEXT(
				"NewSurfaceAssetNameCollision",
				"新建失败：目标路径已经存在资产，不会静默覆盖：{0}。请更换资产名称。"),
			FText::FromString(ObjectPath));
		return Result;
	}
	if (SurfaceAsset)
	{
		const FString ExistingAssetFilename = FPackageName::LongPackageNameToFilename(
			SurfaceAsset->GetOutermost()->GetName(),
			FPackageName::GetAssetPackageExtension());
		if (IFileManager::Get().FileExists(*ExistingAssetFilename)
			&& IFileManager::Get().IsReadOnly(*ExistingAssetFilename))
		{
			Result.Message = FText::Format(
				LOCTEXT(
					"SurfaceAssetReadOnlyBeforeModify",
					"更新失败：目标资产文件为只读，尚未修改内存内容：{0}。请先在 P4 checkout 后重试。"),
				FText::FromString(ExistingAssetFilename));
			return Result;
		}
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateOrUpdateSurfaceAssetTransaction", "创建或更新 RVT 地表资产"));
	if (!SurfaceAsset)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = UDevKitRVTSurfaceAsset::StaticClass();
		SurfaceAsset = Cast<UDevKitRVTSurfaceAsset>(AssetTools.CreateAsset(
			AssetName,
			AssetFolder,
			UDevKitRVTSurfaceAsset::StaticClass(),
			Factory));
		if (!SurfaceAsset)
		{
			Result.Message = FText::Format(
				LOCTEXT("CreateSurfaceAssetFailed", "创建 RVT 地表资产失败：{0}"),
				FText::FromString(ObjectPath));
			return Result;
		}
		Result.bCreatedNewAsset = true;
	}

	SurfaceAsset->Modify();
	SurfaceAsset->PreEditChange(nullptr);
	SurfaceAsset->DisplayName = Request.DisplayName.IsEmpty()
		? FText::FromString(AssetName)
		: Request.DisplayName;
	SurfaceAsset->Description = Request.Description;
	SurfaceAsset->AssetType = Request.AssetType;
	SurfaceAsset->GeometryPolicy = Request.GeometryPolicy;
	SurfaceAsset->Mesh = Mesh;
	SurfaceAsset->Material = Material;
	SurfaceAsset->Priority = Request.Priority;
	SurfaceAsset->DefaultScale = Request.DefaultScale;
	SurfaceAsset->ZOffset = Request.ZOffset;
	SurfaceAsset->bAlignToNormal = Request.bAlignToNormal;
	SurfaceAsset->bRandomYaw = Request.bRandomYaw;
	SurfaceAsset->bBindWorldHeight = Request.bBindWorldHeight;
	SurfaceAsset->bEnableCollision = Request.bEnableCollision;
	SurfaceAsset->bCastShadow = Request.bCastShadow;
	SurfaceAsset->MarkPackageDirty();
	FPropertyChangedEvent PropertyChangedEvent(nullptr, EPropertyChangeType::ValueSet);
	SurfaceAsset->PostEditChangeProperty(PropertyChangedEvent);

	TArray<UPackage*> PackagesToSave{SurfaceAsset->GetPackage()};
	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false))
	{
		Result.Message = FText::Format(
			LOCTEXT(
				"SaveSurfaceAssetFailed",
				"RVT 地表资产已更新到内存，但保存失败；请检查 P4 checkout/只读状态：{0}"),
			FText::FromString(ObjectPath));
		return Result;
	}

	Result.bSuccess = true;
	Result.Asset = SurfaceAsset;
	Result.ObjectPath = SurfaceAsset->GetPathName();

	// The DataAsset is the authoring source, while each controller serializes the derived component
	// state used for rendering and collision. Refresh every loaded controller that references the
	// updated asset so artists see the change immediately and Undo captures both objects together.
	int32 RefreshedControllerCount = 0;
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (IsEditableEditorWorld(EditorWorld))
	{
		for (TActorIterator<ADevKitRVTSurfaceInstanceActor> It(EditorWorld); It; ++It)
		{
			ADevKitRVTSurfaceInstanceActor* SurfaceActor = *It;
			if (!SurfaceActor || SurfaceActor->SurfaceAsset != SurfaceAsset)
			{
				continue;
			}

			SurfaceActor->Modify();
			if (SurfaceActor->InstanceComponent)
			{
				SurfaceActor->InstanceComponent->Modify();
			}
			SurfaceActor->ApplySurfaceAsset();
			SurfaceActor->MarkPackageDirty();
			if (ULevel* ActorLevel = SurfaceActor->GetLevel())
			{
				ActorLevel->MarkPackageDirty();
			}
			++RefreshedControllerCount;
		}

		if (RefreshedControllerCount > 0)
		{
			GEditor->RedrawLevelEditingViewports();
		}
	}

	const bool bWritesSurface = MaterialWritesSurfaceToRuntimeVirtualTexture(Material);
	const bool bWritesHeight = MaterialWritesWorldHeightToRuntimeVirtualTexture(Material);
	FText Warning = FText::GetEmpty();
	if (!bWritesSurface)
	{
		Warning = LOCTEXT(
			"SurfaceAssetNoSurfaceWriterWarning",
			" 注意：当前材质未检测到 Surface RVT Output；资产已保存，但不会写入地表 RVT，需由美术继续配置材质。");
	}
	else if (Request.bBindWorldHeight && !bWritesHeight)
	{
		Warning = LOCTEXT(
			"SurfaceAssetNoHeightWriterWarning",
			" 注意：资产要求绑定 WorldHeight，但当前材质未检测到 WorldHeight Output；资产已保存，需由美术继续配置材质。");
	}
	const FText AssetSaveMessage = FText::Format(
		Result.bCreatedNewAsset
			? LOCTEXT("CreatedSurfaceAsset", "已创建 RVT 地表资产：{0}{1}")
			: LOCTEXT("UpdatedSurfaceAsset", "已更新 RVT 地表资产：{0}{1}"),
		FText::FromString(Result.ObjectPath),
		Warning);
	Result.Message = RefreshedControllerCount > 0
		? FText::Format(
			LOCTEXT(
				"SurfaceAssetSavedAndControllersRefreshed",
				"{0} 已同步刷新当前 World 中引用该资产的 {1} 个控制 Actor。"),
			AssetSaveMessage,
			FText::AsNumber(RefreshedControllerCount))
		: AssetSaveMessage;
	return Result;
}

FDevKitRVTSurfaceControllerResult FDevKitRVTMeshDecalService::FindOrCreateSurfaceInstanceActor(
	UWorld* World,
	UDevKitRVTSurfaceAsset* SurfaceAsset,
	const bool bPlaceInCurrentLevel)
{
	FScopedTransaction Transaction(LOCTEXT("FindOrCreateSurfaceControllerTransaction", "创建或刷新 RVT 地表控制 Actor"));
	FDevKitRVTSurfaceControllerResult Result = FindOrCreateSurfaceInstanceActorInternal(
		World,
		SurfaceAsset,
		bPlaceInCurrentLevel);
	if (!Result.bSuccess)
	{
		Transaction.Cancel();
	}
	return Result;
}

FDevKitRVTSurfaceControllerResult FDevKitRVTMeshDecalService::PlaceSurfaceAssetInstanceAtViewportCursor(
	FLevelEditorViewportClient* ViewportClient,
	UDevKitRVTSurfaceAsset* SurfaceAsset,
	const bool bPlaceInCurrentLevel)
{
	if (!ViewportClient || !SurfaceAsset)
	{
		FDevKitRVTSurfaceControllerResult Result;
		Result.Message = LOCTEXT("InvalidSurfaceCursorPlacementInput", "关卡视口或 RVT 地表资产无效。");
		return Result;
	}

	const FViewportCursorLocation Cursor = ViewportClient->GetCursorWorldLocationFromMousePos();
	return PlaceSurfaceAssetInstanceFromRay(
		ViewportClient,
		SurfaceAsset,
		bPlaceInCurrentLevel,
		Cursor.GetOrigin(),
		Cursor.GetDirection());
}

FDevKitRVTSurfaceControllerResult FDevKitRVTMeshDecalService::PlaceSurfaceAssetInstanceAtViewportCenter(
	FLevelEditorViewportClient* ViewportClient,
	UDevKitRVTSurfaceAsset* SurfaceAsset,
	const bool bPlaceInCurrentLevel)
{
	FDevKitRVTSurfaceControllerResult Result;
	if (!ViewportClient || !ViewportClient->Viewport || !SurfaceAsset)
	{
		Result.Message = LOCTEXT("InvalidSurfaceCenterPlacementInput", "关卡视口或 RVT 地表资产无效。");
		return Result;
	}

	const FIntPoint ViewportSize = ViewportClient->Viewport->GetSizeXY();
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		Result.Message = LOCTEXT("InvalidSurfaceCenterViewportSize", "关卡视口尺寸无效，无法从中心放置实例。");
		return Result;
	}

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));
	FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
	if (!SceneView)
	{
		Result.Message = LOCTEXT("NoSurfaceCenterSceneView", "无法计算当前关卡视口的中心射线。");
		return Result;
	}

	const FViewportCursorLocation CenterCursor(
		SceneView,
		ViewportClient,
		ViewportSize.X / 2,
		ViewportSize.Y / 2);
	return PlaceSurfaceAssetInstanceFromRay(
		ViewportClient,
		SurfaceAsset,
		bPlaceInCurrentLevel,
		CenterCursor.GetOrigin(),
		CenterCursor.GetDirection());
}

FDevKitRVTSurfaceControllerResult FDevKitRVTMeshDecalService::PlaceSurfaceAssetInstanceFromRay(
	FLevelEditorViewportClient* ViewportClient,
	UDevKitRVTSurfaceAsset* SurfaceAsset,
	const bool bPlaceInCurrentLevel,
	const FVector& RayOrigin,
	const FVector& RayDirection)
{
	FDevKitRVTSurfaceControllerResult Result;
	if (!ViewportClient || !SurfaceAsset)
	{
		Result.Message = LOCTEXT("InvalidSurfacePlacementInput", "关卡视口或 RVT 地表资产无效。");
		return Result;
	}

	UWorld* World = ViewportClient->GetWorld();
	if (!IsEditableEditorWorld(World))
	{
		Result.Message = LOCTEXT(
			"SurfacePlacementNotEditorWorld",
			"只能在非 PIE/SIE 的编辑器关卡视口中放置 RVT 地表实例。");
		return Result;
	}

	const FVector TraceStart = RayOrigin;
	const FVector TraceEnd = TraceStart
		+ RayDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector) * HALF_WORLD_MAX;
	FCollisionQueryParams QueryParams(TEXT("DevKitRVTSurfaceAssetPlacement"), false);
	QueryParams.bReturnPhysicalMaterial = false;
	// Excluded instance containers can contain blocking collision. Ignore them at query
	// time so the trace can still reach the Landscape/static-mesh surface underneath.
	for (TActorIterator<ADevKitRVTSurfaceInstanceActor> It(World); It; ++It)
	{
		QueryParams.AddIgnoredActor(*It);
	}
	for (TActorIterator<AInstancedFoliageActor> It(World); It; ++It)
	{
		QueryParams.AddIgnoredActor(*It);
	}
	const FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);
	TArray<FHitResult> Hits;
	World->LineTraceMultiByObjectType(Hits, TraceStart, TraceEnd, ObjectQueryParams, QueryParams);

	const FHitResult* PlacementHit = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		if (IsSurfacePlacementComponent(Hit.Component.Get()))
		{
			PlacementHit = &Hit;
			break;
		}
	}
	if (!PlacementHit)
	{
		Result.Message = LOCTEXT(
			"SurfacePlacementMissed",
			"放置失败：鼠标位置没有命中可放置的 Landscape、Static Mesh 或 BSP 表面。");
		return Result;
	}

	const FVector SurfaceNormal = PlacementHit->ImpactNormal.GetSafeNormal(
		UE_SMALL_NUMBER,
		FVector::UpVector);
	FQuat PlacementRotation = SurfaceAsset->bAlignToNormal
		? FQuat::FindBetweenNormals(FVector::UpVector, SurfaceNormal)
		: FQuat::Identity;
	if (SurfaceAsset->bRandomYaw)
	{
		const FVector YawAxis = SurfaceAsset->bAlignToNormal ? SurfaceNormal : FVector::UpVector;
		PlacementRotation = FQuat(YawAxis, FMath::DegreesToRadians(FMath::FRandRange(0.0f, 360.0f)))
			* PlacementRotation;
	}
	PlacementRotation.Normalize();
	const FTransform InstanceTransform(
		PlacementRotation,
		PlacementHit->ImpactPoint + SurfaceNormal * SurfaceAsset->ZOffset,
		SurfaceAsset->DefaultScale);

	FScopedTransaction Transaction(LOCTEXT("PlaceSurfaceAssetInstanceTransaction", "放置单个 RVT 地表资产实例"));
	Result = FindOrCreateSurfaceInstanceActorInternal(World, SurfaceAsset, bPlaceInCurrentLevel);
	if (!Result.bSuccess || !Result.Actor.IsValid())
	{
		Transaction.Cancel();
		return Result;
	}

	ADevKitRVTSurfaceInstanceActor* SurfaceActor = Result.Actor.Get();
	SurfaceActor->Modify();
	Result.InstanceIndex = SurfaceActor->AddSurfaceInstance(InstanceTransform);
	if (Result.InstanceIndex == INDEX_NONE)
	{
		Result.bSuccess = false;
		Result.Message = LOCTEXT("AddSurfaceAssetInstanceFailed", "控制 Actor 已就绪，但添加 ISM 实例失败。");
		return Result;
	}

	SurfaceActor->MarkPackageDirty();
	if (SurfaceActor->GetLevel())
	{
		SurfaceActor->GetLevel()->MarkPackageDirty();
	}
	SelectOnlySurfaceActor(SurfaceActor);
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}

	Result.bSuccess = true;
	Result.Message = FText::Format(
		LOCTEXT(
			"PlacedSurfaceAssetInstance",
			"已在 {0} 放置第 {1} 个实例。控制 Actor 已选中；在视口点击单个实例后可用 W/E/R 独立移动、旋转和缩放。"),
		FText::FromString(SurfaceActor->GetActorLabel()),
		FText::AsNumber(Result.InstanceIndex + 1));
	return Result;
}

FDevKitRVTSurfaceControllerResult FDevKitRVTMeshDecalService::SelectSurfaceInstanceActor(
	UWorld* World,
	UDevKitRVTSurfaceAsset* SurfaceAsset,
	const bool bPlaceInCurrentLevel)
{
	FScopedTransaction Transaction(LOCTEXT("SelectSurfaceControllerTransaction", "选择 RVT 地表控制 Actor"));
	FDevKitRVTSurfaceControllerResult Result = FindOrCreateSurfaceInstanceActorInternal(
		World,
		SurfaceAsset,
		bPlaceInCurrentLevel);
	if (!Result.bSuccess || !Result.Actor.IsValid())
	{
		Transaction.Cancel();
		return Result;
	}

	SelectOnlySurfaceActor(Result.Actor.Get());
	Result.Message = FText::Format(
		LOCTEXT(
			"SelectedSurfaceController",
			"已选择控制 Actor：{0}。继续点击视口中的单个实例，即可用 W/E/R 独立编辑其 Transform。"),
		FText::FromString(Result.Actor->GetActorLabel()));
	return Result;
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

FString FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(
	const FString& MaterialObjectPath,
	int32 TranslucencySortPriority,
	EDevKitRVTSurfaceItemMode ItemMode,
	const FString& MeshObjectPath)
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
	FString SourceIdentity = FString::Printf(
		TEXT("%d|%s|%s"),
		static_cast<int32>(ItemMode),
		*NormalizeObjectPath(MeshObjectPath),
		*NormalizeObjectPath(MaterialObjectPath));
	SourceIdentity.ToLowerInline();
	const FString SourceHash = FString::Printf(TEXT("H%08X"), FCrc::StrCrc32(*SourceIdentity));
	if (ItemMode == EDevKitRVTSurfaceItemMode::VisibleGroundObject)
	{
		FString MeshName = SanitizeNameToken(
			FPackageName::ObjectPathToObjectName(NormalizeObjectPath(MeshObjectPath)));
		if (MeshName.IsEmpty())
		{
			MeshName = TEXT("Mesh");
		}
		return FString::Printf(
			TEXT("FT_RVTObject_%s_%s_%s_%s"),
			*MeshName,
			*MaterialName,
			*PriorityToken,
			*SourceHash);
	}

	return FString::Printf(TEXT("FT_RVTDecal_%s_%s_%s"), *MaterialName, *PriorityToken, *SourceHash);
}

FString FDevKitRVTMeshDecalService::GetDefaultPlaneMeshObjectPath()
{
	return DefaultPlaneMeshObjectPath;
}

TOptional<FDevKitRVTMeshDecalPaths> FDevKitRVTMeshDecalService::BuildPaths(
	const FDevKitRVTMeshDecalRequest& Request,
	FText& OutError)
{
	FDevKitRVTMeshDecalPaths Paths;
	Paths.FoliageTypeFolder = NormalizeFolder(Request.FoliageTypeFolder);
	Paths.FoliageTypeName = Request.FoliageTypeNameOverride;
	Paths.FoliageTypeName.TrimStartAndEndInline();
	if (Paths.FoliageTypeName.IsEmpty())
	{
		Paths.FoliageTypeName = BuildDefaultFoliageTypeName(
			Request.MaterialObjectPath,
			Request.TranslucencySortPriority,
			Request.ItemMode,
			Request.MeshObjectPath);
	}

	if (!FPackageName::IsValidLongPackageName(Paths.FoliageTypeFolder, true))
	{
		OutError = LOCTEXT(
			"InvalidFoliageTypeFolder",
			"库保存目录必须是有效内容路径，例如 /Game/Art/Map/Map_Data/LevelName/BakeInfo/RVTDecalFoliage。");
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

	if (Request.Density < 0.0f || Request.Radius < 0.0f)
	{
		OutError = LOCTEXT("InvalidPaintingValues", "Density 和 Radius 不能小于 0。");
		return TOptional<FDevKitRVTMeshDecalPaths>();
	}

	if (NormalizeObjectPath(Request.SurfaceRuntimeVirtualTextureObjectPath).IsEmpty())
	{
		OutError = LOCTEXT("MissingSurfaceRVT", "没有解析到当前关卡的 Surface RVT；请先用“关卡 RVT 工具”创建并加载 RVT。");
		return TOptional<FDevKitRVTMeshDecalPaths>();
	}

	if (Request.ItemMode == EDevKitRVTSurfaceItemMode::VisibleGroundObject
		&& Request.bBindWorldHeight
		&& NormalizeObjectPath(Request.HeightRuntimeVirtualTextureObjectPath).IsEmpty())
	{
		OutError = LOCTEXT("MissingHeightRVT", "可见地表物件已要求写入 WorldHeight，但当前关卡没有解析到 WorldHeight RVT。");
		return TOptional<FDevKitRVTMeshDecalPaths>();
	}

	Paths.FoliageTypePackage = Paths.FoliageTypeFolder / Paths.FoliageTypeName;
	Paths.FoliageTypeObjectPath = FString::Printf(TEXT("%s.%s"), *Paths.FoliageTypePackage, *Paths.FoliageTypeName);
	OutError = FText::GetEmpty();
	return Paths;
}

FDevKitRVTMeshDecalResult FDevKitRVTMeshDecalService::CreateOrUpdateFoliageType(
	const FDevKitRVTMeshDecalRequest& Request)
{
	FDevKitRVTMeshDecalResult Result;
	FText Error;
	const TOptional<FDevKitRVTMeshDecalPaths> Paths = BuildPaths(Request, Error);
	if (!Paths.IsSet())
	{
		Result.Message = Error;
		return Result;
	}
	Result.Paths = Paths.GetValue();

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *NormalizeObjectPath(Request.MeshObjectPath));
	if (!Mesh)
	{
		Result.Message = FText::Format(
			LOCTEXT("LoadMeshFailed", "无法加载基础网格：{0}"),
			FText::FromString(Request.MeshObjectPath));
		return Result;
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *NormalizeObjectPath(Request.MaterialObjectPath));
	if (!Material)
	{
		Result.Message = FText::Format(
			LOCTEXT("LoadMaterialFailed", "无法加载材质：{0}"),
			FText::FromString(Request.MaterialObjectPath));
		return Result;
	}
	Result.bMaterialWritesSurfaceRVT = MaterialWritesSurfaceToRuntimeVirtualTexture(Material);
	Result.bMaterialWritesWorldHeightRVT = MaterialWritesWorldHeightToRuntimeVirtualTexture(Material);
	if (!Result.bMaterialWritesSurfaceRVT)
	{
		Result.Message = FText::Format(
			LOCTEXT(
				"MaterialMissingSurfaceRVTOutput",
				"材质没有连接 Surface RVT Output，不能作为 RVT 地表库 Writer：{0}"),
			FText::FromString(Request.MaterialObjectPath));
		return Result;
	}
	if (Request.ItemMode == EDevKitRVTSurfaceItemMode::VisibleGroundObject
		&& Request.bBindWorldHeight
		&& !Result.bMaterialWritesWorldHeightRVT)
	{
		Result.Message = FText::Format(
			LOCTEXT(
				"MaterialMissingHeightRVTOutput",
				"可见地表物件要求写入 WorldHeight，但材质的 RVT Output 未连接 WorldHeight：{0}"),
			FText::FromString(Request.MaterialObjectPath));
		return Result;
	}

	URuntimeVirtualTexture* SurfaceRVT = LoadObject<URuntimeVirtualTexture>(
		nullptr,
		*NormalizeObjectPath(Request.SurfaceRuntimeVirtualTextureObjectPath));
	if (!SurfaceRVT || !IsSurfaceRVT(SurfaceRVT))
	{
		Result.Message = FText::Format(
			LOCTEXT("LoadSurfaceRVTFailed", "目标 Surface RVT 无法加载或类型不是地表材质 RVT：{0}"),
			FText::FromString(Request.SurfaceRuntimeVirtualTextureObjectPath));
		return Result;
	}

	URuntimeVirtualTexture* HeightRVT = nullptr;
	if (Request.ItemMode == EDevKitRVTSurfaceItemMode::VisibleGroundObject && Request.bBindWorldHeight)
	{
		HeightRVT = LoadObject<URuntimeVirtualTexture>(
			nullptr,
			*NormalizeObjectPath(Request.HeightRuntimeVirtualTextureObjectPath));
		if (!HeightRVT || !IsWorldHeightRVT(HeightRVT))
		{
			Result.Message = FText::Format(
				LOCTEXT("LoadHeightRVTFailed", "目标 WorldHeight RVT 无法加载或类型不是 WorldHeight：{0}"),
				FText::FromString(Request.HeightRuntimeVirtualTextureObjectPath));
			return Result;
		}
	}

	if (!EnsureContentFolder(Paths->FoliageTypeFolder))
	{
		Result.Message = FText::Format(
			LOCTEXT("CreateFolderFailed", "创建库保存目录失败：{0}"),
			FText::FromString(Paths->FoliageTypeFolder));
		return Result;
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateRVTSurfaceFoliageTypeTransaction", "创建 RVT 地表物件库项"));

	UFoliageType_InstancedStaticMesh* FoliageType = LoadObject<UFoliageType_InstancedStaticMesh>(
		nullptr,
		*Paths->FoliageTypeObjectPath);
	if (!FoliageType)
	{
		UPackage* Package = CreatePackage(*Paths->FoliageTypePackage);
		if (!Package)
		{
			Result.Message = FText::Format(
				LOCTEXT("CreatePackageFailed", "创建 FoliageType 包失败：{0}"),
				FText::FromString(Paths->FoliageTypePackage));
			return Result;
		}

		FoliageType = NewObject<UFoliageType_InstancedStaticMesh>(
			Package,
			UFoliageType_InstancedStaticMesh::StaticClass(),
			*Paths->FoliageTypeName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!FoliageType)
		{
			Result.Message = LOCTEXT("CreateFoliageTypeFailed", "创建 FoliageType 资产失败。");
			return Result;
		}

		FAssetRegistryModule::AssetCreated(FoliageType);
		Result.bCreatedNewAsset = true;
	}

	const UStaticMesh* PreviousMesh = FoliageType->GetStaticMesh();
	const bool bMeshChanged = PreviousMesh != Mesh;
	const bool bVisibleGroundObject = Request.ItemMode == EDevKitRVTSurfaceItemMode::VisibleGroundObject;
	FProperty* ChangedProperty = bMeshChanged
		? FoliageType->GetClass()->FindPropertyByName(TEXT("Mesh"))
		: nullptr;

	FoliageType->Modify();
	FoliageType->PreEditChange(ChangedProperty);
	FoliageType->SetStaticMesh(Mesh);
	FoliageType->OverrideMaterials.Init(Material, FMath::Max(1, Mesh->GetStaticMaterials().Num()));
	FoliageType->NaniteOverrideMaterials.Init(Material, FMath::Max(1, Mesh->GetStaticMaterials().Num()));
	FoliageType->RuntimeVirtualTextures.Reset();
	FoliageType->RuntimeVirtualTextures.Add(SurfaceRVT);
	if (HeightRVT)
	{
		FoliageType->RuntimeVirtualTextures.AddUnique(HeightRVT);
	}
	FoliageType->VirtualTextureRenderPassType = bVisibleGroundObject
		? ERuntimeVirtualTextureMainPassType::Always
		: ERuntimeVirtualTextureMainPassType::Exclusive;
	FoliageType->VirtualTextureCullMips = 0;
	FoliageType->TranslucencySortPriority = Request.TranslucencySortPriority;
	FoliageType->Density = Request.Density;
	FoliageType->Radius = Request.Radius;
	FoliageType->Scaling = EFoliageScaling::Uniform;
	FoliageType->ScaleX = FFloatInterval(Request.MinScale, Request.MaxScale);
	FoliageType->ScaleY = FFloatInterval(Request.MinScale, Request.MaxScale);
	FoliageType->ScaleZ = FFloatInterval(1.0f, 1.0f);
	FoliageType->AlignToNormal = Request.bAlignToNormal;
	FoliageType->AlignMaxAngle = 90.0f;
	FoliageType->RandomYaw = Request.bRandomYaw;
	FoliageType->RandomPitchAngle = 0.0f;
	FoliageType->ZOffset = FFloatInterval(Request.ZOffset, Request.ZOffset);
	FoliageType->CollisionWithWorld = false;
	FoliageType->CastShadow = bVisibleGroundObject;
	FoliageType->bCastDynamicShadow = bVisibleGroundObject;
	FoliageType->bCastStaticShadow = bVisibleGroundObject;
	FoliageType->bCastContactShadow = bVisibleGroundObject;
	FoliageType->bReceivesDecals = bVisibleGroundObject;
	FoliageType->bEnableDensityScaling = false;
	FoliageType->bIncludeInHLOD = bVisibleGroundObject;
	FoliageType->Mobility = EComponentMobility::Static;
	FoliageType->MarkPackageDirty();
	FPropertyChangedEvent PropertyChangedEvent(ChangedProperty);
	FoliageType->PostEditChangeProperty(PropertyChangedEvent);

	TArray<UPackage*> PackagesToSave{FoliageType->GetPackage()};
	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false))
	{
		Result.Message = FText::Format(
			LOCTEXT(
				"SaveFoliageTypeFailed",
				"FoliageType 已更新到内存，但保存失败；请检查 P4 checkout/只读状态：{0}"),
			FText::FromString(Paths->FoliageTypeObjectPath));
		return Result;
	}
	RefreshFoliagePaletteIfOpen();

	Result.bSuccess = true;
	Result.FoliageType = FoliageType;
	const FText BaseMessage = FText::Format(
		Result.bCreatedNewAsset
			? LOCTEXT("CreateSuccess", "已创建 RVT 地表物件库项：{0}")
			: LOCTEXT("UpdateSuccess", "已更新 RVT 地表物件库项：{0}"),
		FText::FromString(Paths->FoliageTypeObjectPath));
	Result.Message = BaseMessage;
	return Result;
}

FDevKitRVTAutoBindingResult FDevKitRVTMeshDecalService::ResolveRuntimeVirtualTexturesForWorld(UWorld* World)
{
	FDevKitRVTAutoBindingResult Result;
	if (!World || !World->GetOutermost())
	{
		Result.Message = LOCTEXT("NoWorldForRVTResolve", "没有可用的编辑器关卡，无法自动解析 RVT。");
		return Result;
	}

	const FString WorldPackagePath = World->GetOutermost()->GetName();
	const FString BakeInfoFolder = FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(WorldPackagePath);
	const FString BaseName = FDevKitLevelRVTService::BuildDefaultGroundRVTNameFromWorldPackage(WorldPackagePath);

	// Only accept volumes backed by this map's BakeInfo folder. Mixing streaming levels can silently pair
	// a Surface RVT from one map with WorldHeight from another map.
	TArray<URuntimeVirtualTexture*> LoadedCurrentMapRVTs;
	const FString BakeInfoPrefix = BakeInfoFolder + TEXT("/");
	for (TActorIterator<ARuntimeVirtualTextureVolume> It(World); It; ++It)
	{
		URuntimeVirtualTexture* RuntimeVirtualTexture = It->VirtualTextureComponent
			? It->VirtualTextureComponent->GetVirtualTexture()
			: nullptr;
		if (RuntimeVirtualTexture
			&& RuntimeVirtualTexture->GetOutermost()->GetName().StartsWith(BakeInfoPrefix, ESearchCase::IgnoreCase))
		{
			LoadedCurrentMapRVTs.AddUnique(RuntimeVirtualTexture);
		}
	}

	LoadedCurrentMapRVTs.Sort([](const URuntimeVirtualTexture& A, const URuntimeVirtualTexture& B)
	{
		const int32 PriorityA = GetSurfaceRVTPriority(&A);
		const int32 PriorityB = GetSurfaceRVTPriority(&B);
		return PriorityA == PriorityB
			? A.GetPathName() < B.GetPathName()
			: PriorityA < PriorityB;
	});
	for (URuntimeVirtualTexture* RuntimeVirtualTexture : LoadedCurrentMapRVTs)
	{
		if (!Result.bSurfaceResolved && IsSurfaceRVT(RuntimeVirtualTexture))
		{
			Result.SurfaceObjectPath = ToObjectPath(RuntimeVirtualTexture);
			Result.bSurfaceResolved = true;
		}
	}

	const FString ExpectedHeightName = FDevKitLevelRVTService::BuildAssetNameForLayout(
		BaseName,
		EDevKitLevelRVTLayout::WorldHeight);
	for (URuntimeVirtualTexture* RuntimeVirtualTexture : LoadedCurrentMapRVTs)
	{
		if (IsWorldHeightRVT(RuntimeVirtualTexture)
			&& (!Result.bHeightResolved || RuntimeVirtualTexture->GetName() == ExpectedHeightName))
		{
			Result.HeightObjectPath = ToObjectPath(RuntimeVirtualTexture);
			Result.bHeightResolved = true;
			if (RuntimeVirtualTexture->GetName() == ExpectedHeightName)
			{
				break;
			}
		}
	}

	// The DataBake level may be unloaded. Fall back only to deterministic names in this map's BakeInfo.
	const EDevKitLevelRVTLayout SurfaceLayouts[] =
	{
		EDevKitLevelRVTLayout::BaseColorNormalSpecular,
		EDevKitLevelRVTLayout::BaseColorNormalRoughness,
		EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask,
		EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular,
		EDevKitLevelRVTLayout::BaseColor
	};
	if (!Result.bSurfaceResolved)
	{
		for (const EDevKitLevelRVTLayout Layout : SurfaceLayouts)
		{
			const FString AssetName = FDevKitLevelRVTService::BuildAssetNameForLayout(BaseName, Layout);
			if (URuntimeVirtualTexture* Candidate = LoadObject<URuntimeVirtualTexture>(nullptr, *MakeObjectPath(BakeInfoFolder, AssetName)))
			{
				if (IsSurfaceRVT(Candidate))
				{
					Result.SurfaceObjectPath = ToObjectPath(Candidate);
					Result.bSurfaceResolved = true;
					break;
				}
			}
		}
	}

	if (!Result.bHeightResolved)
	{
		if (URuntimeVirtualTexture* Candidate = LoadObject<URuntimeVirtualTexture>(
			nullptr,
			*MakeObjectPath(BakeInfoFolder, ExpectedHeightName)))
		{
			if (IsWorldHeightRVT(Candidate))
			{
				Result.HeightObjectPath = ToObjectPath(Candidate);
				Result.bHeightResolved = true;
			}
		}
	}

	if (Result.bSurfaceResolved && Result.bHeightResolved)
	{
		Result.Message = FText::Format(
			LOCTEXT("ResolvedSurfaceAndHeight", "已从当前关卡解析 Surface RVT 与 WorldHeight RVT：{0} / {1}"),
			FText::FromString(FPackageName::ObjectPathToObjectName(Result.SurfaceObjectPath)),
			FText::FromString(FPackageName::ObjectPathToObjectName(Result.HeightObjectPath)));
	}
	else if (Result.bSurfaceResolved)
	{
		Result.Message = FText::Format(
			LOCTEXT("ResolvedSurfaceOnly", "已解析 Surface RVT：{0}；当前关卡未找到 WorldHeight RVT。"),
			FText::FromString(FPackageName::ObjectPathToObjectName(Result.SurfaceObjectPath)));
	}
	else
	{
		Result.Message = LOCTEXT(
			"NoRVTResolved",
			"当前关卡未找到可用 Surface RVT；请先在“关卡 RVT 工具”中创建并加载 Surface RVT/Volume。");
	}

	return Result;
}

FDevKitRVTPlacementResult FDevKitRVTMeshDecalService::AddFoliageTypeToCurrentActor(
	UWorld* World,
	UFoliageType_InstancedStaticMesh* FoliageType,
	bool bPlaceInCurrentLevel)
{
	FDevKitRVTPlacementResult Result;
	if (!World || !FoliageType)
	{
		Result.Message = LOCTEXT("InvalidAddToActorInput", "当前关卡或 FoliageType 无效。");
		return Result;
	}
	if (!IsEditableEditorWorld(World))
	{
		Result.Message = LOCTEXT("AddToActorNotEditorWorld", "只能在非 PIE/SIE 的编辑器关卡中修改 InstancedFoliageActor。");
		return Result;
	}

	// Match native FEdModeFoliage::AddFoliageAsset. In a partitioned world GetDefault creates the
	// transient palette IFA instead of materializing an empty actor partition at a stale click location.
	const FScopedTransaction Transaction(LOCTEXT("AddRVTSurfaceTypeToIFA", "将 RVT 地表物件加入 Foliage 库"));
	AInstancedFoliageActor* InstancedFoliageActor = AInstancedFoliageActor::GetDefault(World);
	if (!InstancedFoliageActor)
	{
		Result.Message = LOCTEXT("GetInstancedFoliageActorFailed", "无法获取当前关卡的 InstancedFoliageActor。");
		return Result;
	}

	(void)bPlaceInCurrentLevel;
	InstancedFoliageActor->Modify();
	UFoliageType* LocalFoliageType = InstancedFoliageActor->AddFoliageType(FoliageType);
	if (!LocalFoliageType)
	{
		Result.Message = LOCTEXT("AddFoliageTypeToActorFailed", "向 InstancedFoliageActor 注册 FoliageType 失败。");
		return Result;
	}
	SelectOnlyFoliageType(World, LocalFoliageType);

	InstancedFoliageActor->MarkPackageDirty();
	RefreshFoliagePaletteIfOpen();
	Result.bSuccess = true;
	Result.InstancedFoliageActor = InstancedFoliageActor;
	Result.Message = FText::Format(
		LOCTEXT("AddedFoliageTypeToActor", "已将 {0} 加入当前 Foliage 库并设为唯一激活项（Palette IFA：{1}）。"),
		FText::FromString(FoliageType->GetName()),
		FText::FromString(InstancedFoliageActor->GetActorLabel()));
	return Result;
}

FDevKitRVTPlacementResult FDevKitRVTMeshDecalService::PlaceSingleInstanceAtViewportCursor(
	FLevelEditorViewportClient* ViewportClient,
	UFoliageType_InstancedStaticMesh* FoliageType,
	bool bPlaceInCurrentLevel)
{
	FDevKitRVTPlacementResult Result;
	if (!ViewportClient || !FoliageType)
	{
		Result.Message = LOCTEXT("InvalidSinglePlacementInput", "关卡视口或 FoliageType 无效。");
		return Result;
	}

	UWorld* World = ViewportClient->GetWorld();
	if (!World)
	{
		Result.Message = LOCTEXT("NoWorldForSinglePlacement", "目标视口没有可编辑关卡。");
		return Result;
	}
	if (!IsEditableEditorWorld(World))
	{
		Result.Message = LOCTEXT("SinglePlacementNotEditorWorld", "放置失败：只能在非 PIE/SIE 的编辑器关卡视口中写入实例。");
		return Result;
	}

	const FViewportCursorLocation Cursor = ViewportClient->GetCursorWorldLocationFromMousePos();
	FDesiredFoliageInstance DesiredInstance(
		Cursor.GetOrigin(),
		Cursor.GetOrigin() + Cursor.GetDirection() * HALF_WORLD_MAX,
		FoliageType);
	FHitResult Hit;
	const FFoliageTraceFilterFunc TraceFilter = [](const UPrimitiveComponent* Component)
	{
		// Match the native Landscape/Static Mesh/BSP filters and avoid stacking on foliage instances.
		return Component
			&& (IsLandscapeCollisionComponent(Component)
				|| (Component->IsA<UStaticMeshComponent>()
					&& !Component->IsA<UFoliageInstancedStaticMeshComponent>())
				|| Component->IsA<UBrushComponent>()
				|| Component->IsA<UModelComponent>());
	};
	const bool bHit = AInstancedFoliageActor::FoliageTrace(
		World,
		Hit,
		DesiredInstance,
		TEXT("DevKitRVTSurfaceLibraryPlacement"),
		false,
		TraceFilter,
		false);
	if (!bHit || !Hit.Component.IsValid())
	{
		Result.Message = LOCTEXT("SinglePlacementMissed", "放置失败：拖放位置没有命中可放置的 Landscape、Static Mesh 或 BSP 表面。");
		return Result;
	}

	const FDoubleInterval HeightRange(
		static_cast<double>(FoliageType->Height.Min),
		static_cast<double>(FoliageType->Height.Max));
	if (!HeightRange.Contains(Hit.ImpactPoint.Z))
	{
		Result.Message = LOCTEXT("SinglePlacementHeightRejected", "放置失败：落点高度不在 FoliageType 的 Height 范围内。");
		return Result;
	}
	const double NormalZ = Hit.ImpactNormal.GetSafeNormal().Z;
	const double MaxNormalAngle = FMath::Cos(FMath::DegreesToRadians(FoliageType->GroundSlopeAngle.Max));
	const double MinNormalAngle = FMath::Cos(FMath::DegreesToRadians(FoliageType->GroundSlopeAngle.Min));
	if (MaxNormalAngle > NormalZ + SMALL_NUMBER || MinNormalAngle < NormalZ - SMALL_NUMBER)
	{
		Result.Message = LOCTEXT("SinglePlacementSlopeRejected", "放置失败：落点坡度不在 FoliageType 的 Ground Slope Angle 范围内。");
		return Result;
	}

	FPotentialInstance PotentialInstance(
		Hit.ImpactPoint,
		Hit.ImpactNormal,
		Hit.Component.Get(),
		1.0f,
		DesiredInstance);
	FFoliageInstance Instance;
	if (!PotentialInstance.PlaceInstance(World, FoliageType, Instance, false))
	{
		Result.Message = LOCTEXT(
			"SinglePlacementRejected",
			"放置失败：该位置未通过 FoliageType 的缩放、Z Offset 或碰撞规则。");
		return Result;
	}
	Instance.BaseComponent = PotentialInstance.HitComponent;

	const float PlacementRadius = FoliageType->GetRadius(true);
	if (PlacementRadius > 0.0f)
	{
		const FSphere PlacementSphere(Hit.ImpactPoint, PlacementRadius);
		for (TActorIterator<AInstancedFoliageActor> It(World); It; ++It)
		{
			if (const FFoliageInfo* ExistingInfo = It->FindInfo(FoliageType))
			{
				if (ExistingInfo->CheckForOverlappingSphere(PlacementSphere))
				{
					Result.Message = LOCTEXT("SinglePlacementRadiusRejected", "放置失败：最终落点与同类型实例的距离小于 Single Instance Radius。");
					return Result;
				}
			}
		}
	}

	ULevel* TargetLevel = World->PersistentLevel.Get();
	if (bPlaceInCurrentLevel && World->GetCurrentLevel())
	{
		TargetLevel = World->GetCurrentLevel();
	}
	else if (PotentialInstance.HitComponent && PotentialInstance.HitComponent->GetComponentLevel())
	{
		TargetLevel = PotentialInstance.HitComponent->GetComponentLevel();
	}

	// Resolve/create the partition only after PlaceInstance has produced its final location and BSP base.
	// The transaction starts before Get so a newly-created IFA participates in Undo.
	const FScopedTransaction Transaction(LOCTEXT("PlaceSingleRVTSurfaceInstance", "放置单个 RVT 地表物件实例"));
	AInstancedFoliageActor* InstancedFoliageActor = AInstancedFoliageActor::Get(
		World,
		true,
		TargetLevel,
		Instance.Location);
	if (!InstancedFoliageActor)
	{
		Result.Message = LOCTEXT("SinglePlacementNoIFA", "放置失败：无法获取最终落点所属的 InstancedFoliageActor。");
		return Result;
	}

	InstancedFoliageActor->Modify();
	FFoliageInfo* FoliageInfo = nullptr;
	UFoliageType* LocalFoliageType = InstancedFoliageActor->AddFoliageType(FoliageType, &FoliageInfo);
	if (!LocalFoliageType || !FoliageInfo)
	{
		Result.Message = LOCTEXT("SinglePlacementAddTypeFailed", "放置失败：无法向 InstancedFoliageActor 注册 FoliageType。");
		return Result;
	}
	SelectOnlyFoliageType(World, LocalFoliageType);

	FoliageInfo->AddInstance(LocalFoliageType, Instance, PotentialInstance.HitComponent);
	InstancedFoliageActor->MarkPackageDirty();
	AInstancedFoliageActor::InstanceCountChanged.Broadcast(LocalFoliageType);
	RefreshFoliagePaletteIfOpen();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}

	Result.bSuccess = true;
	Result.InstancedFoliageActor = InstancedFoliageActor;
	Result.Message = FText::Format(
		LOCTEXT("SinglePlacementSuccess", "已在 {0} 中放置 1 个 {1} 实例。"),
		FText::FromString(InstancedFoliageActor->GetActorLabel()),
		FText::FromString(FoliageType->GetName()));
	return Result;
}

bool FDevKitRVTMeshDecalService::OpenFoliageMode(
	bool bSingleInstanceMode,
	bool bPlaceInCurrentLevel,
	FText& OutMessage)
{
	if (!GEditor)
	{
		OutMessage = LOCTEXT("NoEditorForFoliageMode", "编辑器尚未就绪，无法打开植被模式。");
		return false;
	}
	if (!IsEditableEditorWorld(GEditor->GetEditorWorldContext().World()))
	{
		OutMessage = LOCTEXT("FoliageModeNotEditorWorld", "只能在非 PIE/SIE 的编辑器关卡中打开并修改 Foliage Mode。");
		return false;
	}

	FModuleManager::LoadModuleChecked<IFoliageEditModule>(TEXT("FoliageEdit"));
	FEditorModeTools& ModeTools = GLevelEditorModeTools();
	// A recycled Foliage mode preserves its previous Select/Lasso/Reapply tool flags. Public editor
	// APIs do not expose FEdModeFoliage::OnSetPlace, so purge any active, pending, or recycled mode
	// before activation. The fresh mode defaults to Paint and loads Single/current-level from config.
	ModeTools.DestroyMode(FBuiltinEditorModes::EM_Foliage);
	GConfig->SetBool(TEXT("FoliageEdit"), TEXT("IsInSingleInstantiationMode"), bSingleInstanceMode, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("FoliageEdit"), TEXT("IsInSpawnInCurrentLevelMode"), bPlaceInCurrentLevel, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
	ModeTools.ActivateMode(FBuiltinEditorModes::EM_Foliage);
	if (!ModeTools.IsModeActive(FBuiltinEditorModes::EM_Foliage))
	{
		OutMessage = LOCTEXT("ActivateFoliageModeFailed", "无法激活 UE 原生 Foliage Mode。");
		return false;
	}
	RefreshFoliagePaletteIfOpen();

	OutMessage = bSingleInstanceMode
		? LOCTEXT("OpenedFoliageSingleMode", "已进入 UE 原生植被模式的 Single 单体放置；库项已在当前 InstancedFoliageActor 中。")
		: LOCTEXT("OpenedFoliageEditMode", "已进入 UE 原生植被模式；可使用 Paint、Single、Select、Move、Reapply 编辑当前 Actor 的实例。");
	return true;
}

bool FDevKitRVTMeshDecalService::MaterialWritesSurfaceToRuntimeVirtualTexture(const UMaterialInterface* Material)
{
	const UMaterialExpressionRuntimeVirtualTextureOutput* Output = FindRuntimeVirtualTextureOutput(Material);
	return Output
		&& (Output->BaseColor.IsConnected()
			|| Output->Specular.IsConnected()
			|| Output->Roughness.IsConnected()
			|| Output->Normal.IsConnected()
			|| Output->Mask.IsConnected());
}

bool FDevKitRVTMeshDecalService::MaterialWritesWorldHeightToRuntimeVirtualTexture(
	const UMaterialInterface* Material)
{
	const UMaterialExpressionRuntimeVirtualTextureOutput* Output = FindRuntimeVirtualTextureOutput(Material);
	return Output && Output->WorldHeight.IsConnected();
}

bool FDevKitRVTMeshDecalService::IsSurfaceRuntimeVirtualTexture(
	const URuntimeVirtualTexture* RuntimeVirtualTexture)
{
	return IsSurfaceRVT(RuntimeVirtualTexture);
}

bool FDevKitRVTMeshDecalService::IsWorldHeightRuntimeVirtualTexture(
	const URuntimeVirtualTexture* RuntimeVirtualTexture)
{
	return IsWorldHeightRVT(RuntimeVirtualTexture);
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
