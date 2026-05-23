#include "Story/StoryMetadataExportCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "GameplayTagContainer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace StoryMetadataExport
{
const FString DefaultOutputPath = TEXT("Docs/StoryPipeline/Metadata/ue_project_assets.json");

FString ReadOutputPath(const FString& Params)
{
	FString PathValue;
	if (FParse::Value(*Params, TEXT("Output="), PathValue))
	{
		return FPaths::ConvertRelativePathToFull(PathValue);
	}
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), DefaultOutputPath);
}

FString PackagePathForObject(const UObject* Object)
{
	return Object ? Object->GetPackage()->GetName() : FString();
}

TArray<FString> TagsToStrings(const FGameplayTagContainer& Tags)
{
	TArray<FGameplayTag> TagArray;
	Tags.GetGameplayTagArray(TagArray);
	TArray<FString> Out;
	for (const FGameplayTag& Tag : TagArray)
	{
		Out.Add(Tag.ToString());
	}
	Out.Sort();
	return Out;
}

TArray<TSharedPtr<FJsonValue>> StringArrayToJson(const TArray<FString>& Values)
{
	TArray<TSharedPtr<FJsonValue>> Out;
	for (const FString& Value : Values)
	{
		Out.Add(MakeShared<FJsonValueString>(Value));
	}
	return Out;
}

template <typename T>
TArray<T*> CollectAssetsOfClass()
{
	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	RegistryModule.Get().SearchAllAssets(true);

	FARFilter Filter;
	Filter.ClassPaths.Add(T::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	RegistryModule.Get().GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.LexicalLess(B.PackageName);
	});

	TArray<T*> Out;
	for (const FAssetData& AssetData : Assets)
	{
		if (T* Asset = Cast<T>(AssetData.GetAsset()))
		{
			Out.Add(Asset);
		}
	}
	return Out;
}

TSharedPtr<FJsonObject> RoomToJson(const URoomDataAsset* Room)
{
	TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("name"), GetNameSafe(Room));
	Object->SetStringField(TEXT("path"), PackagePathForObject(Room));
	Object->SetStringField(TEXT("roomName"), Room ? Room->RoomName.ToString() : FString());
	Object->SetStringField(TEXT("displayName"), Room ? Room->DisplayName.ToString() : FString());
	Object->SetArrayField(TEXT("tags"), Room ? StringArrayToJson(TagsToStrings(Room->RoomTags)) : TArray<TSharedPtr<FJsonValue>>());
	Object->SetBoolField(TEXT("isHubRoom"), Room ? Room->bIsHubRoom : false);
	Object->SetNumberField(TEXT("enemyCount"), Room ? Room->EnemyPool.Num() : 0);
	Object->SetNumberField(TEXT("buffCount"), Room ? Room->BuffPool.Num() : 0);
	Object->SetNumberField(TEXT("lootCount"), Room ? Room->LootPool.Num() : 0);
	Object->SetNumberField(TEXT("portalCount"), Room ? Room->PortalDestinations.Num() : 0);
	Object->SetBoolField(TEXT("forceSinglePortal"), Room ? Room->bForceSinglePortal : false);
	Object->SetNumberField(TEXT("forcedPortalIndex"), Room ? Room->ForcedPortalIndex : 0);
	Object->SetBoolField(TEXT("useFixedRewardOptions"), Room ? Room->bUseFixedRewardOptions : false);
	Object->SetNumberField(TEXT("fixedRewardCount"), Room ? Room->FixedRewardOptions.Num() : 0);
	return Object;
}

