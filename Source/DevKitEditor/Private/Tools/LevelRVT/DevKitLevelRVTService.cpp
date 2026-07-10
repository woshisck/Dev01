#include "Tools/LevelRVT/DevKitLevelRVTService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/PrimitiveComponent.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Tools/EnvBatchSourceTagRules.h"
#include "UObject/UnrealType.h"
#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureEnum.h"
#include "VT/RuntimeVirtualTextureVolume.h"

#define LOCTEXT_NAMESPACE "DevKitLevelRVTService"

namespace
{
	const FString LevelAssetFolderName = TEXT("LevelAsset");
	const FString BakeInfoFolderName = TEXT("BakeInfo");
	const FString GroundRVTSuffix = TEXT("_Ground");
	const FString DataBakeSuffix = TEXT("_DataBake");

	bool EnsureContentFolder(const FString& LongPackagePath)
	{
		const FString FolderFilename = FPackageName::LongPackageNameToFilename(LongPackagePath);
		return IFileManager::Get().MakeDirectory(*FolderFilename, true);
	}

	bool SetObjectProperty(UObject* Object, const FName PropertyName, UObject* Value)
	{
		if (!Object)
		{
			return false;
		}

		if (FObjectProperty* Property = FindFProperty<FObjectProperty>(Object->GetClass(), PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Object, Value);
			return true;
		}
		return false;
	}

	bool SetBoolProperty(UObject* Object, const FName PropertyName, bool bValue)
	{
		if (!Object)
		{
			return false;
		}

		if (FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
		{
			Property->SetPropertyValue_InContainer(Object, bValue);
			return true;
		}
		return false;
	}

	FString NormalizePackageFolder(FString PackagePath);

	FString InferDataBakeLevelPackageFromBakeInfoFolder(const FString& BakeInfoFolder)
	{
		FString Normalized = NormalizePackageFolder(BakeInfoFolder);
		const FString LevelFolder = FPackageName::GetLongPackagePath(Normalized);
		const FString LevelName = FPackageName::GetLongPackageAssetName(LevelFolder);
		return LevelFolder / LevelAssetFolderName / FString::Printf(TEXT("%s%s"), *LevelName, *DataBakeSuffix);
	}

	FString StripKnownSublevelSuffix(FString LevelName)
	{
		static const TCHAR* KnownSuffixes[] =
		{
			TEXT("_Art"),
			TEXT("_Batched"),
			TEXT("_DataBake"),
			TEXT("_Gameplay"),
			TEXT("_Light"),
			TEXT("_PLA")
		};

		for (const TCHAR* Suffix : KnownSuffixes)
		{
			if (LevelName.EndsWith(Suffix, ESearchCase::IgnoreCase))
			{
				LevelName.LeftChopInline(FCString::Strlen(Suffix));
				break;
			}
		}

		return LevelName;
	}

	FString NormalizePackageFolder(FString PackagePath)
	{
		PackagePath.TrimStartAndEndInline();
		while (PackagePath.EndsWith(TEXT("/")))
		{
			PackagePath.LeftChopInline(1);
		}
		return PackagePath;
	}

	FString InferLevelAssetFolderFromWorldPackage(const FString& WorldPackagePath)
	{
		FString Normalized = NormalizePackageFolder(WorldPackagePath);
		const FString LevelAssetToken = FString::Printf(TEXT("/%s/"), *LevelAssetFolderName);
		const int32 LevelAssetIndex = Normalized.Find(LevelAssetToken, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		if (LevelAssetIndex != INDEX_NONE)
		{
			return Normalized.Left(LevelAssetIndex) / LevelAssetFolderName;
		}

		int32 LastSlashIndex = INDEX_NONE;
		if (Normalized.FindLastChar(TEXT('/'), LastSlashIndex))
		{
			return Normalized.Left(LastSlashIndex);
		}

		return TEXT("/Game");
	}

	bool ValidateGroundBatchSourceTag(const FString& SourceTag, FText& OutError)
	{
		FEnvBatchSourceTagSpec Spec;
		if (!ParseEnvBatchSourceTag(SourceTag, Spec))
		{
			OutError = FText::Format(
				LOCTEXT("InvalidGroundBatchSourceTag", "地面合批 Source Tag 无效：{0}"),
				FText::FromString(SourceTag));
			return false;
		}

		if (Spec.ActorKind != TEXT("Ground") || Spec.ProcessingMode != TEXT("Batched"))
		{
			OutError = FText::Format(
				LOCTEXT("GroundBatchSourceTagRequired", "RVT 地面 Source Tag 必须是 Ground.Batched：{0}"),
				FText::FromString(SourceTag));
			return false;
		}

		if (Spec.bHasExplicitVTCGroup)
		{
			OutError = FText::Format(
				LOCTEXT("GroundBatchSourceTagMustOmitVTC", "RVT 地面 Source Tag 不需要贴图集合组段，请使用 EnvBatch.Source.<关卡名>.Ground.Batched.<流水号>：{0}"),
				FText::FromString(SourceTag));
			return false;
		}

		return true;
	}

	ULevel* FindLoadedDataBakeLevel(UWorld* World, const FString& DataBakeLevelPackage)
	{
		if (!World)
		{
			return nullptr;
		}

		for (ULevel* Level : World->GetLevels())
		{
			if (!Level || !Level->GetOutermost())
			{
				continue;
			}

			if (Level->GetOutermost()->GetName().Equals(DataBakeLevelPackage, ESearchCase::IgnoreCase))
			{
				return Level;
			}
		}

		return nullptr;
	}

	int32 ApplyGroundBatchSourceTag(const TArray<AActor*>& Actors, const FString& SourceTag)
	{
		int32 TaggedActorCount = 0;
		for (AActor* Actor : Actors)
		{
			if (!Actor)
			{
				continue;
			}

			Actor->Modify();
			Actor->Tags.RemoveAll([](const FName& ExistingTag)
			{
				return IsEnvBatchSourceTagString(ExistingTag.ToString());
			});
			Actor->Tags.AddUnique(FName(*SourceTag));
			Actor->MarkPackageDirty();
			++TaggedActorCount;
		}
		return TaggedActorCount;
	}

	template <typename EnumType>
	bool SetEnumProperty(UObject* Object, const FName PropertyName, EnumType Value)
	{
		if (!Object)
		{
			return false;
		}

		if (FEnumProperty* Property = FindFProperty<FEnumProperty>(Object->GetClass(), PropertyName))
		{
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Object), static_cast<int64>(Value));
			return true;
		}
		if (FByteProperty* ByteProperty = FindFProperty<FByteProperty>(Object->GetClass(), PropertyName))
		{
			ByteProperty->SetPropertyValue_InContainer(Object, static_cast<uint8>(Value));
			return true;
		}
		return false;
	}

