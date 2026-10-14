#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"

#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Tools/CharacterLookDev/DevKitLookDevLightingBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "EditorLevelUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Engine/Selection.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "LevelEditorViewport.h"
#include "LumenVisualizationData.h"
#include "RenderUtils.h"
#include "VirtualShadowMapVisualizationData.h"
#include "Materials/MaterialInterface.h"
#include "MaterialShaderType.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/MetaData.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "DevKitLookDevLevelService"

namespace
{
const FString LookDevRoot = TEXT("/YogLookDev");
const FString ReviewRoot = LookDevRoot / TEXT("ReviewLevel");
const FString LightRoot = LookDevRoot / TEXT("LightDev");

UWorld* EditorWorld()
{
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

FLevelEditorViewportClient* ActiveLevelViewport()
{
	if (!GEditor) return nullptr;
	FLevelEditorViewportClient* Fallback = nullptr;
	for (FLevelEditorViewportClient* Viewport : GEditor->GetLevelViewportClients())
	{
		if (!Viewport || Viewport->GetWorld() != EditorWorld() || !Viewport->IsPerspective() || !Viewport->Viewport) continue;
		if (Viewport->Viewport == GEditor->GetActiveViewport()) return Viewport;
		if (!Fallback || Viewport == GCurrentLevelEditingViewportClient) Fallback = Viewport;
	}
	return Fallback;
}

FLevelEditorViewportClient* ActiveLookDevViewport()
{
	return FDevKitLookDevLevelService::IsMainWorld() ? ActiveLevelViewport() : nullptr;
}

bool SelectedSubjectBounds(FVector& OutOrigin, FBox& OutBounds, FText& OutMessage)
{
	OutBounds = FBox(ForceInit);
	AActor* Subject = nullptr;
	if (GEditor && GEditor->GetSelectedActors())
	{
		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			AActor* Actor = Cast<AActor>(*It);
			if (!Actor || Actor->GetWorld() != EditorWorld()) continue;
			if (Subject)
			{
				OutMessage = LOCTEXT("SelectSingleSubject", "请只选择一个 Review 物件作为镜头目标；不会自动选择或移动场景资产。");
				return false;
			}
			Subject = Actor;
		}
	}
	if (!Subject)
	{
		OutMessage = LOCTEXT("CameraSubjectMissing", "请在 Review 关卡中选择一个角色或物件，再应用对照镜头。");
		return false;
	}
	OutOrigin = Subject->GetActorLocation();
	OutBounds = Subject->GetComponentsBoundingBox(true);
	if (!OutBounds.IsValid) OutBounds = FBox(OutOrigin - FVector(50.f), OutOrigin + FVector(50.f));
	return true;
}

bool CanMoveLookDevViewport(FLevelEditorViewportClient*& OutViewport, FText& OutMessage)
{
	if (!GEditor || GEditor->IsPlaySessionInProgress())
	{
		OutMessage = LOCTEXT("CameraStopPIE", "捕获后请手动停止 PIE，再将镜头应用到 LookDev；工具不会结束或保存 PIE / 关卡。");
		return false;
	}
	OutViewport = ActiveLookDevViewport();
	if (!OutViewport)
	{
		OutMessage = LOCTEXT("CameraLookDevOnly", "请打开 LookDevMain 并激活一个透视视口；镜头/诊断操作不会修改其他生产关卡视口。");
		return false;
	}
	return true;
}

FString ReadConsoleValue(const TCHAR* Name)
{
	const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
	return Variable ? Variable->GetString() : TEXT("不可用");
}

FString SelectedMaterialDiagnostics()
{
	FString Result(TEXT("\n所选 Review 物件 / 已加载材质状态（只读，不迁移资产）："));
	int32 SelectedActorCount = 0;
	int32 TotalSlots = 0;
	int32 DisplayedSlots = 0;
	constexpr int32 MaxDisplayedSlots = 8;
	if (GEditor && GEditor->GetSelectedActors())
	{
		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			const AActor* Actor = Cast<AActor>(*It);
			if (!Actor || Actor->GetWorld() != EditorWorld()) continue;
			++SelectedActorCount;
			TInlineComponentArray<UMeshComponent*> Meshes;
			Actor->GetComponents(Meshes, true);
			for (const UMeshComponent* Mesh : Meshes)
			{
				if (!Mesh) continue;
				for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
				{
					++TotalSlots;
					if (DisplayedSlots >= MaxDisplayedSlots) continue;
					++DisplayedSlots;
					const UMaterialInterface* Material = Mesh->GetMaterial(Slot);
					FString State(TEXT("未绑定材质；请检查 Mesh / 材质槽"));
					if (Material)
					{
						const FMaterialShadingModelField Models = Material->GetShadingModels();
						TArray<FString> Responses;
						if (Models.HasShadingModel(MSM_StylizedCharacterLit)) Responses.Add(TEXT("StylizedCharacterLit → 支持角色连续受光"));
						if (Models.HasShadingModel(MSM_DefaultLit)) Responses.Add(TEXT("DefaultLit → 原生受光，只参加全局调色，不使用角色专属受光参数"));
						if (Models.HasShadingModel(MSM_Unlit)) Responses.Add(TEXT("Unlit → 不参加真实灯光；角色受光参数无效"));
						if (Responses.IsEmpty()) Responses.Add(TEXT("其他着色模型；请按材质类型检查受光支持"));
						State = FString::Printf(TEXT("%s | %s | %s"), *Material->GetPathName(), *GetShadingModelFieldString(Models), *FString::Join(Responses, TEXT("；")));
					}
					Result += FString::Printf(TEXT("\n%s / %s / 槽 %d：%s"), *Actor->GetActorLabel(), *Mesh->GetName(), Slot, *State);
				}
			}
		}
	}
	if (SelectedActorCount == 0) Result += TEXT("\n请在 Review 关卡中选择一个角色或物件，查看实际 Mesh 材质；不会自动加载玩家或迁移旧 Unlit 材质。");
	else if (TotalSlots == 0) Result += TEXT("\n所选物件没有可读取的 Mesh 材质槽。");
	if (TotalSlots > DisplayedSlots) Result += FString::Printf(TEXT("\n另有 %d 个材质槽未展开；最多显示前 8 个，请缩小所选物件范围。"), TotalSlots - DisplayedSlots);
	return Result;
}