TSharedPtr<FJsonObject> CampaignToJson(const UCampaignDataAsset* Campaign)
{
	TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("name"), GetNameSafe(Campaign));
	Object->SetStringField(TEXT("path"), PackagePathForObject(Campaign));
	Object->SetStringField(TEXT("layerTag"), Campaign && Campaign->LayerTag.IsValid() ? Campaign->LayerTag.ToString() : FString());
	Object->SetNumberField(TEXT("floorCount"), Campaign ? Campaign->FloorTable.Num() : 0);
	Object->SetStringField(TEXT("defaultStartingRoom"), PackagePathForObject(Campaign ? Campaign->DefaultStartingRoom.Get() : nullptr));

	TArray<TSharedPtr<FJsonValue>> RoomPool;
	if (Campaign)
	{
		for (const TObjectPtr<URoomDataAsset>& Room : Campaign->RoomPool)
		{
			RoomPool.Add(MakeShared<FJsonValueString>(PackagePathForObject(Room.Get())));
		}
	}
	Object->SetArrayField(TEXT("roomPool"), RoomPool);

	TArray<TSharedPtr<FJsonValue>> Floors;
	if (Campaign)
	{
		for (int32 Index = 0; Index < Campaign->FloorTable.Num(); ++Index)
		{
			const FFloorConfig& Floor = Campaign->FloorTable[Index];
			TSharedPtr<FJsonObject> FloorObject = MakeShared<FJsonObject>();
			FloorObject->SetNumberField(TEXT("index"), Index);
			FloorObject->SetNumberField(TEXT("totalDifficultyScore"), Floor.TotalDifficultyScore);
			FloorObject->SetBoolField(TEXT("forceElite"), Floor.bForceElite);
			FloorObject->SetNumberField(TEXT("eliteChance"), Floor.EliteChance);
			FloorObject->SetNumberField(TEXT("shopChance"), Floor.ShopChance);
			FloorObject->SetNumberField(TEXT("eventChance"), Floor.EventChance);
			FloorObject->SetStringField(TEXT("globalStageTag"), Floor.GlobalStageTag.IsValid() ? Floor.GlobalStageTag.ToString() : FString());
			FloorObject->SetArrayField(TEXT("storyEventTags"), StringArrayToJson(TagsToStrings(Floor.StoryEventTags)));
			Floors.Add(MakeShared<FJsonValueObject>(FloorObject));
		}
	}
	Object->SetArrayField(TEXT("floors"), Floors);
	return Object;
}
}

UStoryMetadataExportCommandlet::UStoryMetadataExportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UStoryMetadataExportCommandlet::Main(const FString& Params)
{
	using namespace StoryMetadataExport;

	const FString OutputPath = ReadOutputPath(Params);
	TArray<TSharedPtr<FJsonValue>> Rooms;
	for (const URoomDataAsset* Room : CollectAssetsOfClass<URoomDataAsset>())
	{
		Rooms.Add(MakeShared<FJsonValueObject>(RoomToJson(Room)));
	}

	TArray<TSharedPtr<FJsonValue>> Campaigns;
	for (const UCampaignDataAsset* Campaign : CollectAssetsOfClass<UCampaignDataAsset>())
	{
		Campaigns.Add(MakeShared<FJsonValueObject>(CampaignToJson(Campaign)));
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schemaVersion"), 1);
	Root->SetStringField(TEXT("generatedAt"), FDateTime::UtcNow().ToIso8601());
	Root->SetStringField(TEXT("source"), TEXT("ue-commandlet"));
	Root->SetArrayField(TEXT("roomData"), Rooms);
	Root->SetArrayField(TEXT("campaigns"), Campaigns);

	TSharedPtr<FJsonObject> Summary = MakeShared<FJsonObject>();
	Summary->SetNumberField(TEXT("roomData"), Rooms.Num());
	Summary->SetNumberField(TEXT("campaigns"), Campaigns.Num());
	Root->SetObjectField(TEXT("summary"), Summary);

	FString JsonText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
	if (!FFileHelper::SaveStringToFile(JsonText, *OutputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[StoryMetadataExport] Failed to write %s"), *OutputPath);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[StoryMetadataExport] Wrote %s (%d room(s), %d campaign(s))"),
		*OutputPath, Rooms.Num(), Campaigns.Num());
	return 0;
}
