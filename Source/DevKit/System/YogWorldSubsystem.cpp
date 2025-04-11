// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogWorldSubsystem.h"
#include <Engine/AssetManager.h>
#include <DevKit/DevAssetManager.h>



UYogWorldSubsystem::UYogWorldSubsystem()
{

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

void UYogWorldSubsystem::LoadNextLevel()
{
	if (PendingLevels.Num() == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("All levels loaded!"));
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
		UE_LOG(LogTemp, Display, TEXT("Successfully loaded: %s"),
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