bool CanUseEditor(FText& OutMessage)
{
	if (!GEditor)
	{
		OutMessage = LOCTEXT("NoEditor", "当前没有可用的关卡编辑器。");
		return false;
	}
	if (GEditor->IsPlaySessionInProgress())
	{
		OutMessage = LOCTEXT("StopPIE", "请先停止 PIE / Simulate，再切换 LookDev 关卡。");
		return false;
	}
	return true;
}

TArray<FDevKitLookDevLevelEntry> DefaultReviews()
{
	return {
		{ TEXT("Review"), LOCTEXT("Review", "通用 Review"), ReviewRoot / TEXT("LookDev_Review"), LOCTEXT("ReviewIntent", "通用资产检查；关卡留空，由美术填充。") },
		{ TEXT("Character"), LOCTEXT("Character", "角色 Review"), ReviewRoot / TEXT("LookDev_Character"), LOCTEXT("CharacterIntent", "角色模型、材质与动作检查；关卡留空，由美术填充。") },
		{ TEXT("Building"), LOCTEXT("Building", "Building"), ReviewRoot / TEXT("LookDev_Building"), LOCTEXT("BuildingIntent", "建筑与模块化构件检查；关卡留空，由美术填充。") },
		{ TEXT("Prop"), LOCTEXT("Prop", "Prop"), ReviewRoot / TEXT("LookDev_Prop"), LOCTEXT("PropIntent", "道具模型与材质检查；关卡留空，由美术填充。") }
	};
}

TArray<FDevKitLookDevLevelEntry> DefaultLights()
{
	TArray<FDevKitLookDevLevelEntry> Entries;
	for (const FDevKitCharacterLookDevPreset& Preset : FDevKitCharacterLookDevPresets::GetPresets())
	{
		Entries.Add({ Preset.Id, FText::FromString(Preset.Label), LightRoot / (TEXT("LookDev_Light_") + Preset.Id),
			FText::FromString(Preset.Intent + TEXT("\n") + Preset.Basis) });
	}
	return Entries;
}

void DiscoverMaps(const FString& Folder, TArray<FDevKitLookDevLevelEntry>& Entries)
{
	TArray<FString> Files;
	const FString DiskFolder = FPackageName::LongPackageNameToFilename(Folder);
	IFileManager::Get().FindFilesRecursive(Files, *DiskFolder, TEXT("*.umap"), true, false);
	Files.Sort();
	for (const FString& Filename : Files)
	{
		FString PackagePath;
		if (!FPackageName::TryConvertFilenameToLongPackageName(Filename, PackagePath)) continue;
		if (Entries.ContainsByPredicate([&PackagePath](const auto& Entry) { return Entry.PackagePath == PackagePath; })) continue;
		const FString Name = FPackageName::GetLongPackageAssetName(PackagePath);
		Entries.Add({ Name, FText::FromString(Name), PackagePath,
			LOCTEXT("DiscoveredMap", "该文件夹中的自定义子关卡。加载后保留关卡中已制作的内容。") });
	}
}

ULevelStreaming* FindStreaming(UWorld* World, const FString& PackagePath)
{
	if (!World) return nullptr;
	for (ULevelStreaming* Streaming : World->GetStreamingLevels())
	{
		if (Streaming && Streaming->GetWorldAssetPackageName() == PackagePath) return Streaming;
	}
	return nullptr;
}

ULevelStreamingDynamic* AddStreamingReference(UWorld* World, const FString& PackagePath, bool bVisible)
{
	ULevelStreamingDynamic* Streaming = NewObject<ULevelStreamingDynamic>(World, NAME_None, RF_Transactional);
	Streaming->SetWorldAssetByPackageName(FName(*PackagePath));
	Streaming->LevelTransform = FTransform::Identity;
	Streaming->bInitiallyLoaded = bVisible;
	Streaming->bInitiallyVisible = bVisible;
	Streaming->SetShouldBeLoaded(bVisible);
	Streaming->SetShouldBeVisible(bVisible);
	Streaming->SetShouldBeVisibleInEditor(bVisible);
	World->AddStreamingLevel(Streaming);
	return Streaming;
}

FString VisiblePath(UWorld* World, const TArray<FDevKitLookDevLevelEntry>& Entries)
{
	for (const auto& Entry : Entries)
	{
		if (const ULevelStreaming* Streaming = FindStreaming(World, Entry.PackagePath))
		{
			if (Streaming->GetShouldBeVisibleInEditor()) return Entry.PackagePath;
		}
	}
	return FString();
}

FString VisiblePathInFolder(UWorld* World, const FString& Folder)
{
	if (!World) return FString();
	const FString Prefix = Folder + TEXT("/");
	for (const ULevelStreaming* Streaming : World->GetStreamingLevels())
	{
		if (Streaming && Streaming->GetShouldBeVisibleInEditor())
		{
			const FString PackagePath = Streaming->GetWorldAssetPackageName();
			if (PackagePath.StartsWith(Prefix)) return PackagePath;
		}
	}
	return FString();
}

