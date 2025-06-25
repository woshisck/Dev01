// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Array.h"
#include "Containers/List.h"


#include "YogWorldSubsystem.generated.h"


USTRUCT(BlueprintType)
struct FSublevelTreeNode
{
	GENERATED_BODY()

	// Name/identifier of the sublevel
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SublevelName;

	// Reference to the streaming level (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> SublevelReference;

	// Child nodes
	//UPROPERTY()
	TLinkedList<FSublevelTreeNode*> Children;

	// Stats/data you want to track
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float LoadTime = 0.0f;


	// Add more stats as needed
};

/** Object that is written to and read from the save game archive, with a data version */
UCLASS()
class UYogWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Constructor */
	UYogWorldSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;



	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;


	ULevelStreaming* FindLoadedLevel(const FName& LevelName);

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void StartLoadingLevels(const TArray<FName>& LevelsToStream, float DelayBetweenLoads = 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sublevel Tree")
	FSublevelTreeNode RootNode;


protected:

	UPROPERTY()
	TArray<ULevelStreaming*> LoadedLevels;

	///** Map of items to item data */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FPrimaryAssetId, FRPGItemData> InventoryData;

	///** Map of slotted items */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FRPGItemSlot, FPrimaryAssetId> SlottedItems;



	UFUNCTION(BlueprintCallable)
	UWorld* GetCurrentWorld();

	UFUNCTION(BlueprintCallable)
	void UnloadAllStreamLevels();


private:


	TArray<FName> PendingLevels;

	FTimerHandle LevelLoadTimerHandle;
	float LoadDelay;

	void LoadNextLevel();

	UFUNCTION()
	void OnLevelLoaded();
};
