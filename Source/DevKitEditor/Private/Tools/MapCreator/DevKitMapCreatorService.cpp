#include "Tools/MapCreator/DevKitMapCreatorService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorLevelUtils.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#include "Map/YogLevelScript.h"
#include "Map/YogMapDefinition.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingAlwaysLoaded.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "DevKitMapCreatorService"

namespace
{
	const TArray<FString> DungeonLayers{
		TEXT("L1"),
		TEXT("L2"),
		TEXT("L3")
	};

	const TArray<FString> LevelTypes{
		TEXT("CommonLevel"),
		TEXT("EliteLevel"),
		TEXT("BossLevel"),
		TEXT("EventLevel"),
		TEXT("ShopLevel")
	};

	const TArray<FString> SublevelSuffixes{
		TEXT("Art"),
		TEXT("Batched"),
		TEXT("DataBake"),
		TEXT("Gameplay"),
		TEXT("Light"),
		TEXT("PLA")
	};

	const FString TemplateLevelAssetFolder = TEXT("/Game/Art/Map/Templates/LevelAsset");
	const FString PersistentTemplateMap = TemplateLevelAssetFolder / TEXT("L_Temp");

	const TMap<FString, FString> FixedSublevelTemplateMap{
		{TEXT("Art"), TemplateLevelAssetFolder / TEXT("L_Temp_Art")},
		{TEXT("Batched"), TemplateLevelAssetFolder / TEXT("L_Temp_Batched")},
		{TEXT("DataBake"), TemplateLevelAssetFolder / TEXT("L_Temp_DataBake")},
		{TEXT("Gameplay"), TemplateLevelAssetFolder / TEXT("L_Temp_Gameplay")},
		{TEXT("PLA"), TemplateLevelAssetFolder / TEXT("L_Temp_PLA")}
	};

	const TMap<FString, FString> LightTemplateByDungeonLayer{
		{TEXT("L1"), TemplateLevelAssetFolder / TEXT("L_Temp_Light_Dungeon")},
		{TEXT("L2"), TemplateLevelAssetFolder / TEXT("L_Temp_Light_Outside")},
		{TEXT("L3"), TemplateLevelAssetFolder / TEXT("L_Temp_Light_Inside")}
	};

	FString BuildMapObjectPath(const FString& PackagePath)
	{
		return FString::Printf(TEXT("%s.%s"), *PackagePath, *FPackageName::GetLongPackageAssetName(PackagePath));
	}

	bool EnsureContentFolder(const FString& LongPackagePath)
	{
		const FString FolderFilename = FPackageName::LongPackageNameToFilename(LongPackagePath);
		return IFileManager::Get().MakeDirectory(*FolderFilename, true);
	}

	bool ValidatePersistentEditorWorld(UWorld* World, FText& OutError)
	{
		if (!GEditor || !World || !World->PersistentLevel || World->WorldType != EWorldType::Editor
			|| GEditor->GetEditorWorldContext().World() != World)
		{
			OutError = LOCTEXT("SublevelNotEditorWorld", "请先打开目标主关卡，再创建或关联子关卡。");
			return false;
		}
		if (GEditor->IsPlaySessionInProgress())
		{
			OutError = LOCTEXT("SublevelPIERunning", "请先停止 PIE 或 Simulate，再修改子关卡。");
			return false;
		}
		if (World->IsPartitionedWorld())
		{
			OutError = LOCTEXT("SublevelPartitionedWorld", "World Partition 关卡不支持此传统子关卡操作，请使用 Data Layers。");
			return false;
		}
		const FString ParentPath = World->GetOutermost()->GetName();
		if (!ParentPath.StartsWith(TEXT("/Game/")) || !FPackageName::DoesPackageExist(ParentPath))
		{
			OutError = LOCTEXT("SublevelUnsavedWorld", "主关卡必须先保存到 /Game 下；不会自动保存为临时关卡。");
			return false;
		}
		if (World->GetOutermost()->IsDirty())
		{
			OutError = LOCTEXT("SublevelDirtyWorld", "主关卡有未保存修改，请先保存；本操作不会连带保存其他资产。");
			return false;
		}
		return true;
	}