bool EnsureReferences(UWorld* World, FText& OutMessage)
{
	ULevel* OriginalCurrentLevel = World->GetCurrentLevel();
	ON_SCOPE_EXIT
	{
		// AddLevelToWorld selects its new child. Discovery must not redirect artist placement
		// from the active Review into a lighting map, including a partially failed attach.
		if (OriginalCurrentLevel && World->GetLevels().Contains(OriginalCurrentLevel))
		{
			World->SetCurrentLevel(OriginalCurrentLevel);
		}
	};
	TArray<FDevKitLookDevLevelEntry> Entries = FDevKitLookDevLevelService::GetReviewEntries();
	Entries.Append(FDevKitLookDevLevelService::GetLightingEntries());
	for (const auto& Entry : Entries)
	{
		if (Entry.bStandalone) continue;
		if (FindStreaming(World, Entry.PackagePath)) continue;
		if (!FPackageName::DoesPackageExist(Entry.PackagePath))
		{
			OutMessage = FText::Format(LOCTEXT("SublevelMissing", "子关卡不存在：{0}。请完成 LookDev 关卡初始化。"), FText::FromString(Entry.PackagePath));
			return false;
		}
		const FString ObjectPath = Entry.PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(Entry.PackagePath);
		UWorld* MapAsset = LoadObject<UWorld>(nullptr, *ObjectPath);
		if (!MapAsset || MapAsset->IsPartitionedWorld())
		{
			OutMessage = FText::Format(LOCTEXT("ClassicLevelRequired", "LookDev 子关卡需要普通、非 World Partition 关卡：{0}"), FText::FromString(Entry.PackagePath));
			return false;
		}
		// New custom maps join the saved main stack on the next explicit map save.
		UEditorLevelUtils::FAddLevelToWorldParams Params(ULevelStreamingDynamic::StaticClass(), FName(*Entry.PackagePath));
		Params.LevelStreamingCreatedCallback = [](ULevelStreaming* Streaming)
		{
			Streaming->SetShouldBeVisibleInEditor(false);
			Streaming->SetShouldBeVisible(false);
			Streaming->SetShouldBeLoaded(false);
			if (auto* Dynamic = Cast<ULevelStreamingDynamic>(Streaming))
			{
				Dynamic->bInitiallyLoaded = false;
				Dynamic->bInitiallyVisible = false;
			}
		};
		if (!UEditorLevelUtils::AddLevelToWorld(World, Params))
		{
			OutMessage = FText::Format(LOCTEXT("AddFailed", "无法挂载子关卡：{0}"), FText::FromString(Entry.PackagePath));
			return false;
		}
	}
	return true;
}

bool SelectExclusive(UWorld* World, const TArray<FDevKitLookDevLevelEntry>& Entries,
	const FString& SelectedPath, bool bMakeCurrent, FText& OutMessage)
{
	const auto* Entry = Entries.FindByPredicate([&SelectedPath](const auto& Item) { return Item.PackagePath == SelectedPath; });
	if (!Entry)
	{
		OutMessage = LOCTEXT("InvalidSelection", "请选择列表中的 LookDev 子关卡。");
		return false;
	}
	ULevelStreaming* Selected = FindStreaming(World, SelectedPath);
	if (Selected) Selected->SetShouldBeLoaded(true);
	if (!Selected || !Selected->GetLoadedLevel())
	{
		World->FlushLevelStreaming();
		Selected = FindStreaming(World, SelectedPath);
	}
	if (!Selected || !Selected->GetLoadedLevel())
	{
		OutMessage = FText::Format(LOCTEXT("LoadFailed", "子关卡加载失败：{0}"), FText::FromString(SelectedPath));
		return false;
	}
	if (bMakeCurrent) World->SetCurrentLevel(World->PersistentLevel);
	TArray<ULevel*> Levels;
	TArray<bool> Visibility;
	for (const auto& Item : Entries)
	{
		ULevelStreaming* Streaming = FindStreaming(World, Item.PackagePath);
		if (!Streaming) continue;
		const bool bSelected = Item.PackagePath == SelectedPath;
		// Keep editor packages loaded so a hidden authored review can retain unsaved edits.
		// Switching previews changes visibility only; no streaming level is unloaded.
		Streaming->SetShouldBeLoaded(true);
		Streaming->SetShouldBeVisible(bSelected);
		if (auto* Dynamic = Cast<ULevelStreamingDynamic>(Streaming))
		{
			Dynamic->bInitiallyLoaded = true;
			Dynamic->bInitiallyVisible = bSelected;
		}
		if (ULevel* Level = Streaming->GetLoadedLevel())
		{
			Levels.Add(Level);
			Visibility.Add(bSelected);
		}
		else Streaming->SetShouldBeVisibleInEditor(bSelected);
	}
	// Preview choices do not dirty all review/light packages or save them implicitly.
	UEditorLevelUtils::SetLevelsVisibility(Levels, Visibility, true, ELevelVisibilityDirtyMode::DontModify);
	if (bMakeCurrent) World->SetCurrentLevel(Selected->GetLoadedLevel());
	GEditor->RedrawLevelEditingViewports();
	OutMessage = FText::Format(LOCTEXT("Loaded", "已加载：{0}"), Entry->Label);
	return true;
}

bool CreateSavedWorld(const FString& PackagePath, const FDevKitCharacterLookDevPreset* Preset,
	bool bMain, FText& OutMessage)
{
	if (FindPackage(nullptr, *PackagePath))
	{
		OutMessage = FText::Format(LOCTEXT("UnsavedPackageConflict", "同名关卡已在内存中，请先保存或关闭它：{0}"), FText::FromString(PackagePath));
		return false;
	}
	UPackage* Package = CreatePackage(*PackagePath);
	UWorld::InitializationValues Init;
	Init.CreatePhysicsScene(false).ShouldSimulatePhysics(false).CreateNavigation(false).CreateAISystem(false).CreateWorldPartition(false);
	TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Inactive, false,
		FName(*FPackageName::GetLongPackageAssetName(PackagePath)), Package, false, ERHIFeatureLevel::Num, &Init));
	if (!World.IsValid()) return false;
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		// Later editor loads must read the saved package, not reuse this cleaned-up bootstrap world.
		World->MarkObjectsPendingKill();
	};
	World->SetFlags(RF_Public | RF_Standalone);
	if (Preset && !YogLookDev::BuildLightingLevel(World.Get(), Preset->Id, OutMessage))
	{
		OutMessage = FText::Format(LOCTEXT("RigCreateFailed", "创建灯光失败：{0}"), FText::FromString(PackagePath));
		return false;
	}
	if (Preset)
	{
		Package->GetMetaData().SetValue(World.Get(), TEXT("LookDevPreset"), *Preset->Id);
		Package->GetMetaData().SetValue(World.Get(), TEXT("LookDevLightingBasis"), *Preset->Basis);
	}
	if (bMain)
	{
		const auto Reviews = DefaultReviews();
		const auto Lights = DefaultLights();
		for (int32 Index = 0; Index < Reviews.Num(); ++Index) AddStreamingReference(World.Get(), Reviews[Index].PackagePath, Index == 0);
		for (int32 Index = 0; Index < Lights.Num(); ++Index) AddStreamingReference(World.Get(), Lights[Index].PackagePath, Index == 0);
	}
	const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetMapPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
	// Recheck immediately before the only write path. Setup never saves an existing map.
	if (IFileManager::Get().FileExists(*Filename))
	{
		OutMessage = FText::Format(LOCTEXT("ConcurrentMapCreated", "关卡已由另一个任务创建，未覆盖：{0}"), FText::FromString(PackagePath));
		return false;
	}
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	Args.bSlowTask = false;
	if (!UPackage::SavePackage(Package, World.Get(), *Filename, Args))
	{
		OutMessage = FText::Format(LOCTEXT("MapSaveFailed", "关卡保存失败：{0}"), FText::FromString(PackagePath));
		return false;
	}
	FAssetRegistryModule::AssetCreated(World.Get());
	Package->SetDirtyFlag(false);
	return true;
}
}

