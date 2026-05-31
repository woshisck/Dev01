#include "Story/FirstRunTutorialRoomPoolSetupCommandlet.h"

#include "Data/RoomDataAsset.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

// 教程关卡路由配置：每个传送门 index 去往哪个房间是确定的，复用现有 PortalDestinations 机制。
// 期望链路：Hub → 01a → 01b → WaterDungeon → (01a 或 01b) → PrayerRoom
//   portal[0] 走主链；portal[1] 在 01a / 01b 上保留为去 PrayerRoom 的备用出口（教程最后一段由
//   FirstRunTutorialDirectorSubsystem 通过 bForceSinglePortal + PortalIndex=1 切换到这条门）。
namespace FirstRunTutorialRoomPoolSetup
{
const FString HubRoomPath           = TEXT("/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom");
const FString Corridor01aPath       = TEXT("/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a");
const FString Corridor01bPath       = TEXT("/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b");
const FString WaterDungeonRoomPath  = TEXT("/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon");
const FString PrayerRoomPath        = TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom");

FString ToObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

template<typename AssetT>
AssetT* LoadAssetByPackagePath(const FString& PackagePath)
{
	return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
}

// 将 Room.PortalDestinations[PortalIndex].RoomPool 重置为指定单一目标（或目标列表）。
// 如果 PortalIndex 不存在则追加。返回 true 表示资产被改写。
bool SetPortalRoomPool(URoomDataAsset* Room, int32 PortalIndex, const TArray<URoomDataAsset*>& Targets)
{
	if (!Room)
	{
		return false;
	}

	TArray<TObjectPtr<URoomDataAsset>> DesiredPool;
	for (URoomDataAsset* Target : Targets)
	{
		if (Target)
		{
			DesiredPool.Add(Target);
		}
	}

	FPortalDestConfig* Existing = nullptr;
	for (FPortalDestConfig& Cfg : Room->PortalDestinations)
	{
		if (Cfg.PortalIndex == PortalIndex)
		{
			Existing = &Cfg;
			break;
		}
	}

	const auto PoolsMatch = [&](const TArray<TObjectPtr<URoomDataAsset>>& A,
		const TArray<TObjectPtr<URoomDataAsset>>& B) -> bool
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].Get() != B[Index].Get())
			{
				return false;
			}
		}
		return true;
	};

	if (Existing)
	{
		if (PoolsMatch(Existing->RoomPool, DesiredPool))
		{
			return false;
		}
		Room->Modify();
		Existing->RoomPool = DesiredPool;
		return true;
	}

	Room->Modify();
	FPortalDestConfig NewCfg;
	NewCfg.PortalIndex = PortalIndex;
	NewCfg.RoomPool = DesiredPool;
	Room->PortalDestinations.Add(MoveTemp(NewCfg));
	return true;
}

// 把指定房间从所有传送门池中移除（用于撤销之前 commandlet 把 WaterDungeon 加到所有池里的副作用）。
bool RemoveRoomFromAllPortalPools(URoomDataAsset* Room, URoomDataAsset* ToRemove)
{
	if (!Room || !ToRemove)
	{
		return false;
	}

	bool bChanged = false;
	for (FPortalDestConfig& Cfg : Room->PortalDestinations)
	{
		const int32 RemovedCount = Cfg.RoomPool.RemoveAll([ToRemove](const TObjectPtr<URoomDataAsset>& Entry)
		{
			return Entry.Get() == ToRemove;
		});
		if (RemovedCount > 0)
		{
			bChanged = true;
		}
	}

	if (bChanged)
	{
		Room->Modify();
	}
	return bChanged;
}

bool KeepOnlyPortalIndices(URoomDataAsset* Room, const TArray<int32>& AllowedPortalIndices)
{
	if (!Room)
	{
		return false;
	}

	const int32 RemovedCount = Room->PortalDestinations.RemoveAll([&AllowedPortalIndices](const FPortalDestConfig& Cfg)
	{
		return !AllowedPortalIndices.Contains(Cfg.PortalIndex);
	});

	if (RemovedCount <= 0)
	{
		return false;
	}

	Room->Modify();
	return true;
}
}