	bool HasStreamingReference(const UWorld* World, const FString& PackagePath)
	{
		for (const ULevelStreaming* Streaming : World->GetStreamingLevels())
		{
			if (Streaming && Streaming->GetWorldAssetPackageName() == PackagePath)
			{
				return true;
			}
		}
		return World->GetOutermost()->GetName() == PackagePath;
	}

	bool SaveScopedPackages(const TArray<UPackage*>& Packages)
	{
		FEditorFileUtils::FPromptForCheckoutAndSaveParams Params;
		Params.bCheckDirty = true;
		Params.bPromptToSave = false;
		Params.bCanBeDeclined = false;
		Params.bIsExplicitSave = true;
		return FEditorFileUtils::PromptForCheckoutAndSave(Packages, Params) == FEditorFileUtils::PR_Success;
	}

	bool CheckoutWritableParent(UWorld* World, FText& OutError)
	{
		if (!FEditorFileUtils::PromptToCheckoutPackages(false, {World->GetOutermost()}))
		{
			OutError = LOCTEXT("ParentCheckoutCancelled", "主关卡签出已取消，未创建或关联子关卡。");
			return false;
		}
		// The prompt's return value only confirms that the user did not cancel.
		// Verify the actual file before creating a child or changing references.
		FString ParentFilename;
		if (!FPackageName::DoesPackageExist(World->GetOutermost()->GetName(), &ParentFilename)
			|| !IFileManager::Get().FileExists(*ParentFilename)
			|| IFileManager::Get().IsReadOnly(*ParentFilename))
		{
			OutError = LOCTEXT("ParentStillReadOnly", "主关卡文件不存在或签出后仍为只读；未创建或关联子关卡。请检查签出结果后重试。");
			return false;
		}
		return true;
	}

	bool SaveCurrentWorldBeforeReplacement()
	{
		UWorld* CurrentWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!CurrentWorld)
		{
			return true;
		}
		TArray<UPackage*> Packages;
		for (ULevel* Level : CurrentWorld->GetLevels())
		{
			if (!Level)
			{
				continue;
			}
			if (Level->GetOutermost()->IsDirty())
			{
				Packages.AddUnique(Level->GetOutermost());
			}
			for (AActor* Actor : Level->Actors)
			{
				UPackage* ExternalPackage = Actor ? Actor->GetExternalPackage() : nullptr;
				if (ExternalPackage && ExternalPackage->IsDirty())
				{
					Packages.AddUnique(ExternalPackage);
				}
			}
		}
		if (Packages.IsEmpty())
		{
			return true;
		}
		FEditorFileUtils::FPromptForCheckoutAndSaveParams Params;
		Params.bCheckDirty = true;
		Params.bPromptToSave = true;
		Params.bCanBeDeclined = false;
		Params.bIsExplicitSave = true;
		Params.Title = LOCTEXT("SaveReplacedWorldTitle", "创建地图包前保存当前关卡");
		Params.Message = LOCTEXT("SaveReplacedWorldMessage", "仅保存当前主关卡、已加载子关卡和外部 Actor 包；不会保存其他内容资产。");
		if (FEditorFileUtils::PromptForCheckoutAndSave(Packages, Params) != FEditorFileUtils::PR_Success)
		{
			return false;
		}
		// An unchecked package must not be discarded by changing the editor world.
		for (UPackage* Package : Packages)
		{
			if (Package && Package->IsDirty())
			{
				return false;
			}
		}
		return true;
	}
}

const TArray<FString>& FDevKitMapCreatorService::GetDungeonLayers()
{
	return DungeonLayers;
}

const TArray<FString>& FDevKitMapCreatorService::GetLevelTypes()
{
	return LevelTypes;
}

const TArray<FString>& FDevKitMapCreatorService::GetSublevelSuffixes()
{
	return SublevelSuffixes;
}

FString FDevKitMapCreatorService::GetPersistentTemplateMapPath()
{
	return PersistentTemplateMap;
}

FString FDevKitMapCreatorService::GetSublevelTemplateMapPath(const FString& SublevelSuffix, const FString& DungeonLayer)
{
	if (SublevelSuffix == TEXT("Light"))
	{
		if (const FString* TemplatePath = LightTemplateByDungeonLayer.Find(DungeonLayer))
		{
			return *TemplatePath;
		}
		return FString();
	}

	if (const FString* TemplatePath = FixedSublevelTemplateMap.Find(SublevelSuffix))
	{
		return *TemplatePath;
	}
	return FString();
}