	ERuntimeVirtualTextureMaterialType ToMaterialType(EDevKitLevelRVTLayout Layout)
	{
		switch (Layout)
		{
		case EDevKitLevelRVTLayout::Mask4: return ERuntimeVirtualTextureMaterialType::Mask4;
		case EDevKitLevelRVTLayout::BaseColor: return ERuntimeVirtualTextureMaterialType::BaseColor;
		case EDevKitLevelRVTLayout::BaseColorNormalSpecular: return ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular;
		case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular: return ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_YCoCg;
		case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask: return ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_Mask_YCoCg;
		case EDevKitLevelRVTLayout::WorldHeight: return ERuntimeVirtualTextureMaterialType::WorldHeight;
		case EDevKitLevelRVTLayout::Displacement: return ERuntimeVirtualTextureMaterialType::Displacement;
		case EDevKitLevelRVTLayout::BaseColorNormalRoughness:
		default: return ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Roughness;
		}
	}

	FString GetLayoutSuffix(EDevKitLevelRVTLayout Layout)
	{
		switch (Layout)
		{
		case EDevKitLevelRVTLayout::Mask4: return TEXT("Mask4");
		case EDevKitLevelRVTLayout::BaseColor: return TEXT("BaseColor");
		case EDevKitLevelRVTLayout::BaseColorNormalSpecular: return TEXT("BaseColorNormalSpecular");
		case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular: return TEXT("YCoCgBaseColorNormalSpecular");
		case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask: return TEXT("YCoCgBaseColorNormalSpecularMask");
		case EDevKitLevelRVTLayout::WorldHeight: return TEXT("WorldHeight");
		case EDevKitLevelRVTLayout::Displacement: return TEXT("Displacement");
		case EDevKitLevelRVTLayout::BaseColorNormalRoughness:
		default: return TEXT("BaseColorNormalRoughness");
		}
	}
}