FString FDevKitLookDevLevelService::MainPackagePath()
{
	return LookDevRoot / TEXT("LookDevMain");
}

TArray<FDevKitLookDevLevelEntry> FDevKitLookDevLevelService::GetReviewEntries()
{
	auto Entries = DefaultReviews();
	DiscoverMaps(ReviewRoot, Entries);
	const FDevKitLookDevLevelEntry ExistingGyms[] = {
		{ TEXT("GymL1"), LOCTEXT("GymL1", "Gym / L1 美术检查（独立关卡）"), TEXT("/Game/Art/Map/ArtCheckLevel/GymLeve_L1"),
			LOCTEXT("GymL1Description", "保留原有嵌套关卡与灯光；作为独立关卡打开。"), true },
		{ TEXT("GymLoot"), LOCTEXT("GymLoot", "Gym / 掉落与战斗测试（独立关卡）"), TEXT("/Game/Developers/sunchuankai/LootTest/Gym"),
			LOCTEXT("GymLootDescription", "保留现有测试内容与灯光；作为独立关卡打开。"), true }
	};
	for (const auto& Entry : ExistingGyms)
	{
		if (FPackageName::DoesPackageExist(Entry.PackagePath)) Entries.Add(Entry);
	}
	return Entries;
}

TArray<FDevKitLookDevLevelEntry> FDevKitLookDevLevelService::GetLightingEntries()
{
	return DefaultLights();
}

TArray<FString> FDevKitLookDevLevelService::GetRetiredLightingPackagePaths()
{
	return {
		LightRoot / TEXT("LookDev_Light_L1_Shadow"),
		LightRoot / TEXT("LookDev_Light_L2_SmokeShadow"),
		LightRoot / TEXT("LookDev_Light_L2_Backlight"),
		LightRoot / TEXT("LookDev_Light_L3_NaveShadow"),
		LightRoot / TEXT("LookDev_Light_L3_Corruption")
	};
}