FString FDevKitMapCreatorService::GetDefaultRootFolder()
{
	return TEXT("/Game/Art/Map/Map_Data");
}

TOptional<FDevKitMapCreatorPaths> FDevKitMapCreatorService::BuildPaths(const FDevKitMapCreatorRequest& Request, FText& OutError)
{
	const FString RootFolder = NormalizeRootFolder(Request.RootFolder);
	if ((RootFolder != TEXT("/Game") && !RootFolder.StartsWith(TEXT("/Game/")))
		|| !FPackageName::IsValidLongPackageName(RootFolder))
	{
		OutError = LOCTEXT("InvalidRootFolder", "保存根目录必须是有效内容路径，例如 /Game/Art/Map/Map_Data。");
		return TOptional<FDevKitMapCreatorPaths>();
	}

	if (!DungeonLayers.Contains(Request.DungeonLayer))
	{
		OutError = LOCTEXT("InvalidDungeonLayer", "地牢层级必须是 L1、L2 或 L3。");
		return TOptional<FDevKitMapCreatorPaths>();
	}

	if (!LevelTypes.Contains(Request.LevelType))
	{
		OutError = LOCTEXT("InvalidLevelType", "地图创建器不支持当前关卡类型。");
		return TOptional<FDevKitMapCreatorPaths>();
	}

	if (!IsValidNameToken(Request.NameSuffix))
	{
		OutError = LOCTEXT("InvalidSuffix", "Map name suffix/后缀只能使用字母、数字和下划线。");
		return TOptional<FDevKitMapCreatorPaths>();
	}

	FDevKitMapCreatorPaths Paths;
	Paths.BaseName = FString::Printf(TEXT("%s_%s_%s"), *Request.DungeonLayer, *Request.LevelType, *Request.NameSuffix);
	Paths.TargetFolder = RootFolder / Paths.BaseName;
	Paths.LevelAssetFolder = Paths.TargetFolder / TEXT("LevelAsset");
	Paths.LevelMaterialFolder = Paths.TargetFolder / TEXT("LevelMaterial");
	Paths.BatchedAssetFolder = Paths.TargetFolder / TEXT("BatchedAsset");
	Paths.BakeInfoFolder = Paths.TargetFolder / TEXT("BakeInfo");
	Paths.PersistentMap = Paths.LevelAssetFolder / Paths.BaseName;
	Paths.MapDefinition = Paths.TargetFolder / FString::Printf(TEXT("DA_%s"), *Paths.BaseName);

	for (const FString& Suffix : SublevelSuffixes)
	{
		Paths.Sublevels.Add(Paths.LevelAssetFolder / FString::Printf(TEXT("%s_%s"), *Paths.BaseName, *Suffix));
	}

	OutError = FText::GetEmpty();
	return Paths;
}