FString FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(const FString& WorldPackagePath)
{
	FString Normalized = NormalizeRootFolder(WorldPackagePath);
	const FString LevelAssetToken = FString::Printf(TEXT("/%s/"), *LevelAssetFolderName);
	const int32 LevelAssetIndex = Normalized.Find(LevelAssetToken, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (LevelAssetIndex != INDEX_NONE)
	{
		return Normalized.Left(LevelAssetIndex) / BakeInfoFolderName;
	}

	int32 LastSlashIndex = INDEX_NONE;
	if (Normalized.FindLastChar(TEXT('/'), LastSlashIndex))
	{
		return Normalized.Left(LastSlashIndex) / BakeInfoFolderName;
	}

	return FString(TEXT("/Game")) / BakeInfoFolderName;
}

FString FDevKitLevelRVTService::InferDataBakeLevelPackageFromWorldPackage(const FString& WorldPackagePath)
{
	const FString LevelAssetFolder = InferLevelAssetFolderFromWorldPackage(WorldPackagePath);
	FString LevelName = FPackageName::GetLongPackageAssetName(NormalizeRootFolder(WorldPackagePath));
	if (LevelName.IsEmpty())
	{
		LevelName = TEXT("Level");
	}

	LevelName = StripKnownSublevelSuffix(LevelName);
	return LevelAssetFolder / FString::Printf(TEXT("%s%s"), *LevelName, *DataBakeSuffix);
}

FString FDevKitLevelRVTService::BuildDefaultGroundRVTNameFromWorldPackage(const FString& WorldPackagePath)
{
	const FString BakeInfoFolder = InferBakeInfoFolderFromWorldPackage(WorldPackagePath);
	FString MapFolderName = FPackageName::GetLongPackageAssetName(FPackageName::GetLongPackagePath(BakeInfoFolder));
	if (MapFolderName.IsEmpty())
	{
		MapFolderName = TEXT("Level");
	}
	return FString::Printf(TEXT("RVT_%s%s"), *MapFolderName, *GroundRVTSuffix);
}

FString FDevKitLevelRVTService::GetLayoutDisplayName(EDevKitLevelRVTLayout Layout)
{
	switch (Layout)
	{
	case EDevKitLevelRVTLayout::Mask4: return TEXT("Mask4");
	case EDevKitLevelRVTLayout::BaseColor: return TEXT("Base Color");
	case EDevKitLevelRVTLayout::BaseColorNormalSpecular: return TEXT("Base Color + Normal + Roughness + Specular");
	case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular: return TEXT("YCoCg Base Color + Normal + Roughness + Specular");
	case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask: return TEXT("YCoCg Base Color + Normal + Roughness + Specular + Mask");
	case EDevKitLevelRVTLayout::WorldHeight: return TEXT("World Height");
	case EDevKitLevelRVTLayout::Displacement: return TEXT("Displacement");
	case EDevKitLevelRVTLayout::BaseColorNormalRoughness:
	default: return TEXT("Base Color + Normal + Roughness");
	}
}

FString FDevKitLevelRVTService::BuildAssetNameForLayout(const FString& BaseName, EDevKitLevelRVTLayout Layout)
{
	return Layout == EDevKitLevelRVTLayout::BaseColorNormalRoughness
		? BaseName
		: FString::Printf(TEXT("%s_%s"), *BaseName, *GetLayoutSuffix(Layout));
}

TOptional<FDevKitLevelRVTPaths> FDevKitLevelRVTService::BuildPaths(const FDevKitLevelRVTRequest& Request, FText& OutError)
{
	return BuildPaths(Request, EDevKitLevelRVTLayout::BaseColorNormalRoughness, OutError);
}

TOptional<FDevKitLevelRVTPaths> FDevKitLevelRVTService::BuildPaths(const FDevKitLevelRVTRequest& Request, EDevKitLevelRVTLayout Layout, FText& OutError)
{
	FDevKitLevelRVTPaths Paths;
	Paths.BakeInfoFolder = NormalizeRootFolder(Request.BakeInfoFolder);
	Paths.RuntimeVirtualTextureName = BuildAssetNameForLayout(Request.RuntimeVirtualTextureName, Layout);
	Paths.RuntimeVirtualTextureName.TrimStartAndEndInline();
	Paths.Layout = Layout;

	if (!FPackageName::IsValidLongPackageName(Paths.BakeInfoFolder, true))
	{
		OutError = LOCTEXT("InvalidBakeInfoFolder", "BakeInfo 目录必须是有效内容路径，例如 /Game/Art/Map/Map_Data/LevelName/BakeInfo。");
		return TOptional<FDevKitLevelRVTPaths>();
	}

	if (!IsValidNameToken(Paths.RuntimeVirtualTextureName))
	{
		OutError = LOCTEXT("InvalidRVTName", "RVT 资产名称只能使用字母、数字和下划线。");
		return TOptional<FDevKitLevelRVTPaths>();
	}

	Paths.RuntimeVirtualTexturePackage = Paths.BakeInfoFolder / Paths.RuntimeVirtualTextureName;
	Paths.DataBakeLevelPackage = InferDataBakeLevelPackageFromBakeInfoFolder(Paths.BakeInfoFolder);
	Paths.VolumeActorName = FString::Printf(TEXT("%s_Volume"), *Paths.RuntimeVirtualTextureName);
	OutError = FText::GetEmpty();
	return Paths;
}

TArray<AActor*> FDevKitLevelRVTService::GetSelectedActors()
{
	TArray<AActor*> Actors;
	if (!GEditor)
	{
		return Actors;
	}

	USelection* Selection = GEditor->GetSelectedActors();
	if (!Selection)
	{
		return Actors;
	}

	for (FSelectionIterator It(*Selection); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			Actors.Add(Actor);
		}
	}
	return Actors;
}

TArray<AActor*> FDevKitLevelRVTService::GetActorsWithGroundBatchSourceTag(UWorld* World, const FString& SourceTag)
{
	TArray<AActor*> Actors;
	if (!World || SourceTag.IsEmpty())
	{
		return Actors;
	}

	const FName SourceTagName(*SourceTag);
	for (ULevel* Level : World->GetLevels())
	{
		if (!Level)
		{
			continue;
		}

		for (AActor* Actor : Level->Actors)
		{
			if (Actor && Actor->Tags.Contains(SourceTagName))
			{
				Actors.Add(Actor);
			}
		}
	}
	return Actors;
}

FDevKitLevelRVTBatchStats FDevKitLevelRVTService::GetGroundBatchStats(UWorld* World, const FString& SourceTag)
{
	FDevKitLevelRVTBatchStats Stats;
	const TArray<AActor*> Actors = GetActorsWithGroundBatchSourceTag(World, SourceTag);
	const TArray<UPrimitiveComponent*> Components = CollectPrimitiveComponents(Actors);
	Stats.ActorCount = Actors.Num();
	Stats.PrimitiveComponentCount = Components.Num();

	for (const UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const int32 BindingCount = Component->RuntimeVirtualTextures.Num();
		if (BindingCount > 0)
		{
			++Stats.BoundComponentCount;
			Stats.RuntimeVirtualTextureReferenceCount += BindingCount;
		}
	}
	return Stats;
}