bool FDevKitLookDevLevelService::PruneRetiredLightingReferences(FText& OutMessage)
{
	const FString MainPath = MainPackagePath();
	const FString MainObjectPath = MainPath + TEXT(".") + FPackageName::GetLongPackageAssetName(MainPath);
	UWorld* MainWorld = LoadObject<UWorld>(nullptr, *MainObjectPath);
	if (!MainWorld || !MainWorld->GetPackage())
	{
		OutMessage = LOCTEXT("RetireMainMissing", "LookDevMain 不存在或无法加载；没有删除灯光关卡引用。");
		return false;
	}
	const TArray<FString> RetiredPaths = GetRetiredLightingPackagePaths();
	TArray<ULevelStreaming*> RetiredReferences;
	for (ULevelStreaming* Streaming : MainWorld->GetStreamingLevels())
	{
		if (Streaming && RetiredPaths.Contains(Streaming->GetWorldAssetPackageName()))
		{
			RetiredReferences.Add(Streaming);
		}
	}
	for (ULevelStreaming* Streaming : RetiredReferences)
	{
		MainWorld->RemoveStreamingLevel(Streaming);
	}
	if (!RetiredReferences.IsEmpty())
	{
		MainWorld->GetPackage()->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(MainPath, FPackageName::GetMapPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		Args.bSlowTask = false;
		if (!UPackage::SavePackage(MainWorld->GetPackage(), MainWorld, *Filename, Args))
		{
			OutMessage = LOCTEXT("RetireMainSaveFailed", "清理 LookDevMain 旧灯光引用时保存失败。");
			return false;
		}
		MainWorld->GetPackage()->SetDirtyFlag(false);
	}
	OutMessage = FText::Format(LOCTEXT("RetireMainDone", "已从 LookDevMain 移除 {0} 个旧灯光引用。"), FText::AsNumber(RetiredReferences.Num()));
	return true;
}

bool FDevKitLookDevLevelService::IsMainWorld()
{
	const UWorld* World = EditorWorld();
	return World && World->WorldType == EWorldType::Editor && World->GetPackage()->GetName() == MainPackagePath();
}

FString FDevKitLookDevLevelService::GetCurrentReviewPath()
{
	// Slate queries this every frame. Read loaded streaming state without scanning the filesystem.
	if (IsMainWorld()) return VisiblePathInFolder(EditorWorld(), ReviewRoot);
	if (const UWorld* World = EditorWorld())
	{
		const FString CurrentPath = World->GetPackage()->GetName();
		if (CurrentPath == TEXT("/Game/Art/Map/ArtCheckLevel/GymLeve_L1")
			|| CurrentPath == TEXT("/Game/Developers/sunchuankai/LootTest/Gym")) return CurrentPath;
	}
	return FString();
}

FString FDevKitLookDevLevelService::GetCurrentLightingPath()
{
	return IsMainWorld() ? VisiblePathInFolder(EditorWorld(), LightRoot) : FString();
}

bool FDevKitLookDevLevelService::OpenMain(FText& OutMessage)
{
	if (!CanUseEditor(OutMessage)) return false;
	if (FApp::IsUnattended() && !FPackageName::DoesPackageExist(MainPackagePath()))
	{
		OutMessage = LOCTEXT("UnattendedMainMissing",
			"LookDevMain 尚不存在；无人值守打开面板不会自动创建关卡。请先显式运行 LookDev 初始化，或完成已有 LookDev 资产的迁移/恢复后再打开。");
		return false;
	}
	if (!IsMainWorld())
	{
		// The editor's standard save/cancel path protects the artist's currently open map.
		if (FApp::IsUnattended() && FEditorFileUtils::HasDirtyPackages())
		{
			OutMessage = LOCTEXT("UnattendedDirty", "存在未保存资产；无人值守打开 LookDev 已取消。");
			return false;
		}
		if (!FEditorFileUtils::SaveDirtyPackages(true, true, true))
		{
			OutMessage = LOCTEXT("OpenCancelled", "打开 LookDev 已取消，当前关卡保留。");
			return false;
		}
		if (!FPackageName::DoesPackageExist(MainPackagePath()) && !CreateMissingAssets(OutMessage)) return false;
		const FString Filename = FPackageName::LongPackageNameToFilename(MainPackagePath(), FPackageName::GetMapPackageExtension());
		if (!UEditorLoadingAndSavingUtils::LoadMap(Filename) || !IsMainWorld())
		{
			OutMessage = LOCTEXT("MainLoadFailed", "LookDevMain 打开失败，请查看日志。");
			return false;
		}
	}
	UWorld* World = EditorWorld();
	if (!EnsureReferences(World, OutMessage)) return false;
	World->FlushLevelStreaming();
	const auto Reviews = GetReviewEntries();
	const auto Lights = GetLightingEntries();
	FString ReviewPath = VisiblePath(World, Reviews);
	FString LightPath = VisiblePath(World, Lights);
	if (ReviewPath.IsEmpty()) ReviewPath = Reviews[0].PackagePath;
	if (LightPath.IsEmpty()) LightPath = Lights[0].PackagePath;
	if (!SelectExclusive(World, Reviews, ReviewPath, true, OutMessage)
		|| !SelectExclusive(World, Lights, LightPath, false, OutMessage)) return false;
	OutMessage = LOCTEXT("MainReady", "LookDevMain 已打开。选择 Review 后点击“加载关卡”；点击灯光直接切换。");
	return true;
}

bool FDevKitLookDevLevelService::LoadReview(const FString& PackagePath, FText& OutMessage)
{
	const auto Entries = GetReviewEntries();
	const auto* Entry = Entries.FindByPredicate([&PackagePath](const auto& Item) { return Item.PackagePath == PackagePath; });
	if (!Entry)
	{
		OutMessage = LOCTEXT("UnknownReview", "Review 关卡不在列表中。");
		return false;
	}
	if (Entry->bStandalone)
	{
		if (!CanUseEditor(OutMessage)) return false;
		if (FApp::IsUnattended() && FEditorFileUtils::HasDirtyPackages())
		{
			OutMessage = LOCTEXT("UnattendedGymDirty", "存在未保存资产；无人值守打开 Gym 已取消。");
			return false;
		}
		if (!FEditorFileUtils::SaveDirtyPackages(true, true, true))
		{
			OutMessage = LOCTEXT("GymCancelled", "打开 Gym 已取消，当前关卡保留。");
			return false;
		}
		const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetMapPackageExtension());
		if (!UEditorLoadingAndSavingUtils::LoadMap(Filename))
		{
			OutMessage = LOCTEXT("GymLoadFailed", "Gym 关卡打开失败，请查看日志。");
			return false;
		}
		OutMessage = FText::Format(LOCTEXT("GymReady", "已打开 {0}；该独立关卡使用原有灯光。选择 Review 后加载即可返回 LookDevMain。"), Entry->Label);
		return true;
	}
	if (!CanUseEditor(OutMessage) || (!IsMainWorld() && !OpenMain(OutMessage))) return false;
	if (!EnsureReferences(EditorWorld(), OutMessage)) return false;
	return SelectExclusive(EditorWorld(), Entries, PackagePath, true, OutMessage);
}

bool FDevKitLookDevLevelService::SwitchLighting(const FString& PackagePath, FText& OutMessage)
{
	const auto Entries = GetLightingEntries();
	if (!Entries.ContainsByPredicate([&PackagePath](const auto& Entry) { return Entry.PackagePath == PackagePath; }))
	{
		OutMessage = LOCTEXT("UnknownLight", "灯光关卡不在列表中。");
		return false;
	}
	if (!CanUseEditor(OutMessage) || (!IsMainWorld() && !OpenMain(OutMessage))) return false;
	if (!EnsureReferences(EditorWorld(), OutMessage)) return false;
	return SelectExclusive(EditorWorld(), Entries, PackagePath, false, OutMessage);
}