UFirstRunTutorialRoomPoolSetupCommandlet::UFirstRunTutorialRoomPoolSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UFirstRunTutorialRoomPoolSetupCommandlet::Main(const FString& Params)
{
	using namespace FirstRunTutorialRoomPoolSetup;

	URoomDataAsset* Hub          = LoadAssetByPackagePath<URoomDataAsset>(HubRoomPath);
	URoomDataAsset* Room01a      = LoadAssetByPackagePath<URoomDataAsset>(Corridor01aPath);
	URoomDataAsset* Room01b      = LoadAssetByPackagePath<URoomDataAsset>(Corridor01bPath);
	URoomDataAsset* WaterDungeon = LoadAssetByPackagePath<URoomDataAsset>(WaterDungeonRoomPath);
	URoomDataAsset* PrayerRoom   = LoadAssetByPackagePath<URoomDataAsset>(PrayerRoomPath);

	if (!Hub || !Room01a || !Room01b || !WaterDungeon || !PrayerRoom)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FirstRunTutorialRoomPoolSetup] Missing required assets. Hub=%s 01a=%s 01b=%s Water=%s Prayer=%s"),
			*GetNameSafe(Hub),
			*GetNameSafe(Room01a),
			*GetNameSafe(Room01b),
			*GetNameSafe(WaterDungeon),
			*GetNameSafe(PrayerRoom));
		return 1;
	}

	TArray<UPackage*> DirtyPackages;
	const auto MarkDirty = [&DirtyPackages](URoomDataAsset* Room)
	{
		if (Room)
		{
			Room->MarkPackageDirty();
			DirtyPackages.AddUnique(Room->GetPackage());
		}
	};

	// 1) 先清理：把 WaterDungeon 从教程路由房间池里全部移除，再按确定路由重新写入。
	//    这样可以撤销旧版 commandlet 把 WaterDungeon 加到每个池子的副作用。
	TArray<URoomDataAsset*> RoutedTutorialRooms = { Hub, Room01a, Room01b, WaterDungeon };
	for (URoomDataAsset* Room : RoutedTutorialRooms)
	{
		if (RemoveRoomFromAllPortalPools(Room, WaterDungeon))
		{
			MarkDirty(Room);
		}
	}

	// 2) 表格没有列出的传送门池不参与教程链路，清掉旧配置，避免 WaterDungeon portal[1] 等旧池继续生效。
	struct FPortalIndexWhitelist
	{
		URoomDataAsset* Room;
		TArray<int32> AllowedPortalIndices;
	};

	const TArray<FPortalIndexWhitelist> PortalIndexWhitelists = {
		{ Hub,          { 0 } },
		{ Room01a,      { 0, 1 } },
		{ Room01b,      { 0, 1 } },
		{ WaterDungeon, { 0 } },
	};

	for (const FPortalIndexWhitelist& Whitelist : PortalIndexWhitelists)
	{
		if (KeepOnlyPortalIndices(Whitelist.Room, Whitelist.AllowedPortalIndices))
		{
			MarkDirty(Whitelist.Room);
		}
	}

	// 3) 按期望链路写入确定性的 portal[0] / portal[1] 池：
	//    Hub        portal[0] → [01a]
	//    01a        portal[0] → [01b]            portal[1] → [PrayerRoom]
	//    01b        portal[0] → [WaterDungeon]   portal[1] → [PrayerRoom]
	//    WaterDungeon portal[0] → [01a, 01b]
	struct FPortalAssignment
	{
		URoomDataAsset* Room;
		int32 PortalIndex;
		TArray<URoomDataAsset*> Targets;
	};

	const TArray<FPortalAssignment> Assignments = {
		{ Hub,          0, { Room01a } },
		{ Room01a,      0, { Room01b } },
		{ Room01a,      1, { PrayerRoom } },
		{ Room01b,      0, { WaterDungeon } },
		{ Room01b,      1, { PrayerRoom } },
		{ WaterDungeon, 0, { Room01a, Room01b } },
	};

	int32 ChangedPortalConfigs = 0;
	for (const FPortalAssignment& Assignment : Assignments)
	{
		if (SetPortalRoomPool(Assignment.Room, Assignment.PortalIndex, Assignment.Targets))
		{
			++ChangedPortalConfigs;
			MarkDirty(Assignment.Room);
		}
	}

	if (!DirtyPackages.IsEmpty())
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, true);
	}

	UE_LOG(LogTemp, Display,
		TEXT("[FirstRunTutorialRoomPoolSetup] Done. ChangedPortalConfigs=%d DirtyPackages=%d."),
		ChangedPortalConfigs,
		DirtyPackages.Num());

	for (URoomDataAsset* Room : RoutedTutorialRooms)
	{
		if (!Room) continue;
		UE_LOG(LogTemp, Display,
			TEXT("[FirstRunTutorialRoomPoolSetup] Room=%s PortalDestinations=%d."),
			*GetNameSafe(Room),
			Room->PortalDestinations.Num());
		for (const FPortalDestConfig& Cfg : Room->PortalDestinations)
		{
			FString PoolDesc;
			for (const TObjectPtr<URoomDataAsset>& Entry : Cfg.RoomPool)
			{
				if (!PoolDesc.IsEmpty())
				{
					PoolDesc.Append(TEXT(", "));
				}
				PoolDesc.Append(GetNameSafe(Entry.Get()));
			}
			UE_LOG(LogTemp, Display,
				TEXT("[FirstRunTutorialRoomPoolSetup]   Portal=%d RoomPool=[%s]"),
				Cfg.PortalIndex,
				*PoolDesc);
		}
	}

	return 0;
}