FDevKitLevelRVTAssetState FDevKitLevelRVTService::GetRVTAssetState(UWorld* World, const FDevKitLevelRVTRequest& Request)
{
	FDevKitLevelRVTAssetState State;
	State.RequestedLayoutCount = Request.Layouts.Num();
	if (!World || Request.Layouts.IsEmpty())
	{
		return State;
	}

	ULevel* DataBakeLevel = nullptr;
	for (const EDevKitLevelRVTLayout Layout : Request.Layouts)
	{
		FText Error;
		const TOptional<FDevKitLevelRVTPaths> Paths = BuildPaths(Request, Layout, Error);
		if (!Paths.IsSet())
		{
			continue;
		}

		if (FPackageName::DoesPackageExist(Paths->RuntimeVirtualTexturePackage))
		{
			++State.ExistingAssetCount;
		}

		if (!DataBakeLevel)
		{
			DataBakeLevel = FindLoadedDataBakeLevel(World, Paths->DataBakeLevelPackage);
			State.bDataBakeLevelLoaded = DataBakeLevel != nullptr;
		}
		if (!DataBakeLevel)
		{
			continue;
		}

		const bool bVolumeExists = DataBakeLevel->Actors.ContainsByPredicate([&Paths](const AActor* Actor)
		{
			const ARuntimeVirtualTextureVolume* Volume = Cast<ARuntimeVirtualTextureVolume>(Actor);
			return Volume && Volume->GetActorLabel() == Paths->VolumeActorName;
		});
		if (bVolumeExists)
		{
			++State.ExistingVolumeCount;
		}
	}
	return State;
}

FDevKitLevelRVTResult FDevKitLevelRVTService::AssignGroundBatchSourceTagToSelection(UWorld* World, const FString& SourceTag)
{
	FDevKitLevelRVTResult Result;
	Result.GroundBatchSourceTag = SourceTag;
	if (!World)
	{
		Result.Message = LOCTEXT("NoWorldAssignTag", "当前没有可用的编辑器世界。");
		return Result;
	}

	FText Error;
	if (!ValidateGroundBatchSourceTag(SourceTag, Error))
	{
		Result.Message = Error;
		return Result;
	}

	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		Result.Message = LOCTEXT("NoSelectionAssignTag", "请先在场景中选择需要归入该地面批次的 Actor。");
		return Result;
	}

	const FScopedTransaction Transaction(LOCTEXT("AssignGroundBatchTagTransaction", "写入地面合批 Source Tag"));
	ApplyGroundBatchSourceTag(Actors, SourceTag);
	Result.bSuccess = true;
	Result.ActorCount = Actors.Num();
	Result.PrimitiveComponentCount = CollectPrimitiveComponents(Actors).Num();
	Result.Message = FText::Format(
		LOCTEXT("AssignGroundBatchTagSuccess", "已为 {0} 个 Actor 写入地面批次 Tag：{1}。后续 RVT 操作会直接处理该 Tag 下的全部模型。"),
		FText::AsNumber(Result.ActorCount),
		FText::FromString(SourceTag));
	return Result;
}

FDevKitLevelRVTResult FDevKitLevelRVTService::RemoveGroundBatchSourceTagFromSelection(UWorld* World, const FString& SourceTag)
{
	FDevKitLevelRVTResult Result;
	Result.GroundBatchSourceTag = SourceTag;
	if (!World)
	{
		Result.Message = LOCTEXT("NoWorldRemoveTag", "当前没有可用的编辑器世界。");
		return Result;
	}

	FText Error;
	if (!ValidateGroundBatchSourceTag(SourceTag, Error))
	{
		Result.Message = Error;
		return Result;
	}

	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		Result.Message = LOCTEXT("NoSelectionRemoveTag", "请先选择需要清除 Tag 的地面 Actor，或先点击列表中的批次以选中整批模型。");
		return Result;
	}

	const FName SourceTagName(*SourceTag);
	const FScopedTransaction Transaction(LOCTEXT("RemoveGroundBatchTagTransaction", "清除地面合批 Source Tag"));
	int32 ChangedActorCount = 0;
	for (AActor* Actor : Actors)
	{
		if (!Actor || !Actor->Tags.Contains(SourceTagName))
		{
			continue;
		}

		Actor->Modify();
		Actor->Tags.Remove(SourceTagName);
		Actor->MarkPackageDirty();
		++ChangedActorCount;
	}

	Result.bSuccess = true;
	Result.ActorCount = ChangedActorCount;
	Result.Message = ChangedActorCount > 0
		? FText::Format(
			LOCTEXT("RemoveGroundBatchTagSuccess", "已从所选 Actor 中清除 Tag：{0}。共修改 {1} 个 Actor；RVT 写入、资产和 Volume 均未改变。"),
			FText::FromString(SourceTag),
			FText::AsNumber(ChangedActorCount))
		: FText::Format(
			LOCTEXT("RemoveGroundBatchTagNoMatch", "所选 Actor 中没有 Tag：{0}，未做修改。"),
			FText::FromString(SourceTag));
	return Result;
}

