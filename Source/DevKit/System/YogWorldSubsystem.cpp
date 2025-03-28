// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogWorldSubsystem.h"
#include <Engine/AssetManager.h>
#include <DevKit/DevAssetManager.h>



UYogWorldSubsystem::UYogWorldSubsystem()
{
	CurrentMapIndex = 0;
}

void UYogWorldSubsystem::Init()
{
}

void UYogWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	
	UE_LOG(LogTemp, Warning,TEXT("YogSubsystem onWorldBeginPlay"));
	ULevel* CurrentLevel;
	

	UWorld* world = GetCurrentWorld();

	CurrentLevel = world->GetCurrentLevel();

	if (world != nullptr)
	{
		FName worldName;
		worldName = world->GetFName();
		
		UE_LOG(LogTemp, Warning, TEXT("WORLD: %s"), *worldName.ToString());
	}


}

UWorld* UYogWorldSubsystem::GetCurrentWorld()
{
	UWorld* world = nullptr;
	FName worldName;
#if WITH_EDITOR
	world = GEditor->GetEditorWorldContext().World();
	return world;
#else
	world = GEngine->GetCurrentPlayWorld();
	return world;
#endif
	


//#if WITH_EDITOR
//	if (GIsEditor)
//	{
//		if (GPlayInEditorID == -1)
//		{
//			FWorldContext* worldContext = GEditor->GetPIEWorldContext(1);
//			if (worldContext == nullptr)
//			{
//				if (UGameViewportClient* viewport = GEngine->GameViewport)
//				{
//					world = viewport->GetWorld();
//				}
//			}
//			else
//			{
//				world = worldContext->World();
//			}
//		}
//		else
//		{
//			FWorldContext* worldContext = GEditor->GetPIEWorldContext(GPlayInEditorID);
//			if (worldContext == nullptr)
//			{
//				return nullptr;
//			}
//			world = worldContext->World();
//		}
//	}
//	else
//	{
//		world = GEngine->GetCurrentPlayWorld(nullptr);
//	}
//	worldName = world->GetFName();
//	UE_LOG(LogTemp, Warning, TEXT("WORLD: %s"), *worldName.ToString());
//#else
//	world = GEngine->GetCurrentPlayWorld(nullptr);
//#endif
//	world = GEngine->GetWorld();
//	worldName = world->GetFName();
//	UE_LOG(LogTemp, Warning, TEXT("WORLD: %s"),*worldName.ToString());

}

void UYogWorldSubsystem::HandleLoadFinished()
{
	UE_LOG(LogTemp, Log, TEXT("Load finished event triggered."));
}



void UYogWorldSubsystem::InitDungeonMap()
{

}

void UYogWorldSubsystem::LoadNextDungeonMap(TSoftObjectPtr<UWorld>& Map)
{
	UDevAssetManager* AssetManager = Cast<UDevAssetManager>(UAssetManager::GetIfValid());

	if (DungeonMaps.IsValidIndex(CurrentMapIndex))
	{
		UWorld* currentWorld = GetWorld();
		TSoftObjectPtr<UWorld > CurrentMap = DungeonMaps[CurrentMapIndex];
		UDevAssetManager::AsyncLoadAsset<UWorld>(TEXT("Texture2D'/Game/Path/To/Your/Texture.Texture'"), [](UWorld* LoadedMap)
			{
				if (LoadedMap)
				{
					// Do something with the loaded texture
					UE_LOG(LogTemp, Log, TEXT("Texture Loaded Successfully!"));
				}
			});



		//TArray<TSoftObjectPtr<UWorld>> DungeonMaps;
		//AssetManager->FuncAsyncLoadAsset<UWorld>(Map, [](UWorld* LoadedTexture)
		//{
		//	
		//});

		CurrentMapIndex++;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No more maps to load or invalid index."));
	}
	//if (DungeonMaps.IsValidIndex(CurrentMapIndex))
	//{
	//	UGameplayStatics::OpenLevel(this, DungeonMaps[CurrentMapIndex]);
	//	CurrentMapIndex++;  // Move to the next map in the list
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("No more maps to load or invalid index."));
	//}
}
