// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Array.h"
#include "Containers/List.h"

#include "YogWorldSubsystem.generated.h"


UENUM(BlueprintType)
enum class ESublevelType : uint8
{
	Combat,
	Event,
	Shop,
	Rest,
	Boss
};

USTRUCT(BlueprintType)
struct FSublevelTreeNode
{
	GENERATED_BODY()

	FSublevelTreeNode() = default;

	FSublevelTreeNode(FName InPackageName, const FString& InDisplayName) 
		:LevelPackageName(InPackageName){};

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESublevelType NodeType;


	// Name/identifier of the sublevel
	UPROPERTY()
	FName LevelPackageName;

	UPROPERTY()
	FName DisplayName;
	 
	// Reference to the streaming level (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> SublevelReference;

	// Child nodes
	//UPROPERTY()
	TArray<FSublevelTreeNode> ChildrenNode;

	TArray<FSublevelTreeNode> ParentNode;

	UPROPERTY(Transient)
	ULevelStreaming* StreamingLevel = nullptr;

	void ClearChildrenNode()
	{
		for (FSublevelTreeNode Child : ChildrenNode)
		{
			Child.ClearChildrenNode();
		}
		ChildrenNode.Empty();
	}

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
	
	UPROPERTY()
	FSublevelTreeNode map_RootNode;
	

	UFUNCTION(BlueprintCallable, Category = "Sublevel Tree")
	void InitializeTree(const FName& RootSublevelName);



	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void PrintLevelTree();


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void OpenLevelAsPersistentAtRuntime(UObject* WorldContextObject, const FString& LevelName);


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void GetAllSubLevel(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void BuildSampleTree();

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void ClearTree();

protected:

	void PrintLevelTree_Internal(const FSublevelTreeNode& Node, int32 Depth = 0);

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
	TArray<FName> PendingLevels;

	FTimerHandle LevelLoadTimerHandle;
	float LoadDelay;

	void LoadNextLevel();

	UFUNCTION()
	void OnLevelLoaded();
};



//static ConstructorHelpers::FObjectFinder<UClass> DefaultRootNodeMap(TEXT("Class'/Game/Maps/Dungeon/RootNode.RootNode'"));
//static ConstructorHelpers::FObjectFinder<UClass> DefaultEndNodeMap(TEXT("Class'/Game/Maps/Dungeon/EndNode.EndNode'"));