FDevKitLevelRVTResult FDevKitLevelRVTService::CreateGroundRVTAssetsAndVolumes(UWorld* World, const FDevKitLevelRVTRequest& Request)
{
	FDevKitLevelRVTResult Result;
	FText Error;
	const TArray<EDevKitLevelRVTLayout> Layouts = Request.Layouts.IsEmpty()
		? TArray<EDevKitLevelRVTLayout>{EDevKitLevelRVTLayout::BaseColorNormalRoughness}
		: Request.Layouts;
	const TOptional<FDevKitLevelRVTPaths> Paths = BuildPaths(Request, Layouts[0], Error);
	if (!Paths.IsSet())
	{
		Result.Message = Error;
		return Result;
	}
	Result.Paths = Paths.GetValue();
	Result.GroundBatchSourceTag = Request.GroundBatchSourceTag;

	if (!World)
	{
		Result.Message = LOCTEXT("NoWorld", "当前没有可用的编辑器世界。");
		return Result;
	}

	if (!ValidateGroundBatchSourceTag(Request.GroundBatchSourceTag, Error))
	{
		Result.Message = Error;
		return Result;
	}

	const TArray<AActor*> Actors = GetActorsWithGroundBatchSourceTag(World, Request.GroundBatchSourceTag);
	if (Actors.IsEmpty())
	{
		Result.Message = LOCTEXT("NoTaggedGroundActors", "当前场景中没有匹配该 Ground.Batched Tag 的 Actor。请先在步骤 1 写入或选择有效 Tag。");
		return Result;
	}

	const TArray<UPrimitiveComponent*> PrimitiveComponents = CollectPrimitiveComponents(Actors);
	if (PrimitiveComponents.IsEmpty())
	{
		Result.Message = LOCTEXT("NoTaggedPrimitiveComponents", "该 Tag 批次中没有可用于计算 RVT Bounds 的 PrimitiveComponent。");
		return Result;
	}
	Result.ActorCount = Actors.Num();
	Result.PrimitiveComponentCount = PrimitiveComponents.Num();

	ULevel* DataBakeLevel = FindLoadedDataBakeLevel(World, Paths->DataBakeLevelPackage);
	if (!DataBakeLevel)
	{
		Result.Message = FText::Format(
			LOCTEXT("DataBakeLevelNotLoaded", "未找到已加载的 DataBake 子关卡：{0}。请先加载该子关卡，再创建 RVT。"),
			FText::FromString(Paths->DataBakeLevelPackage));
		return Result;
	}

	if (!EnsureContentFolder(Paths->BakeInfoFolder))
	{
		Result.Message = FText::Format(LOCTEXT("CreateBakeInfoFolderFailed", "创建 BakeInfo 文件夹失败：{0}"), FText::FromString(Paths->BakeInfoFolder));
		return Result;
	}

	TArray<URuntimeVirtualTexture*> RuntimeVirtualTextures;
	TArray<FDevKitLevelRVTPaths> LayoutPaths;
	for (const EDevKitLevelRVTLayout Layout : Layouts)
	{
		const TOptional<FDevKitLevelRVTPaths> LayoutPath = BuildPaths(Request, Layout, Error);
		if (!LayoutPath.IsSet())
		{
			Result.Message = Error;
			return Result;
		}

		URuntimeVirtualTexture* RuntimeVirtualTexture = CreateOrLoadRuntimeVirtualTexture(LayoutPath.GetValue(), Error);
		if (!RuntimeVirtualTexture)
		{
			Result.Paths = LayoutPath.GetValue();
			Result.Message = Error;
			return Result;
		}

		RuntimeVirtualTextures.Add(RuntimeVirtualTexture);
		LayoutPaths.Add(LayoutPath.GetValue());
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateGroundRVTTransaction", "创建关卡地面 RVT 资产与 Volume"));
	TArray<ARuntimeVirtualTextureVolume*> Volumes;
	for (int32 Index = 0; Index < RuntimeVirtualTextures.Num(); ++Index)
	{
		ARuntimeVirtualTextureVolume* Volume = FindOrCreateVolume(World, DataBakeLevel, LayoutPaths[Index], RuntimeVirtualTextures[Index], PrimitiveComponents, Error);
		if (!Volume)
		{
			Result.Paths = LayoutPaths[Index];
			Result.Message = Error;
			return Result;
		}
		Volumes.Add(Volume);
	}

	TArray<UPackage*> PackagesToSave;
	for (URuntimeVirtualTexture* RuntimeVirtualTexture : RuntimeVirtualTextures)
	{
		PackagesToSave.AddUnique(RuntimeVirtualTexture->GetPackage());
	}
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	Result.bSuccess = true;
	Result.Message = FText::Format(
		LOCTEXT("CreateRVTSuccess", "已创建/更新 {0} 个 RVT 资产与 Volume，Bounds 来自 Tag 批次的 {1} 个 Actor、{2} 个组件，Volume 位于 {3}。本步骤没有修改模型写入；请在步骤 3 添加 RVT。"),
		FText::AsNumber(RuntimeVirtualTextures.Num()),
		FText::AsNumber(Actors.Num()),
		FText::AsNumber(PrimitiveComponents.Num()),
		FText::FromString(Paths->DataBakeLevelPackage));
	Result.RuntimeVirtualTextureCount = RuntimeVirtualTextures.Num();
	Result.RuntimeVirtualTexture = RuntimeVirtualTextures[0];
	Result.Volume = Volumes[0];
	return Result;
}

FDevKitLevelRVTResult FDevKitLevelRVTService::AddGroundRVTBindings(UWorld* World, const FDevKitLevelRVTRequest& Request)
{
	FDevKitLevelRVTResult Result;
	FText Error;
	const TArray<EDevKitLevelRVTLayout> Layouts = Request.Layouts.IsEmpty()
		? TArray<EDevKitLevelRVTLayout>{EDevKitLevelRVTLayout::BaseColorNormalRoughness}
		: Request.Layouts;
	const TOptional<FDevKitLevelRVTPaths> FirstPaths = BuildPaths(Request, Layouts[0], Error);
	if (!FirstPaths.IsSet())
	{
		Result.Message = Error;
		return Result;
	}
	Result.Paths = FirstPaths.GetValue();
	Result.GroundBatchSourceTag = Request.GroundBatchSourceTag;
	if (!World)
	{
		Result.Message = LOCTEXT("NoWorldAdd", "当前没有可用的编辑器世界。");
		return Result;
	}
	if (!ValidateGroundBatchSourceTag(Request.GroundBatchSourceTag, Error))
	{
		Result.Message = Error;
		return Result;
	}

	const TArray<AActor*> Actors = GetActorsWithGroundBatchSourceTag(World, Request.GroundBatchSourceTag);
	if (Actors.IsEmpty())
	{
		Result.Message = LOCTEXT("NoTaggedActorsAdd", "该 Ground.Batched Tag 下没有 Actor，无法添加 RVT 写入。");
		return Result;
	}
	Result.ActorCount = Actors.Num();

	TArray<URuntimeVirtualTexture*> RuntimeVirtualTextures;
	TArray<FString> MissingLayouts;
	for (const EDevKitLevelRVTLayout Layout : Layouts)
	{
		const TOptional<FDevKitLevelRVTPaths> Paths = BuildPaths(Request, Layout, Error);
		if (!Paths.IsSet())
		{
			Result.Message = Error;
			return Result;
		}
		if (URuntimeVirtualTexture* RuntimeVirtualTexture = LoadRuntimeVirtualTexture(Paths.GetValue()))
		{
			RuntimeVirtualTextures.Add(RuntimeVirtualTexture);
		}
		else
		{
			MissingLayouts.Add(GetLayoutDisplayName(Layout));
		}
	}
	if (!MissingLayouts.IsEmpty())
	{
		Result.Message = FText::Format(
			LOCTEXT("MissingRVTAssetsAdd", "以下 RVT 资产尚未创建：{0}。请先执行步骤 2。"),
			FText::FromString(FString::Join(MissingLayouts, TEXT(", "))));
		return Result;
	}

	const TArray<UPrimitiveComponent*> Components = CollectPrimitiveComponents(Actors);
	Result.PrimitiveComponentCount = Components.Num();
	const FScopedTransaction Transaction(LOCTEXT("AddGroundRVTBindingsTransaction", "添加地面 RVT 写入"));
	int32 ChangedComponentCount = 0;
	for (UPrimitiveComponent* Component : Components)
	{
		Component->Modify();
		bool bChanged = false;
		for (URuntimeVirtualTexture* RuntimeVirtualTexture : RuntimeVirtualTextures)
		{
			if (!Component->RuntimeVirtualTextures.Contains(RuntimeVirtualTexture))
			{
				Component->RuntimeVirtualTextures.Add(RuntimeVirtualTexture);
				bChanged = true;
			}
		}
		if (Component->VirtualTextureRenderPassType != ERuntimeVirtualTextureMainPassType::Exclusive)
		{
			Component->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
			bChanged = true;
		}
		if (bChanged)
		{
			Component->MarkRenderStateDirty();
			Component->MarkPackageDirty();
			++ChangedComponentCount;
		}
	}

	Result.bSuccess = true;
	Result.RuntimeVirtualTextureCount = RuntimeVirtualTextures.Num();
	Result.RuntimeVirtualTexture = RuntimeVirtualTextures[0];
	Result.Message = FText::Format(
		LOCTEXT("AddGroundRVTBindingsSuccess", "已向 Tag 批次的 {0} 个 Actor 添加 {1} 个 RVT 类型；{2} 个组件发生修改。Draw in Main Pass 已设为 From Virtual Texture。"),
		FText::AsNumber(Actors.Num()),
		FText::AsNumber(RuntimeVirtualTextures.Num()),
		FText::AsNumber(ChangedComponentCount));
	return Result;
}

FDevKitLevelRVTResult FDevKitLevelRVTService::RemoveGroundRVTBindings(UWorld* World, const FDevKitLevelRVTRequest& Request)
{
	FDevKitLevelRVTResult Result;
	FText Error;
	const TArray<EDevKitLevelRVTLayout> Layouts = Request.Layouts.IsEmpty()
		? TArray<EDevKitLevelRVTLayout>{EDevKitLevelRVTLayout::BaseColorNormalRoughness}
		: Request.Layouts;
	const TOptional<FDevKitLevelRVTPaths> FirstPaths = BuildPaths(Request, Layouts[0], Error);
	if (!FirstPaths.IsSet())
	{
		Result.Message = Error;
		return Result;
	}
	Result.Paths = FirstPaths.GetValue();
	Result.GroundBatchSourceTag = Request.GroundBatchSourceTag;
	if (!World)
	{
		Result.Message = LOCTEXT("NoWorldRemove", "当前没有可用的编辑器世界。");
		return Result;
	}
	if (!ValidateGroundBatchSourceTag(Request.GroundBatchSourceTag, Error))
	{
		Result.Message = Error;
		return Result;
	}

	const TArray<AActor*> Actors = GetActorsWithGroundBatchSourceTag(World, Request.GroundBatchSourceTag);
	if (Actors.IsEmpty())
	{
		Result.Message = LOCTEXT("NoTaggedActorsRemove", "该 Ground.Batched Tag 下没有 Actor，无法移除 RVT 写入。");
		return Result;
	}
	Result.ActorCount = Actors.Num();

	TArray<URuntimeVirtualTexture*> RuntimeVirtualTextures;
	for (const EDevKitLevelRVTLayout Layout : Layouts)
	{
		const TOptional<FDevKitLevelRVTPaths> Paths = BuildPaths(Request, Layout, Error);
		if (!Paths.IsSet())
		{
			Result.Message = Error;
			return Result;
		}
		if (URuntimeVirtualTexture* RuntimeVirtualTexture = LoadRuntimeVirtualTexture(Paths.GetValue()))
		{
			RuntimeVirtualTextures.Add(RuntimeVirtualTexture);
		}
	}
	if (RuntimeVirtualTextures.IsEmpty())
	{
		Result.Message = LOCTEXT("NoRVTToRemove", "所选类型的 RVT 资产尚不存在，未执行移除。");
		return Result;
	}

	const TArray<UPrimitiveComponent*> Components = CollectPrimitiveComponents(Actors);
	Result.PrimitiveComponentCount = Components.Num();
	const FScopedTransaction Transaction(LOCTEXT("RemoveGroundRVTBindingsTransaction", "移除地面 RVT 写入"));
	int32 ChangedComponentCount = 0;
	for (UPrimitiveComponent* Component : Components)
	{
		const int32 PreviousCount = Component->RuntimeVirtualTextures.Num();
		Component->Modify();
		Component->RuntimeVirtualTextures.RemoveAll([&RuntimeVirtualTextures](const TObjectPtr<URuntimeVirtualTexture>& Candidate)
		{
			return RuntimeVirtualTextures.Contains(Candidate.Get());
		});
		const bool bRemovedBinding = Component->RuntimeVirtualTextures.Num() != PreviousCount;
		if (bRemovedBinding && Component->RuntimeVirtualTextures.IsEmpty())
		{
			Component->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;
		}
		if (bRemovedBinding)
		{
			Component->MarkRenderStateDirty();
			Component->MarkPackageDirty();
			++ChangedComponentCount;
		}
	}

	Result.bSuccess = true;
	Result.RuntimeVirtualTextureCount = RuntimeVirtualTextures.Num();
	Result.RuntimeVirtualTexture = RuntimeVirtualTextures[0];
	Result.Message = FText::Format(
		LOCTEXT("RemoveGroundRVTBindingsSuccess", "已从 Tag 批次的 {0} 个 Actor 移除所选 RVT；{1} 个组件发生修改。组件没有剩余 RVT 时，Draw in Main Pass 已恢复为 Always。"),
		FText::AsNumber(Actors.Num()),
		FText::AsNumber(ChangedComponentCount));
	return Result;
}

FDevKitLevelRVTResult FDevKitLevelRVTService::ClearGroundRVTBindings(UWorld* World, const FDevKitLevelRVTRequest& Request)
{
	FDevKitLevelRVTResult Result;
	Result.GroundBatchSourceTag = Request.GroundBatchSourceTag;
	if (!World)
	{
		Result.Message = LOCTEXT("NoWorldClear", "当前没有可用的编辑器世界。");
		return Result;
	}

	FText Error;
	if (!ValidateGroundBatchSourceTag(Request.GroundBatchSourceTag, Error))
	{
		Result.Message = Error;
		return Result;
	}

	const TArray<AActor*> Actors = GetActorsWithGroundBatchSourceTag(World, Request.GroundBatchSourceTag);
	if (Actors.IsEmpty())
	{
		Result.Message = LOCTEXT("NoTaggedActorsClear", "该 Ground.Batched Tag 下没有 Actor，无法清空 RVT 写入。");
		return Result;
	}
	const TArray<UPrimitiveComponent*> Components = CollectPrimitiveComponents(Actors);
	Result.ActorCount = Actors.Num();
	Result.PrimitiveComponentCount = Components.Num();

	const FScopedTransaction Transaction(LOCTEXT("ClearGroundRVTBindingsTransaction", "清空地面 RVT 写入"));
	int32 ChangedComponentCount = 0;
	int32 RemovedReferenceCount = 0;
	for (UPrimitiveComponent* Component : Components)
	{
		const int32 PreviousCount = Component->RuntimeVirtualTextures.Num();
		const bool bNeedsMainPassRestore = Component->VirtualTextureRenderPassType == ERuntimeVirtualTextureMainPassType::Exclusive;
		if (PreviousCount == 0 && !bNeedsMainPassRestore)
		{
			continue;
		}

		Component->Modify();
		RemovedReferenceCount += PreviousCount;
		Component->RuntimeVirtualTextures.Reset();
		Component->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;
		Component->MarkRenderStateDirty();
		Component->MarkPackageDirty();
		++ChangedComponentCount;
	}

	Result.bSuccess = true;
	Result.Message = FText::Format(
		LOCTEXT("ClearGroundRVTBindingsSuccess", "已清空该 Tag 批次全部 RVT 写入：移除 {0} 个组件引用，修改 {1} 个组件，并将 Draw in Main Pass 恢复为 Always。RVT 资产与 Volume 保留。"),
		FText::AsNumber(RemovedReferenceCount),
		FText::AsNumber(ChangedComponentCount));
	return Result;
}

bool FDevKitLevelRVTService::IsValidNameToken(const FString& Token)
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

FString FDevKitLevelRVTService::NormalizeRootFolder(FString RootFolder)
{
	RootFolder.TrimStartAndEndInline();
	while (RootFolder.EndsWith(TEXT("/")))
	{
		RootFolder.LeftChopInline(1);
	}
	return RootFolder;
}

TArray<UPrimitiveComponent*> FDevKitLevelRVTService::CollectPrimitiveComponents(const TArray<AActor*>& Actors)
{
	TArray<UPrimitiveComponent*> Components;
	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}

		TInlineComponentArray<UPrimitiveComponent*> ActorComponents;
		Actor->GetComponents(ActorComponents);
		for (UPrimitiveComponent* Component : ActorComponents)
		{
			if (Component && Component->IsRegistered())
			{
				Components.AddUnique(Component);
			}
		}
	}
	return Components;
}