FDevKitMapCreatorResult FDevKitMapCreatorService::CreateLevelStack(const FDevKitMapCreatorRequest& Request)
{
	FText Error;
	TOptional<FDevKitMapCreatorPaths> Paths = BuildPaths(Request, Error);
	if (!Paths.IsSet())
	{
		return FDevKitMapCreatorResult{false, Error, FDevKitMapCreatorPaths()};
	}

	if (!GEditor)
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("NoEditor", "当前没有可用的 Unreal Editor 实例。"), Paths.GetValue()};
	}

	if (GEditor->IsPlaySessionInProgress())
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("PIERunning", "创建地图包前请先停止 PIE 或 Simulate。"), Paths.GetValue()};
	}

	TArray<FString> ExistingPackages;
	ExistingPackages.Reserve(2 + Paths->Sublevels.Num());
	ExistingPackages.Add(Paths->PersistentMap);
	ExistingPackages.Add(Paths->MapDefinition);
	ExistingPackages.Append(Paths->Sublevels);
	for (const FString& PackagePath : ExistingPackages)
	{
		if (FPackageName::DoesPackageExist(PackagePath) || FindPackage(nullptr, *PackagePath))
		{
			return FDevKitMapCreatorResult{
				false,
				FText::Format(LOCTEXT("PackageExists", "资产已存在：{0}"), FText::FromString(PackagePath)),
				Paths.GetValue()
			};
		}
	}

	const FString PersistentTemplatePath = GetPersistentTemplateMapPath();
	if (!FPackageName::DoesPackageExist(PersistentTemplatePath))
	{
		return FDevKitMapCreatorResult{
			false,
			FText::Format(LOCTEXT("PersistentTemplatePackageMissing", "Persistent 关卡模板不存在：{0}"), FText::FromString(PersistentTemplatePath)),
			Paths.GetValue()
		};
	}

	TMap<FString, UWorld*> TemplateWorlds;
	for (const FString& Suffix : SublevelSuffixes)
	{
		const FString TemplatePath = GetSublevelTemplateMapPath(Suffix, Request.DungeonLayer);
		if (TemplatePath.IsEmpty())
		{
			continue;
		}

		if (!FPackageName::DoesPackageExist(TemplatePath))
		{
			return FDevKitMapCreatorResult{
				false,
				FText::Format(LOCTEXT("TemplatePackageMissing", "子关卡模板不存在：{0}"), FText::FromString(TemplatePath)),
				Paths.GetValue()
			};
		}

		UWorld* TemplateWorld = LoadObject<UWorld>(nullptr, *BuildMapObjectPath(TemplatePath));
		if (!TemplateWorld)
		{
			return FDevKitMapCreatorResult{
				false,
				FText::Format(LOCTEXT("TemplateLoadFailed", "子关卡模板加载失败：{0}"), FText::FromString(TemplatePath)),
				Paths.GetValue()
			};
		}

		TemplateWorlds.Add(Suffix, TemplateWorld);
	}

	if (!SaveCurrentWorldBeforeReplacement())
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("SaveCurrentFailed", "地图创建已取消，或当前未保存资产无法保存。"), Paths.GetValue()};
	}

	const TArray<FString> FoldersToCreate{
		Paths->LevelAssetFolder,
		Paths->LevelMaterialFolder,
		Paths->BatchedAssetFolder,
		Paths->BakeInfoFolder
	};
	for (const FString& FolderPath : FoldersToCreate)
	{
		if (!EnsureContentFolder(FolderPath))
		{
			return FDevKitMapCreatorResult{
				false,
				FText::Format(LOCTEXT("CreateFolderFailed", "创建文件夹失败：{0}"), FText::FromString(FolderPath)),
				Paths.GetValue()
			};
		}
	}

	const TSubclassOf<ALevelScriptActor> PreviousLevelScriptClass = GEngine ? GEngine->LevelScriptActorClass : nullptr;
	if (GEngine)
	{
		GEngine->LevelScriptActorClass = AYogLevelScript::StaticClass();
	}

	const FString PersistentTemplateFilename = FPackageName::LongPackageNameToFilename(PersistentTemplatePath, FPackageName::GetMapPackageExtension());
	UWorld* World = UEditorLoadingAndSavingUtils::NewMapFromTemplate(PersistentTemplateFilename, false);
	if (GEngine)
	{
		GEngine->LevelScriptActorClass = PreviousLevelScriptClass;
	}

	if (!World)
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("NewMapFailed", "创建 Persistent 关卡世界失败。"), Paths.GetValue()};
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UDataAssetFactory* DataAssetFactory = NewObject<UDataAssetFactory>();
	DataAssetFactory->DataAssetClass = UYogMapDefinition::StaticClass();

	const FString DataAssetName = FPackageName::GetLongPackageAssetName(Paths->MapDefinition);
	UYogMapDefinition* MapDefinition = Cast<UYogMapDefinition>(AssetTools.CreateAsset(
		DataAssetName,
		Paths->TargetFolder,
		UYogMapDefinition::StaticClass(),
		DataAssetFactory));
	if (!MapDefinition)
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("CreateDAFailed", "创建地图数据资产失败。"), Paths.GetValue()};
	}
	MapDefinition->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MapDefinition);

	if (!UEditorLoadingAndSavingUtils::SaveMap(World, Paths->PersistentMap))
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("SavePersistentFailed", "保存 Persistent 关卡失败。"), Paths.GetValue()};
	}

	for (int32 SublevelIndex = 0; SublevelIndex < Paths->Sublevels.Num(); ++SublevelIndex)
	{
		const FString& SublevelPath = Paths->Sublevels[SublevelIndex];
		const FString& Suffix = SublevelSuffixes[SublevelIndex];
		const FString SublevelFilename = FPackageName::LongPackageNameToFilename(SublevelPath, FPackageName::GetMapPackageExtension());
		UEditorLevelUtils::FCreateNewStreamingLevelForWorldParams CreateParams(ULevelStreamingAlwaysLoaded::StaticClass(), SublevelFilename);
		CreateParams.bUseSaveAs = false;
		if (UWorld** TemplateWorld = TemplateWorlds.Find(Suffix))
		{
			CreateParams.TemplateWorld = *TemplateWorld;
		}

		ULevelStreaming* StreamingLevel = UEditorLevelUtils::CreateNewStreamingLevelForWorld(*World, CreateParams);
		if (!StreamingLevel)
		{
			return FDevKitMapCreatorResult{
				false,
				FText::Format(LOCTEXT("CreateSublevelFailed", "创建子关卡失败：{0}"), FText::FromString(SublevelPath)),
				Paths.GetValue()
			};
		}
		StreamingLevel->SetShouldBeLoaded(true);
		StreamingLevel->SetShouldBeVisible(true);
	}

	if (ALevelScriptActor* LevelScriptActor = World->PersistentLevel ? World->PersistentLevel->GetLevelScriptActor() : nullptr)
	{
		if (FObjectProperty* MapDefinitionProperty = FindFProperty<FObjectProperty>(LevelScriptActor->GetClass(), TEXT("Mapdefinition")))
		{
			MapDefinitionProperty->SetObjectPropertyValue_InContainer(LevelScriptActor, MapDefinition);
			if (UObject* DefaultObject = LevelScriptActor->GetClass()->GetDefaultObject(false))
			{
				MapDefinitionProperty->SetObjectPropertyValue_InContainer(DefaultObject, MapDefinition);
			}
			LevelScriptActor->MarkPackageDirty();
		}
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(MapDefinition->GetPackage());
	PackagesToSave.Add(World->GetPackage());
	for (const FString& SublevelPath : Paths->Sublevels)
	{
		if (UPackage* SublevelPackage = FindPackage(nullptr, *SublevelPath))
		{
			PackagesToSave.Add(SublevelPackage);
		}
	}

	if (!SaveScopedPackages(PackagesToSave))
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("SavePackagesFailed", "资产已创建，但一个或多个包保存失败。"), Paths.GetValue()};
	}

	return FDevKitMapCreatorResult{
		true,
		FText::Format(LOCTEXT("CreateSucceeded", "已创建地图包：{0}"), FText::FromString(Paths->TargetFolder)),
		Paths.GetValue()
	};
}