bool FDevKitLookDevLevelService::CreateMissingAssets(FText& OutMessage, TArray<FString>* OutCreated)
{
	if (OutCreated) OutCreated->Reset();
	// Preflight the entire map set before creating even one package. During a
	// migration (or recovery), an original package/redirector means authored data
	// exists and must not be silently replaced by a generated bootstrap map.
	TArray<FString> ExpectedPackages = { MainPackagePath() };
	for (const auto& Entry : DefaultReviews()) ExpectedPackages.Add(Entry.PackagePath);
	for (const auto& Entry : DefaultLights()) ExpectedPackages.Add(Entry.PackagePath);
	for (const FString& PackagePath : ExpectedPackages)
	{
		if (FPackageName::DoesPackageExist(PackagePath)) continue;
		const FString LegacyPackagePath = TEXT("/Game/Art/Map/LookDev") + PackagePath.RightChop(LookDevRoot.Len());
		if (FPackageName::DoesPackageExist(LegacyPackagePath))
		{
			OutMessage = FText::Format(LOCTEXT("MigrationOrRecoveryRequired",
				"缺少插件关卡 {0}，但对应旧 LookDev 包 {1} 仍存在（可能是原资产或重定向）。已停止初始化，未创建任何关卡；请先完成 LookDev 迁移或恢复缺失资产，不能用默认关卡覆盖原有设计。"),
				FText::FromString(PackagePath), FText::FromString(LegacyPackagePath));
			return false;
		}
	}
	int32 Created = 0;
	ON_SCOPE_EXIT
	{
		if (Created > 0)
		{
			// CreateSavedWorld's strong references have been released by this point. Purge its
			// marked bootstrap worlds from the UObject name table before an immediate OpenMain.
			// Native editor keep flags retain other standalone assets and live editor references.
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		}
	};
	for (const auto& Entry : DefaultReviews())
	{
		if (FPackageName::DoesPackageExist(Entry.PackagePath)) continue;
		if (!CreateSavedWorld(Entry.PackagePath, nullptr, false, OutMessage)) return false;
		++Created;
		if (OutCreated) OutCreated->Add(Entry.PackagePath);
	}
	for (const auto& Entry : DefaultLights())
	{
		if (FPackageName::DoesPackageExist(Entry.PackagePath)) continue;
		if (!CreateSavedWorld(Entry.PackagePath, FDevKitCharacterLookDevPresets::Find(Entry.Id), false, OutMessage)) return false;
		++Created;
		if (OutCreated) OutCreated->Add(Entry.PackagePath);
	}
	if (!FPackageName::DoesPackageExist(MainPackagePath()))
	{
		if (!CreateSavedWorld(MainPackagePath(), nullptr, true, OutMessage)) return false;
		++Created;
		if (OutCreated) OutCreated->Add(MainPackagePath());
	}
	OutMessage = FText::Format(LOCTEXT("Created", "LookDev 初始化完成：新建 {0} 个关卡，已存在的关卡保持原内容。"), FText::AsNumber(Created));
	return true;
}

bool FDevKitLookDevCameraReference::IsValid() const
{
	if (!FMath::IsFinite(Rotation.Pitch) || !FMath::IsFinite(Rotation.Yaw) || !FMath::IsFinite(Rotation.Roll))
	{
		return false;
	}
	return !TargetToCameraOffset.ContainsNaN() && FMath::IsFinite(TargetToCameraOffset.SizeSquared()) && TargetToCameraOffset.SizeSquared() > 1.f
		&& FMath::IsFinite(FieldOfView) && FieldOfView > 1.f && FieldOfView < 179.f
		&& FMath::IsFinite(AspectRatio) && AspectRatio > 0.f;
}

bool FDevKitLookDevLevelService::ReadPlayerCameraClass(UClass* PlayerClass, FDevKitLookDevCameraReference& OutCamera, FText& OutMessage)
{
	if (!PlayerClass || !PlayerClass->IsChildOf(AActor::StaticClass()))
	{
		OutMessage = LOCTEXT("InvalidPlayerCameraClass", "请选择含有相机与 SpringArm 的玩家 Blueprint；没有回退到固定俯视镜头。");
		return false;
	}
	const AActor* Defaults = PlayerClass->GetDefaultObject<AActor>();
	const UCameraComponent* Follow = Defaults ? Defaults->FindComponentByClass<UCameraComponent>() : nullptr;
	const USpringArmComponent* Boom = Follow ? Cast<USpringArmComponent>(Follow->GetAttachParent()) : nullptr;
	if (!Boom && Defaults) Boom = Defaults->FindComponentByClass<USpringArmComponent>();
	if (!Boom || !Follow)
	{
		OutMessage = LOCTEXT("PlayerCameraComponentsMissing", "玩家 Blueprint 缺少 CameraBoom / FollowCamera，无法读取真实默认镜头。可捕获 PIE 镜头或手填参数。");
		return false;
	}
	if (Follow->ProjectionMode != ECameraProjectionMode::Perspective)
	{
		OutMessage = LOCTEXT("PlayerCameraNotPerspective", "当前玩家类使用正交镜头，本页 FOV / 臂长参考不适用。没有按透视镜头错误转换。");
		return false;
	}
	// Project player classes may apply authored camera properties at BeginPlay.
	// Read those optional properties by reflection, keeping this plugin independent
	// of the project's native player and custom SpringArm classes.
	const auto ReadCameraScalar = [Defaults](const TCHAR* Name, float Fallback)
	{
		const FNumericProperty* Property = FindFProperty<FNumericProperty>(Defaults->GetClass(), Name);
		if (!Property || !Property->IsFloatingPoint()) return Fallback;
		return static_cast<float>(Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Defaults)));
	};
	// An unregistered CDO component's cached world transform can be identity.
	// Read the authored attachment rotation, rather than that cached transform.
	const FRotator ArmRotation = Boom->GetRelativeRotation();
	FDevKitLookDevCameraReference Candidate;
	Candidate.FieldOfView = ReadCameraScalar(TEXT("DefaultCameraFOV"), Follow->FieldOfView);
	Candidate.AspectRatio = Follow->AspectRatio;
	Candidate.Rotation = (ArmRotation.Quaternion() * Follow->GetRelativeRotation().Quaternion()).Rotator();
	Candidate.TargetToCameraOffset = Boom->GetRelativeLocation() + Boom->TargetOffset
		- ArmRotation.Vector() * ReadCameraScalar(TEXT("DefaultCameraBoomLength"), Boom->TargetArmLength)
		+ ArmRotation.RotateVector(Boom->SocketOffset + Follow->GetRelativeLocation());
	Candidate.Source = FText::Format(LOCTEXT("BlueprintCameraSource", "玩家类默认镜头：{0}（不包含运行时 Volume、镜头碰撞或 CameraManager 偏移）"), FText::FromString(PlayerClass->GetPathName()));
	if (!Candidate.IsValid())
	{
		OutMessage = LOCTEXT("PlayerCameraInvalid", "玩家类相机参数无效；没有修改视口。请检查臂长/FOV，或在 PIE 中捕获最终镜头。");
		return false;
	}
	OutCamera = Candidate;
	OutMessage = Candidate.Source;
	return true;
}