URuntimeVirtualTexture* FDevKitLevelRVTService::CreateOrLoadRuntimeVirtualTexture(const FDevKitLevelRVTPaths& Paths, FText& OutError)
{
	if (FPackageName::DoesPackageExist(Paths.RuntimeVirtualTexturePackage))
	{
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *Paths.RuntimeVirtualTexturePackage, *Paths.RuntimeVirtualTextureName);
		URuntimeVirtualTexture* Existing = LoadObject<URuntimeVirtualTexture>(nullptr, *ObjectPath);
		if (!Existing)
		{
			OutError = FText::Format(LOCTEXT("LoadExistingRVTFailed", "已有 RVT 包加载失败：{0}"), FText::FromString(Paths.RuntimeVirtualTexturePackage));
		}
		return Existing;
	}

	UPackage* Package = CreatePackage(*Paths.RuntimeVirtualTexturePackage);
	if (!Package)
	{
		OutError = FText::Format(LOCTEXT("CreateRVTPackageFailed", "创建 RVT 包失败：{0}"), FText::FromString(Paths.RuntimeVirtualTexturePackage));
		return nullptr;
	}

	URuntimeVirtualTexture* RuntimeVirtualTexture = NewObject<URuntimeVirtualTexture>(
		Package,
		URuntimeVirtualTexture::StaticClass(),
		*Paths.RuntimeVirtualTextureName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!RuntimeVirtualTexture)
	{
		OutError = LOCTEXT("CreateRVTFailed", "创建 Runtime Virtual Texture 资产失败。");
		return nullptr;
	}

	SetEnumProperty(RuntimeVirtualTexture, TEXT("MaterialType"), ToMaterialType(Paths.Layout));
	RuntimeVirtualTexture->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(RuntimeVirtualTexture);
	return RuntimeVirtualTexture;
}

