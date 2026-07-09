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
#include "RuntimeVirtualTextureSetBounds.h"

#define LOCTEXT_NAMESPACE "DevKitLevelRVTService"

namespace
{
	const FString LevelAssetFolderName = TEXT("LevelAsset");
	const FString BakeInfoFolderName = TEXT("BakeInfo");
	const FString GroundRVTSuffix = TEXT("_Ground");

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

	FString InferLevelTokenFromBakeInfoFolder(const FString& BakeInfoFolder)
	{
		FString Normalized = BakeInfoFolder;
		Normalized.TrimStartAndEndInline();
		while (Normalized.EndsWith(TEXT("/")))
		{
			Normalized.LeftChopInline(1);
		}

		const FString ParentFolder = FPackageName::GetLongPackagePath(Normalized);
		FString LevelToken = FPackageName::GetLongPackageAssetName(ParentFolder);
		if (LevelToken.IsEmpty())
		{
			LevelToken = FPackageName::GetLongPackageAssetName(Normalized);
		}
		return SanitizeEnvBatchTagToken(LevelToken, TEXT("Level"));
	}

	int32 FindNextGroundBatchSerial(UWorld* World, const FEnvBatchSourceTagSpec& BaseSpec)
	{
		if (!World)
		{
			return 1;
		}

		const FString Prefix = BuildEnvBatchSourceTagPrefix(BaseSpec);
		int32 MaxSerial = 0;
		for (ULevel* Level : World->GetLevels())
		{
			if (!Level)
			{
				continue;
			}

			for (AActor* Actor : Level->Actors)
			{
				if (!Actor)
				{
					continue;
				}

				for (const FName& Tag : Actor->Tags)
				{
					const FString TagString = Tag.ToString();
					if (!TagString.StartsWith(Prefix))
					{
						continue;
					}

					FEnvBatchSourceTagSpec ParsedSpec;
					if (ParseEnvBatchSourceTag(TagString, ParsedSpec))
					{
						MaxSerial = FMath::Max(MaxSerial, ParsedSpec.SerialNumber);
					}
				}
			}
		}

		return MaxSerial + 1;
	}