bool FDevKitLookDevLevelService::ReadPlayerCameraBlueprint(const FString& ObjectPath, FDevKitLookDevCameraReference& OutCamera, FText& OutMessage)
{
	if (ObjectPath.IsEmpty())
	{
		OutMessage = LOCTEXT("PickPlayerBlueprint", "请先选择玩家 Blueprint，再读取相机；不会把源码默认近似镜头标记为实际游戏镜头。");
		return false;
	}
	const UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath);
	return ReadPlayerCameraClass(Blueprint ? Blueprint->GeneratedClass.Get() : nullptr, OutCamera, OutMessage);
}

bool FDevKitLookDevLevelService::CapturePIECamera(FDevKitLookDevCameraReference& OutCamera, FText& OutMessage)
{
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* Manager = Player ? Player->PlayerCameraManager : nullptr;
	AActor* Target = Manager ? Manager->GetViewTarget() : nullptr;
	if (!World || World->WorldType != EWorldType::PIE || !Manager || !Target)
	{
		OutMessage = LOCTEXT("PIECameraMissing", "当前没有可捕获的本机 PIE 玩家镜头。请在真实游戏关卡运行 PIE 后捕获；现有参考保持不变。");
		return false;
	}
	const FMinimalViewInfo& POV = Manager->GetCameraCacheView();
	if (POV.ProjectionMode != ECameraProjectionMode::Perspective)
	{
		OutMessage = LOCTEXT("PIECameraNotPerspective", "当前 PIE 最终 POV 是正交镜头，本页透视对照不适用；现有参考保持不变。");
		return false;
	}
	FDevKitLookDevCameraReference Candidate;
	Candidate.TargetToCameraOffset = POV.Location - Target->GetActorLocation();
	Candidate.Rotation = POV.Rotation;
	Candidate.FieldOfView = POV.FOV;
	Candidate.AspectRatio = POV.AspectRatio;
	Candidate.bFromPIE = true;
	Candidate.Source = FText::Format(LOCTEXT("LivePIECameraSource", "PIE 最终镜头：{0} / {1}（包含当前运行时距离、FOV 与俯角）"), FText::FromString(World->GetMapName()), FText::FromString(Target->GetName()));
	if (!Candidate.IsValid())
	{
		OutMessage = LOCTEXT("PIECameraInvalid", "PIE 相机参数无效或相机正好位于目标原点；现有参考保持不变。");
		return false;
	}
	OutCamera = Candidate;
	OutMessage = LOCTEXT("PIECameraCaptured", "已捕获 PIE 最终镜头；参考仅保留在本次面板会话。手动停止 PIE、返回 LookDev 并选择物件后，点击“应用游戏参考镜头”。");
	return true;
}

bool FDevKitLookDevLevelService::MakeManualCamera(float DistanceCm, float FOV, float Pitch, float Yaw, FDevKitLookDevCameraReference& OutCamera, FText& OutMessage)
{
	// Validate raw inputs before constructing/using an FRotator: Vector() reports
	// invalid angles as LogCore errors, even if the resulting reference is rejected.
	if (!FMath::IsFinite(DistanceCm) || DistanceCm <= 1.f || !FMath::IsFinite(FOV) || FOV <= 1.f || FOV >= 179.f
		|| !FMath::IsFinite(Pitch) || !FMath::IsFinite(Yaw))
	{
		OutMessage = LOCTEXT("ManualCameraInvalid", "手填镜头参数无效：臂长须大于 1cm，FOV 须在 1°–179°之间，角度须为有限值。没有修改视口。");
		return false;
	}
	FDevKitLookDevCameraReference Candidate;
	Candidate.Rotation = FRotator(Pitch, Yaw, 0.f);
	Candidate.TargetToCameraOffset = -Candidate.Rotation.Vector() * DistanceCm;
	Candidate.FieldOfView = FOV;
	Candidate.Source = LOCTEXT("ManualCameraSource", "手填相机参考（不是实际游戏镜头）");
	if (!Candidate.IsValid())
	{
		OutMessage = LOCTEXT("ManualCameraInvalid", "手填镜头参数无效：臂长须大于 1cm，FOV 须在 1°–179°之间，角度须为有限值。没有修改视口。");
		return false;
	}
	OutCamera = Candidate;
	OutMessage = Candidate.Source;
	return true;
}

bool FDevKitLookDevLevelService::ApplyCameraReference(const FDevKitLookDevCameraReference& Camera, FText& OutMessage)
{
	if (!Camera.IsValid())
	{
		OutMessage = LOCTEXT("CameraReferenceMissing", "尚未读取玩家 Blueprint 或捕获 PIE 镜头；也可显式应用手填参考。没有固定角度的自动回退。");
		return false;
	}
	FLevelEditorViewportClient* Viewport = nullptr;
	if (!CanMoveLookDevViewport(Viewport, OutMessage)) return false;
	FVector Target;
	FBox Bounds;
	if (!SelectedSubjectBounds(Target, Bounds, OutMessage)) return false;
	Viewport->SetViewLocation(Target + Camera.TargetToCameraOffset);
	Viewport->SetViewRotation(Camera.Rotation);
	Viewport->ViewFOV = Camera.FieldOfView;
	Viewport->FOVAngle = Camera.FieldOfView;
	Viewport->SetViewMode(VMI_Lit);
	Viewport->SetRealtime(true);
	Viewport->Invalidate();
	OutMessage = FText::Format(LOCTEXT("CameraApplied", "已应用 {0}；FOV {1}°，目标距 {2}cm，俯角 {3}°。仅移动当前 LookDev 视口，场景曝光保持不变。请以 {4} 画幅验收屏幕尺寸。"),
		Camera.Source, FText::AsNumber(Camera.FieldOfView), FText::AsNumber(Camera.TargetToCameraOffset.Size()), FText::AsNumber(Camera.Rotation.Pitch), FText::AsNumber(Camera.AspectRatio));
	return true;
}

