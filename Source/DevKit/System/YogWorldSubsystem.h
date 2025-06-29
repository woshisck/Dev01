// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Array.h"
#include "Containers/List.h"

#include "YogWorldSubsystem.generated.h"

UCLASS()
class USublevelTreeNode : public UObject
{
	GENERATED_BODY()

	USublevelTreeNode() { Depth = 0; };

public:
	// Name/identifier of the sublevel
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SublevelName;

	// Reference to the streaming level (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> SublevelReference;

	// Child nodes
	//UPROPERTY()
	TArray<USublevelTreeNode*> ChildrenNode;

	TArray<USublevelTreeNode*> ParentNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Depth;

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
	TObjectPtr<USublevelTreeNode> RootNode;

	UFUNCTION(BlueprintCallable, Category = "Sublevel Tree")
	void InitializeTree(const FName& RootSublevelName);

	// Add a child node
	UFUNCTION(BlueprintCallable, Category = "Sublevel Tree")
	bool AddChildNode(const FName& ParentName, USublevelTreeNode* NewNode);

	UFUNCTION()
	USublevelTreeNode* FindNodeByName(USublevelTreeNode* CurrentNode, const FName& SearchName);

	UFUNCTION()
	void UpdateNodeStats(const FName& NodeName, float NewLoadTime, float NewMemoryUsage);

	UFUNCTION(BlueprintCallable, Category = "Sublevel Tree")
	void InitMapTree();

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void OpenLevelAsPersistentAtRuntime(UObject* WorldContextObject, const FString& LevelName);


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void GetAllSubLevel(UObject* WorldContextObject);

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

	UFUNCTION(BlueprintCallable)
	void LoadStreamLevel(ULevelStreaming* StreamingLevel);


private:

	//Change 
	//static ConstructorHelpers::FObjectFinder<UClass> DefaultReticleClass(TEXT("Class'/Game/Characters/Global/Reticles/BP_Reticle_AbilityTargeting.BP_Reticle_AbilityTargeting_C'"));
	FString  path_RootNode = TEXT("Class'/Game/Maps/Dungeon/RootNode.RootNode'");
	FString  path_EndNode = TEXT("Class'/Game/Maps/Dungeon/EndNode.EndNode'");

	TArray<FName> PendingLevels;

	FTimerHandle LevelLoadTimerHandle;
	float LoadDelay;

	void LoadNextLevel();

	UFUNCTION()
	void OnLevelLoaded();
};