URuntimeVirtualTexture* FDevKitLevelRVTService::LoadRuntimeVirtualTexture(const FDevKitLevelRVTPaths& Paths)
{
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *Paths.RuntimeVirtualTexturePackage, *Paths.RuntimeVirtualTextureName);
	return LoadObject<URuntimeVirtualTexture>(nullptr, *ObjectPath);
}

ARuntimeVirtualTextureVolume* FDevKitLevelRVTService::FindOrCreateVolume(UWorld* World, ULevel* DataBakeLevel, const FDevKitLevelRVTPaths& Paths, URuntimeVirtualTexture* RuntimeVirtualTexture, const TArray<UPrimitiveComponent*>& BoundsComponents, FText& OutError)
{
	if (!World || !DataBakeLevel)
	{
		OutError = LOCTEXT("MissingDataBakeLevelVolume", "未找到可写入 RVT Volume 的 DataBake 子关卡。");
		return nullptr;
	}

	ARuntimeVirtualTextureVolume* Volume = nullptr;
	for (AActor* Actor : DataBakeLevel->Actors)
	{
		if (ARuntimeVirtualTextureVolume* Candidate = Cast<ARuntimeVirtualTextureVolume>(Actor); Candidate && Candidate->GetActorLabel() == Paths.VolumeActorName)
		{
			Volume = Candidate;
			break;
		}
	}

	if (!Volume)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = MakeUniqueObjectName(DataBakeLevel, ARuntimeVirtualTextureVolume::StaticClass(), FName(*Paths.VolumeActorName));
		SpawnParams.OverrideLevel = DataBakeLevel;
		SpawnParams.ObjectFlags = RF_Transactional;
		Volume = World->SpawnActor<ARuntimeVirtualTextureVolume>(SpawnParams);
		if (!Volume)
		{
			OutError = LOCTEXT("SpawnVolumeFailed", "创建 Runtime Virtual Texture Volume 失败。");
			return nullptr;
		}
		Volume->SetActorLabel(Paths.VolumeActorName);
	}

	Volume->Modify();
	if (Volume->VirtualTextureComponent)
	{
		Volume->VirtualTextureComponent->SetVirtualTexture(RuntimeVirtualTexture);
		SetBoolProperty(Volume->VirtualTextureComponent, TEXT("bHidePrimitives"), true);

		FBox WorldBounds(ForceInit);
		for (const UPrimitiveComponent* Component : BoundsComponents)
		{
			if (Component)
			{
				WorldBounds += Component->Bounds.GetBox();
			}
		}
		if (!WorldBounds.IsValid)
		{
			OutError = LOCTEXT("InvalidGroundBounds", "该 Tag 批次没有有效的组件 Bounds，无法设置 RVT Volume。");
			return nullptr;
		}

		FVector BoundsSize = WorldBounds.GetSize();
		if (BoundsSize.Z <= UE_KINDA_SMALL_NUMBER)
		{
			WorldBounds = WorldBounds.ExpandBy(FVector(0.0, 0.0, 0.5));
			BoundsSize = WorldBounds.GetSize();
		}
		BoundsSize.X = FMath::Max(BoundsSize.X, 1.0);
		BoundsSize.Y = FMath::Max(BoundsSize.Y, 1.0);
		BoundsSize.Z = FMath::Max(BoundsSize.Z, 1.0);
		Volume->SetActorTransform(FTransform(FQuat::Identity, WorldBounds.Min, BoundsSize));
		Volume->PostEditMove(true);
		Volume->VirtualTextureComponent->MarkRenderStateDirty();
	}
	Volume->MarkPackageDirty();
	return Volume;
}

#undef LOCTEXT_NAMESPACE
