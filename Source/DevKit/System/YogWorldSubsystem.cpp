// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogWorldSubsystem.h"
#include <Engine/AssetManager.h>
#include <DevKit/DevAssetManager.h>



UYogWorldSubsystem::UYogWorldSubsystem()
	:UWorldSubsystem()
{

}

void UYogWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(DevKitLevelSystem, Display, TEXT("INIT YogWorldSubsystem"));
}



void UYogWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	
	UE_LOG(DevKitLevelSystem, Display,TEXT("YogSubsystem onWorldBeginPlay"));
	ULevel* CurrentLevel;
	

	UWorld* world = GetCurrentWorld();
	if (world)
	{
		CurrentLevel = world->GetCurrentLevel();


		FName worldName;
		worldName = world->GetFName();

		UE_LOG(DevKitLevelSystem, Display, TEXT("WORLD: %s"), *worldName.ToString());

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

}


void UYogWorldSubsystem::UnloadAllStreamLevels()
{
	for (ULevelStreaming* Level : LoadedLevels)
	{
		if (Level)
		{
			FLatentActionInfo LatentInfo;
			UGameplayStatics::UnloadStreamLevel(
				GetWorld(),
				Level->GetWorldAssetPackageFName(),
				LatentInfo,
				false
			);
		}
	}
	LoadedLevels.Empty(); // Clear the array
}

void UYogWorldSubsystem::LoadStreamLevel(ULevelStreaming* StreamingLevel)
{
	if (StreamingLevel)
	{
		StreamingLevel->SetShouldBeLoaded(true);
		StreamingLevel->SetShouldBeVisible(true);

		//StreamingLevel->bShouldBlockOnLoad = true; // Block until loaded
		//StreamingLevel->RequestLevel();
	}
}

void UYogWorldSubsystem::LoadNextLevel()
{
	if (PendingLevels.Num() == 0)
	{
		UE_LOG(DevKitLevelSystem, Display, TEXT("All levels loaded!"));
		return;
	}

	FName NextLevel = PendingLevels[0];
	PendingLevels.RemoveAt(0);

	UE_LOG(LogTemp, Display, TEXT("Loading level: %s"), *NextLevel.ToString());

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = "OnLevelLoaded";
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = 0;

	// Load the level asynchronously
	UGameplayStatics::LoadStreamLevel(
		GetWorld(),
		NextLevel,
		true,    // Make visible
		false,   // Don't block on load
		LatentInfo
	);
}

void UYogWorldSubsystem::OnLevelLoaded()
{
	ULevelStreaming* LastLoaded = nullptr;
	for (ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
	{
		if (Level && !LoadedLevels.Contains(Level) && Level->IsLevelLoaded())
		{
			LastLoaded = Level;
			break;
		}
	}

	if (LastLoaded)
	{
		LoadedLevels.Add(LastLoaded);
		UE_LOG(DevKitLevelSystem, Display, TEXT("Successfully loaded: %s"),
			*LastLoaded->GetWorldAssetPackageFName().ToString());
	}

	// Schedule next load if there are more levels
	if (PendingLevels.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			LevelLoadTimerHandle,
			this,
			&UYogWorldSubsystem::LoadNextLevel,
			LoadDelay,
			false
		);
	}
}



ULevelStreaming* UYogWorldSubsystem::FindLoadedLevel(const FName& LevelName)
{
	for (ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
	{
		if (Level && Level->GetWorldAssetPackageFName() == LevelName)
		{
			return Level;
		}
	}
	return nullptr;
}

void UYogWorldSubsystem::StartLoadingLevels(const TArray<FName>& LevelsToStream, float DelayBetweenLoads)
{
	// Clear any existing loads
	GetWorld()->GetTimerManager().ClearTimer(LevelLoadTimerHandle);
	PendingLevels.Empty();

	// Set up new load sequence
	PendingLevels = LevelsToStream;
	LoadDelay = DelayBetweenLoads;

	// Start loading first level immediately
	LoadNextLevel();
}

void UYogWorldSubsystem::InitializeTree(const FName& RootSublevelName)
{
	RootNode->SublevelName = RootSublevelName;
	RootNode->ChildrenNode.Empty();
	InitMapTree();
}

bool UYogWorldSubsystem::AddChildNode(const FName& ParentName, USublevelTreeNode* NewNode)
{
	USublevelTreeNode* Parent = FindNodeByName(RootNode, ParentName);
	if (Parent)
	{
		Parent->ChildrenNode.Add(NewNode);
		return true;
	}
	return false;
}

USublevelTreeNode* UYogWorldSubsystem::FindNodeByName(USublevelTreeNode* CurrentNode, const FName& SearchName)
{
	USublevelTreeNode* obj = NewObject<USublevelTreeNode>();

	if (CurrentNode->SublevelName == SearchName)
	{
		return CurrentNode;
	}

	for (USublevelTreeNode* Child : CurrentNode->ChildrenNode)
	{
		USublevelTreeNode* FoundNode = FindNodeByName(Child, SearchName);
		if (FoundNode)
		{
			return FoundNode;
		}
	}

	return nullptr;

}

void UYogWorldSubsystem::UpdateNodeStats(const FName& NodeName, float NewLoadTime, float NewMemoryUsage)
{
	USublevelTreeNode* Node = FindNodeByName(RootNode, NodeName);
	if (Node)
	{

	}
}

void UYogWorldSubsystem::InitMapTree()
{
	RootNode = NewObject<USublevelTreeNode>(this);
	RootNode->Depth = 0;
	RootNode->SublevelName = "RootNode";
	//USublevelTreeNode* ForestNode;
	//ForestNode->SublevelName = "Forest";
	//SublevelTreeManager->AddChildNode("PersistentLevel", ForestNode);
}

void UYogWorldSubsystem::OpenLevelAsPersistentAtRuntime(UObject* WorldContextObject, const FString& LevelName)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	// This will replace the current persistent level
	UGameplayStatics::OpenLevel(World, FName(*LevelName), true);
}

void UYogWorldSubsystem::GetAllSubLevel(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		// Get all streaming sublevels attached to the persistent level
		const TArray<ULevelStreaming*>& StreamingLevels = World->GetStreamingLevels();

		for (ULevelStreaming* StreamingLevel : StreamingLevels)
		{
			if (StreamingLevel)
			{
				FLatentActionInfo LatentInfo;
				/*UGameplayStatics::LoadStreamLevel(WorldContextObject, *LevelPackageName, true, true, LatentInfo);*/
				StreamingLevel->bShouldBlockOnLoad = true;
				StreamingLevel->SetShouldBeLoaded(true);
				StreamingLevel->SetShouldBeVisible(true);

				// This is the full package name of the level asset
				FString LevelPackageName = StreamingLevel->GetWorldAssetPackageName();




				UE_LOG(LogTemp, Log, TEXT("Sublevel package name: %s"), *LevelPackageName);

				// Check if level is loaded
				ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();
				if (LoadedLevel)
				{
					UE_LOG(LogTemp, Log, TEXT("Sublevel is loaded."));
					// You can access level content here
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("Sublevel is NOT loaded."));
				}
			}
		}
	}

}


//////////////////////////////////////////////////////////////////////////////////////

