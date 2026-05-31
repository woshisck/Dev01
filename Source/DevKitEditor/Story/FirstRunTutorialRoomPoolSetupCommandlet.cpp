#include "Story/FirstRunTutorialRoomPoolSetupCommandlet.h"

#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace FirstRunTutorialRoomPoolSetup
{
const FString TutorialCampaignPath = TEXT("/Game/Docs/Map/DA_Campaign_Tutorial");
const FString WaterDungeonRoomPath = TEXT("/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon");

FString ToObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

template<typename AssetT>
AssetT* LoadAssetByPackagePath(const FString& PackagePath)
{
	return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
}

bool AddRoomUnique(TArray<TObjectPtr<URoomDataAsset>>& RoomPool, URoomDataAsset* Room)
{
	if (!Room)
	{
		return false;
	}

	for (const TObjectPtr<URoomDataAsset>& Existing : RoomPool)
	{
		if (Existing.Get() == Room)
		{
			return false;
		}
	}

	RoomPool.Add(Room);
	return true;
}

URoomDataAsset* FindPortalTemplateRoom(const TArray<TObjectPtr<URoomDataAsset>>& RoomPool, const URoomDataAsset* ExcludedRoom)
{
	for (const TObjectPtr<URoomDataAsset>& RoomPtr : RoomPool)
	{
		URoomDataAsset* Room = RoomPtr.Get();
		if (Room && Room != ExcludedRoom && !Room->PortalDestinations.IsEmpty())
		{
			return Room;
		}
	}
	return nullptr;
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

	UCampaignDataAsset* Campaign = LoadAssetByPackagePath<UCampaignDataAsset>(TutorialCampaignPath);
	URoomDataAsset* WaterDungeon = LoadAssetByPackagePath<URoomDataAsset>(WaterDungeonRoomPath);
	if (!Campaign || !WaterDungeon)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FirstRunTutorialRoomPoolSetup] Missing required assets. Campaign=%s WaterDungeon=%s"),
			*GetNameSafe(Campaign),
			*GetNameSafe(WaterDungeon));
		return 1;
	}

	TArray<UPackage*> DirtyPackages;
	int32 AddedGlobal = 0;
	int32 AddedPortalEntries = 0;
	int32 CopiedPortalDestinations = 0;

	Campaign->Modify();
	if (AddRoomUnique(Campaign->RoomPool, WaterDungeon))
	{
		++AddedGlobal;
		Campaign->MarkPackageDirty();
		DirtyPackages.AddUnique(Campaign->GetPackage());
	}

	TArray<TObjectPtr<URoomDataAsset>> RoomsToPatch = Campaign->RoomPool;
	if (Campaign->DefaultStartingRoom)
	{
		AddRoomUnique(RoomsToPatch, Campaign->DefaultStartingRoom.Get());
	}

	if (WaterDungeon->PortalDestinations.IsEmpty())
	{
		if (URoomDataAsset* TemplateRoom = FindPortalTemplateRoom(RoomsToPatch, WaterDungeon))
		{
			WaterDungeon->Modify();
			WaterDungeon->PortalDestinations = TemplateRoom->PortalDestinations;
			CopiedPortalDestinations = WaterDungeon->PortalDestinations.Num();
			WaterDungeon->MarkPackageDirty();
			DirtyPackages.AddUnique(WaterDungeon->GetPackage());
			UE_LOG(LogTemp, Display,
				TEXT("[FirstRunTutorialRoomPoolSetup] Copied %d PortalDestinations from %s to %s."),
				CopiedPortalDestinations,
				*GetNameSafe(TemplateRoom),
				*GetNameSafe(WaterDungeon));
		}
	}

	for (const TObjectPtr<URoomDataAsset>& RoomPtr : RoomsToPatch)
	{
		URoomDataAsset* Room = RoomPtr.Get();
		if (!Room || Room->PortalDestinations.IsEmpty())
		{
			continue;
		}

		bool bRoomChanged = false;
		Room->Modify();
		for (FPortalDestConfig& PortalDest : Room->PortalDestinations)
		{
			if (AddRoomUnique(PortalDest.RoomPool, WaterDungeon))
			{
				++AddedPortalEntries;
				bRoomChanged = true;
			}
		}

		if (bRoomChanged)
		{
			Room->MarkPackageDirty();
			DirtyPackages.AddUnique(Room->GetPackage());
		}
	}

	if (!DirtyPackages.IsEmpty())
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, true);
	}

	UE_LOG(LogTemp, Display,
		TEXT("[FirstRunTutorialRoomPoolSetup] Campaign=%s WaterDungeon=%s AddedGlobal=%d AddedPortalEntries=%d CopiedPortalDestinations=%d DirtyPackages=%d."),
		*GetNameSafe(Campaign),
		*GetNameSafe(WaterDungeon),
		AddedGlobal,
		AddedPortalEntries,
		CopiedPortalDestinations,
		DirtyPackages.Num());

	for (const TObjectPtr<URoomDataAsset>& RoomPtr : RoomsToPatch)
	{
		const URoomDataAsset* Room = RoomPtr.Get();
		if (!Room)
		{
			continue;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[FirstRunTutorialRoomPoolSetup] Room=%s PortalDestinations=%d."),
			*GetNameSafe(Room),
			Room->PortalDestinations.Num());
		for (const FPortalDestConfig& PortalDest : Room->PortalDestinations)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[FirstRunTutorialRoomPoolSetup]   Portal=%d RoomPool=%d HasWaterDungeon=%d."),
				PortalDest.PortalIndex,
				PortalDest.RoomPool.Num(),
				PortalDest.RoomPool.Contains(WaterDungeon) ? 1 : 0);
		}
	}

	return 0;
}