bool FDevKitMapCreatorService::SaveCurrentEditorWorld()
{
	return SaveCurrentWorldBeforeReplacement();
}

TOptional<FString> FDevKitMapCreatorService::BuildSublevelPath(const FString& PersistentPackagePath,
	const FDevKitSublevelRequest& Request, FText& OutError)
{
	const FString TargetFolder = NormalizeRootFolder(Request.TargetFolder.IsEmpty()
		? FPackageName::GetLongPackagePath(PersistentPackagePath) : Request.TargetFolder);
	if (!PersistentPackagePath.StartsWith(TEXT("/Game/"))
		|| !FPackageName::IsValidLongPackageName(PersistentPackagePath)
		|| (TargetFolder != TEXT("/Game") && !TargetFolder.StartsWith(TEXT("/Game/")))
		|| !FPackageName::IsValidLongPackageName(TargetFolder))
	{
		OutError = LOCTEXT("InvalidSublevelFolder", "主关卡和目标目录必须是有效 /Game 内容路径。");
		return {};
	}
	if (!SublevelSuffixes.Contains(Request.Suffix))
	{
		OutError = LOCTEXT("InvalidSublevelSuffix", "子关卡类型必须为 Art、Batched、DataBake、Gameplay、Light 或 PLA。");
		return {};
	}
	if (!DungeonLayers.Contains(Request.DungeonLayer))
	{
		OutError = LOCTEXT("InvalidSublevelLayer", "请选择 L1、L2 或 L3，用于灯光模板映射。");
		return {};
	}
	const FString PackagePath = TargetFolder / FString::Printf(TEXT("%s_%s"),
		*FPackageName::GetLongPackageAssetName(PersistentPackagePath), *Request.Suffix);
	if (!FPackageName::IsValidLongPackageName(PackagePath))
	{
		OutError = LOCTEXT("InvalidSublevelPath", "生成的子关卡名称无效。");
		return {};
	}
	OutError = FText::GetEmpty();
	return PackagePath;
}