bool FDevKitLookDevLevelService::FrameSelectedSubject(FText& OutMessage)
{
	FLevelEditorViewportClient* Viewport = nullptr;
	if (!CanMoveLookDevViewport(Viewport, OutMessage)) return false;
	FVector Origin;
	FBox Bounds;
	if (!SelectedSubjectBounds(Origin, Bounds, OutMessage)) return false;
	Viewport->FocusViewportOnBox(Bounds, true);
	Viewport->SetViewMode(VMI_Lit);
	Viewport->Invalidate();
	OutMessage = LOCTEXT("CloseCameraApplied", "已聚焦所选物件近景；只移动当前 LookDev 视口，不修改灯光、曝光或资产。");
	return true;
}

bool FDevKitLookDevLevelService::SetPreviewViewMode(int32 Mode, FName VisualizationName, FText& OutMessage)
{
	FLevelEditorViewportClient* Viewport = ActiveLevelViewport();
	if (!Viewport || (GEditor && GEditor->IsPlaySessionInProgress()))
	{
		OutMessage = LOCTEXT("DiagnosticViewportMissing", "请激活一个当前关卡的透视编辑器视口；PIE 游戏视口请使用游戏内诊断。无需打开 LookDevMain。");
		return false;
	}
	if (Mode == VMI_VisualizeVirtualShadowMap && !UseVirtualShadowMaps(GMaxRHIShaderPlatform))
	{
		OutMessage = LOCTEXT("VSMPathUnavailable", "VSM 实际平台路径未启用，无法显示 VSM 诊断。请检查 DX12 / SM6、项目支持与启动配置，修改项目支持后需重启。");
		return false;
	}
	const EViewModeIndex ViewMode = static_cast<EViewModeIndex>(Mode);
	if ((ViewMode == VMI_VisualizeLumen && GetLumenVisualizationData().GetModeID(VisualizationName) == INDEX_NONE)
		|| (ViewMode == VMI_VisualizeVirtualShadowMap && GetVirtualShadowMapVisualizationData().GetModeID(VisualizationName) == INDEX_NONE))
	{
		OutMessage = LOCTEXT("DiagnosticModeMissing", "当前引擎没有注册这个诊断模式，未改变视口。");
		return false;
	}
	if (ViewMode == VMI_VisualizeVirtualShadowMap && ReadConsoleValue(TEXT("r.Shadow.Virtual.Enable")) != TEXT("1"))
	{
		OutMessage = LOCTEXT("VSMNotEnabled", "当前 r.Shadow.Virtual.Enable 不是 1，VSM 诊断不可用；工具不会覆盖当前阴影方法。");
		return false;
	}
	Viewport->SetViewMode(ViewMode);
	if (ViewMode == VMI_VisualizeBuffer) Viewport->ChangeBufferVisualizationMode(VisualizationName);
	else if (ViewMode == VMI_VisualizeVirtualShadowMap) Viewport->ChangeVirtualShadowMapVisualizationMode(VisualizationName);
	else if (ViewMode == VMI_VisualizeLumen) Viewport->ChangeLumenVisualizationMode(VisualizationName);
	Viewport->Invalidate();
	const bool bApplied = Viewport->GetViewMode() == ViewMode;
	OutMessage = bApplied ? LOCTEXT("DiagnosisViewApplied", "已切换当前关卡视口的原生诊断模式；没有改灯光、曝光或资产。")
		: LOCTEXT("DiagnosisViewRejected", "诊断模式未生效，请检查当前视口与渲染平台。");
	return bApplied;
}

FText FDevKitLookDevLevelService::GetRenderingDiagnostics()
{
	const FLevelEditorViewportClient* Viewport = ActiveLevelViewport();
	const FIntPoint Size = Viewport && Viewport->Viewport ? Viewport->Viewport->GetSizeXY() : FIntPoint::ZeroValue;
	const FString RuntimeStatus = FString::Printf(TEXT("只读状态 / CVar  |  GI method=%s · Lumen GI=%s · Reflection method=%s · Lumen reflections=%s\nVSM 请求=%s · 实际平台路径=%s · 项目支持=%s · Nanite 几何渲染=%s\n阴影分级=%s · ScreenPercentage CVar=%s · 视口=%d×%d · ToneMapper=%s\n远镜头重点：接触阴影、移动稳定性、薄构件/脸部阴影与 VSM 缓存；诊断颜色不是最终材质颜色，关卡后处理可能覆盖 GI 方法。"),
		*ReadConsoleValue(TEXT("r.DynamicGlobalIlluminationMethod")), *ReadConsoleValue(TEXT("r.Lumen.DiffuseIndirect.Allow")),
		*ReadConsoleValue(TEXT("r.ReflectionMethod")), *ReadConsoleValue(TEXT("r.Lumen.Reflections.Allow")),
		*ReadConsoleValue(TEXT("r.Shadow.Virtual.Enable")), UseVirtualShadowMaps(GMaxRHIShaderPlatform) ? TEXT("VSM") : TEXT("非 VSM / 依赖未满足"),
		*ReadConsoleValue(TEXT("r.Nanite.ProjectEnabled")), *ReadConsoleValue(TEXT("r.Nanite")), *ReadConsoleValue(TEXT("sg.ShadowQuality")),
		*ReadConsoleValue(TEXT("r.ScreenPercentage")), Size.X, Size.Y,
		Viewport ? (Viewport->EngineShowFlags.Tonemapper ? TEXT("开启") : TEXT("关闭")) : TEXT("未激活关卡透视视口"));
	return FText::FromString(RuntimeStatus + SelectedMaterialDiagnostics());
}

#undef LOCTEXT_NAMESPACE
