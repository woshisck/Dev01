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
#include "UObject/UnrealType.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelStreamingAlwaysLoaded.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"

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
	if (!FPackageName::IsValidLongPackageName(RootFolder, true))
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
		if (FPackageName::DoesPackageExist(PackagePath))
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

	if (!FEditorFileUtils::SaveDirtyPackages(
		/*bPromptUserToSave=*/true,
		/*bSaveMapPackages=*/true,
		/*bSaveContentPackages=*/true))
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

	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false))
	{
		return FDevKitMapCreatorResult{false, LOCTEXT("SavePackagesFailed", "资产已创建，但一个或多个包保存失败。"), Paths.GetValue()};
	}

	return FDevKitMapCreatorResult{
		true,
		FText::Format(LOCTEXT("CreateSucceeded", "已创建地图包：{0}"), FText::FromString(Paths->TargetFolder)),
		Paths.GetValue()
	};
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