FDevKitSublevelResult FDevKitMapCreatorService::CreateAndAttachSublevel(UWorld* PersistentWorld,
	const FDevKitSublevelRequest& Request)
{
	FText Error;
	if (!ValidatePersistentEditorWorld(PersistentWorld, Error))
	{
		return {false, Error};
	}
	const TOptional<FString> Path = BuildSublevelPath(PersistentWorld->GetOutermost()->GetName(), Request, Error);
	if (!Path.IsSet())
	{
		return {false, Error};
	}
	FDevKitSublevelResult Result;
	Result.PackagePath = Path.GetValue();
	if (FPackageName::DoesPackageExist(Result.PackagePath) || FindPackage(nullptr, *Result.PackagePath)
		|| HasStreamingReference(PersistentWorld, Result.PackagePath))
	{
		Result.Message = FText::Format(LOCTEXT("SublevelAlreadyExists", "子关卡已存在或已被关联，不会覆盖：{0}"),
			FText::FromString(Result.PackagePath));
		return Result;
	}
	const FString TemplatePath = GetSublevelTemplateMapPath(Request.Suffix, Request.DungeonLayer);
	UWorld* TemplateWorld = FPackageName::DoesPackageExist(TemplatePath)
		? LoadObject<UWorld>(nullptr, *BuildMapObjectPath(TemplatePath)) : nullptr;
	if (!TemplateWorld || TemplateWorld->IsPartitionedWorld() || TemplateWorld->GetOutermost()->IsDirty())
	{
		Result.Message = FText::Format(LOCTEXT("SublevelTemplateUnavailable", "模板不存在、为 World Partition 或有未保存修改：{0}"),
			FText::FromString(TemplatePath));
		return Result;
	}
	// Resolve checkout before writing a new file. A cancelled checkout creates nothing.
	if (!CheckoutWritableParent(PersistentWorld, Error))
	{
		Result.Message = Error;
		return Result;
	}
	if (!EnsureContentFolder(FPackageName::GetLongPackagePath(Result.PackagePath)))
	{
		Result.Message = LOCTEXT("SublevelFolderFailed", "无法创建子关卡目标目录。");
		return Result;
	}
	const TWeakObjectPtr<ULevel> PreviousCurrentLevel = PersistentWorld->GetCurrentLevel();
	ON_SCOPE_EXIT
	{
		if (PreviousCurrentLevel.IsValid())
		{
			PersistentWorld->SetCurrentLevel(PreviousCurrentLevel.Get());
			FEditorDelegates::NewCurrentLevel.Broadcast();
		}
	};
	const FScopedTransaction Transaction(LOCTEXT("CreateAttachedSublevelTransaction", "创建并关联场景子关卡"));
	PersistentWorld->Modify();
	const FString Filename = FPackageName::LongPackageNameToFilename(Result.PackagePath, FPackageName::GetMapPackageExtension());
	UEditorLevelUtils::FCreateNewStreamingLevelForWorldParams Params(ULevelStreamingAlwaysLoaded::StaticClass(), Filename);
	Params.bUseSaveAs = false;
	Params.TemplateWorld = TemplateWorld;
	ULevelStreaming* Streaming = UEditorLevelUtils::CreateNewStreamingLevelForWorld(*PersistentWorld, Params);
	Result.bAssetCreated = FPackageName::DoesPackageExist(Result.PackagePath);
	Result.bAttached = Streaming && HasStreamingReference(PersistentWorld, Result.PackagePath);
	if (!Result.bAttached)
	{
		Result.Message = Result.bAssetCreated
			? LOCTEXT("SublevelCreatedButUnattached", "子关卡文件已创建，但关联失败。文件已保留，可使用关联已有子关卡重试。")
			: LOCTEXT("SublevelCreationFailed", "子关卡创建失败，未保存主关卡。");
		return Result;
	}
	Streaming->SetFlags(RF_Transactional);
	Streaming->SetShouldBeLoaded(true);
	Streaming->SetShouldBeVisible(true);
	TArray<UPackage*> Packages{PersistentWorld->GetOutermost()};
	if (UPackage* ChildPackage = FindPackage(nullptr, *Result.PackagePath))
	{
		Packages.AddUnique(ChildPackage);
	}
	Result.bParentSaved = SaveScopedPackages(Packages) && !PersistentWorld->GetOutermost()->IsDirty();
	Result.bSuccess = Result.bAssetCreated && Result.bAttached && Result.bParentSaved;
	Result.Message = Result.bSuccess
		? FText::Format(LOCTEXT("SublevelAttachedSuccess", "已创建并关联子关卡：{0}"), FText::FromString(Result.PackagePath))
		: LOCTEXT("SublevelParentSaveFailed", "子关卡文件已保留，编辑器中的关联尚未完全保存。可保存主关卡重试，或撤销关联；不会删除任何文件。");
	return Result;
}