	FString BuildGroundBatchSourceTag(UWorld* World, const FDevKitLevelRVTPaths& Paths)
	{
		FEnvBatchSourceTagSpec Spec;
		Spec.LevelName = InferLevelTokenFromBakeInfoFolder(Paths.BakeInfoFolder);
		Spec.ActorKind = TEXT("Ground");
		Spec.ProcessingMode = TEXT("Batched");
		Spec.VTCGroup = TEXT("Ground");
		Spec.bHasExplicitVTCGroup = true;
		Spec.SerialNumber = FindNextGroundBatchSerial(World, Spec);
		return BuildEnvBatchSourceTag(Spec);
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

TOptional<FDevKitLevelRVTPaths> FDevKitLevelRVTService::BuildPaths(const FDevKitLevelRVTRequest& Request, FText& OutError)
{
	FDevKitLevelRVTPaths Paths;
	Paths.BakeInfoFolder = NormalizeRootFolder(Request.BakeInfoFolder);
	Paths.RuntimeVirtualTextureName = Request.RuntimeVirtualTextureName;
	Paths.RuntimeVirtualTextureName.TrimStartAndEndInline();

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

FDevKitLevelRVTResult FDevKitLevelRVTService::CreateGroundRVTForSelection(UWorld* World, const FDevKitLevelRVTRequest& Request)
{
	FText Error;
	TOptional<FDevKitLevelRVTPaths> Paths = BuildPaths(Request, Error);
	if (!Paths.IsSet())
	{
		return FDevKitLevelRVTResult{false, Error, FDevKitLevelRVTPaths()};
	}

	if (!World)
	{
		return FDevKitLevelRVTResult{false, LOCTEXT("NoWorld", "当前没有可用的编辑器世界。"), Paths.GetValue()};
	}

	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		return FDevKitLevelRVTResult{false, LOCTEXT("NoSelection", "请先选择需要写入 RVT 的地面 Actor。"), Paths.GetValue()};
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents = CollectPrimitiveComponents(Actors);
	if (PrimitiveComponents.IsEmpty())
	{
		return FDevKitLevelRVTResult{false, LOCTEXT("NoPrimitiveComponents", "选中 Actor 中没有可写入 RVT 的 PrimitiveComponent。"), Paths.GetValue(), Actors.Num()};
	}

	if (!EnsureContentFolder(Paths->BakeInfoFolder))
	{
		return FDevKitLevelRVTResult{
			false,
			FText::Format(LOCTEXT("CreateBakeInfoFolderFailed", "创建 BakeInfo 文件夹失败：{0}"), FText::FromString(Paths->BakeInfoFolder)),
			Paths.GetValue(),
			Actors.Num(),
			PrimitiveComponents.Num()
		};
	}

	URuntimeVirtualTexture* RuntimeVirtualTexture = CreateOrLoadRuntimeVirtualTexture(Paths.GetValue(), Error);
	if (!RuntimeVirtualTexture)
	{
		return FDevKitLevelRVTResult{false, Error, Paths.GetValue(), Actors.Num(), PrimitiveComponents.Num()};
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateGroundRVTTransaction", "创建关卡地面 RVT"));
	const FString GroundBatchSourceTag = BuildGroundBatchSourceTag(World, Paths.GetValue());
	const int32 TaggedActorCount = ApplyGroundBatchSourceTag(Actors, GroundBatchSourceTag);

	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		Component->Modify();
		Component->RuntimeVirtualTextures.AddUnique(RuntimeVirtualTexture);
		Component->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;
		Component->MarkRenderStateDirty();
		Component->MarkPackageDirty();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = MakeUniqueObjectName(World->GetCurrentLevel(), ARuntimeVirtualTextureVolume::StaticClass(), FName(*Paths->VolumeActorName));
	SpawnParams.OverrideLevel = World->GetCurrentLevel();
	SpawnParams.ObjectFlags = RF_Transactional;

	ARuntimeVirtualTextureVolume* Volume = World->SpawnActor<ARuntimeVirtualTextureVolume>(SpawnParams);
	if (!Volume)
	{
		return FDevKitLevelRVTResult{
			false,
			LOCTEXT("SpawnVolumeFailed", "创建 Runtime Virtual Texture Volume 失败。"),
			Paths.GetValue(),
			Actors.Num(),
			PrimitiveComponents.Num(),
			RuntimeVirtualTexture
		};
	}

	Volume->Modify();
	Volume->SetActorLabel(Paths->VolumeActorName);
	if (Volume->VirtualTextureComponent)
	{
		Volume->VirtualTextureComponent->SetVirtualTexture(RuntimeVirtualTexture);
		RuntimeVirtualTexture::SetBounds(Volume->VirtualTextureComponent);
	}
	Volume->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave{RuntimeVirtualTexture->GetPackage()};
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	FDevKitLevelRVTResult Result;
	Result.bSuccess = true;
	Result.Message = FText::Format(
		LOCTEXT("CreateRVTSuccess", "已创建/绑定 {0}，处理 {1} 个 Actor、{2} 个组件，并写入地面合批 Tag：{3}。请确认材质包含 RVT Output/Sample，并保存关卡。"),
		FText::FromString(Paths->RuntimeVirtualTexturePackage),
		FText::AsNumber(Actors.Num()),
		FText::AsNumber(PrimitiveComponents.Num()),
		FText::FromString(GroundBatchSourceTag));
	Result.Paths = Paths.GetValue();
	Result.ActorCount = Actors.Num();
	Result.PrimitiveComponentCount = PrimitiveComponents.Num();
	Result.RuntimeVirtualTexture = RuntimeVirtualTexture;
	Result.Volume = Volume;
	Result.GroundBatchSourceTag = GroundBatchSourceTag;
	ensureMsgf(
		TaggedActorCount == Actors.Num(),
		TEXT("Ground RVT tagged %d of %d actors."),
		TaggedActorCount,
		Actors.Num());
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

	SetEnumProperty(RuntimeVirtualTexture, TEXT("MaterialType"), ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Roughness);
	RuntimeVirtualTexture->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(RuntimeVirtualTexture);
	return RuntimeVirtualTexture;
}

#undef LOCTEXT_NAMESPACE