FDevKitSublevelResult FDevKitMapCreatorService::AttachExistingSublevel(UWorld* PersistentWorld, const FString& PackagePath)
{
	FText Error;
	if (!ValidatePersistentEditorWorld(PersistentWorld, Error))
	{
		return {false, Error, PackagePath};
	}
	FDevKitSublevelResult Result;
	Result.PackagePath = PackagePath;
	if (!PackagePath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(PackagePath)
		|| !FPackageName::DoesPackageExist(PackagePath) || HasStreamingReference(PersistentWorld, PackagePath))
	{
		Result.Message = LOCTEXT("ExistingSublevelInvalid", "请选择 /Game 下已有且尚未关联的独立子关卡，不能关联主关卡自身。");
		return Result;
	}
	UWorld* ChildWorld = LoadObject<UWorld>(nullptr, *BuildMapObjectPath(PackagePath));
	if (!ChildWorld || ChildWorld->IsPartitionedWorld() || ChildWorld->GetOutermost()->IsDirty()
		|| !ChildWorld->GetStreamingLevels().IsEmpty())
	{
		Result.Message = LOCTEXT("ExistingSublevelDirtyOrWP", "目标不是独立普通子关卡、已有子关卡引用，或有未保存修改；不支持嵌套引用与循环引用。");
		return Result;
	}
	if (!CheckoutWritableParent(PersistentWorld, Error))
	{
		Result.Message = Error;
		return Result;
	}
	const TWeakObjectPtr<ULevel> PreviousCurrentLevel = PersistentWorld->GetCurrentLevel();
	ON_SCOPE_EXIT
	{
		if (PreviousCurrentLevel.IsValid())
		{
			PersistentWorld->SetCurrentLevel(PreviousCurrentLevel.Get());
			FEditorDelegates::NewCurrentLevel.Broadcast();
		}
	};
	const FScopedTransaction Transaction(LOCTEXT("AttachSublevelTransaction", "关联场景子关卡"));
	PersistentWorld->Modify();
	ULevelStreaming* Streaming = UEditorLevelUtils::AddLevelToWorld(PersistentWorld, *PackagePath,
		ULevelStreamingAlwaysLoaded::StaticClass());
	Result.bAttached = Streaming && HasStreamingReference(PersistentWorld, PackagePath);
	if (!Result.bAttached)
	{
		Result.Message = LOCTEXT("ExistingSublevelAttachFailed", "关联子关卡失败，未保存主关卡。");
		return Result;
	}
	Streaming->SetFlags(RF_Transactional);
	Streaming->SetShouldBeLoaded(true);
	Streaming->SetShouldBeVisible(true);
	Result.bParentSaved = SaveScopedPackages({PersistentWorld->GetOutermost()})
		&& !PersistentWorld->GetOutermost()->IsDirty();
	Result.bSuccess = Result.bParentSaved;
	Result.Message = Result.bSuccess
		? FText::Format(LOCTEXT("ExistingSublevelAttachedSuccess", "已关联子关卡：{0}"), FText::FromString(PackagePath))
		: LOCTEXT("ExistingSublevelSaveFailed", "关联仅保留在编辑器中，主关卡保存失败或已取消。请保存重试或撤销关联；原子关卡未被修改。");
	return Result;
}

bool FDevKitMapCreatorService::IsValidNameToken(const FString& Token)
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

FString FDevKitMapCreatorService::NormalizeRootFolder(FString RootFolder)
{
	RootFolder.TrimStartAndEndInline();
	while (RootFolder.EndsWith(TEXT("/")))
	{
		RootFolder.LeftChopInline(1);
	}
	return RootFolder;
}

#undef LOCTEXT_NAMESPACE
