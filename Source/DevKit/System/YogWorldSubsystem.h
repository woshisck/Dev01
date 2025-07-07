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
	Default,
	Combat,
	Event,
	Shop,
	Rest,
	Boss
};
USTRUCT(BlueprintType)
struct FLevel2DNode
{
	GENERATED_BODY()
public:
	FLevel2DNode()
	{ 
		LevelPackageName = "DefaultNode"; 
		LevelMapSoftPath = FSoftObjectPath(TEXT("/Game/Maps/Dungeon/RootNode.RootNode"));

		NodeType = ESublevelType::Default;

	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESublevelType NodeType;


	// Name/identifier of the sublevel
	UPROPERTY()
	FName LevelPackageName;


	// Reference to the streaming level (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath LevelMapSoftPath;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> pos_Enter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> pos_Leave;


	void setNodeType(ESublevelType type)
	{
		this->NodeType = type;
	}

};

USTRUCT(BlueprintType)
struct FLevel2DRow
{
	GENERATED_BODY()

public:


	FLevel2DRow() {};

	FLevel2DRow(int32 x)
		:row_length(x)
	{
		FLevel2DNode node = FLevel2DNode();
		rows_levelNode.Init(node, row_length);
	};


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLevel2DNode> rows_levelNode;

	int32 row_length;



	FLevel2DNode& operator [] (int32 i)
	{
		return rows_levelNode[i];
	}

	void Add(FLevel2DNode level2Dnode)
	{
		rows_levelNode.Add(level2Dnode);
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

	void Shuffle(TArray<int32>& array);


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	TArray<int32> GenerateRandomIntegers( int rangeMax, int rangeMin);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Streaming")
	TArray<FLevel2DRow> LevelMatrix;



	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;


	ULevelStreaming* FindLoadedLevel(const FName& LevelName);

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void StartLoadingLevels(const TArray<FName>& LevelsToStream, float DelayBetweenLoads = 0.5f);
	

	UFUNCTION(BlueprintCallable, Category = "Sublevel Tree")
	void InitializeMatrix(int x);

	UFUNCTION()
	void MakeConnections();

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void PrintLevelTree();




	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void OpenLevelAsPersistentAtRuntime(UObject* WorldContextObject, const FString& LevelName);


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void GetAllSubLevel(UObject* WorldContextObject);


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void ClearMatrix();

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
	TArray<FName> PendingLevels;

	FTimerHandle LevelLoadTimerHandle;
	float LoadDelay;

	void LoadNextLevel();

	UFUNCTION()
	void OnLevelLoaded();


	UPROPERTY()
	FSoftObjectPath defaultMapSoftPath;
};



//static ConstructorHelpers::FObjectFinder<UClass> DefaultRootNodeMap(TEXT("Class'/Game/Maps/Dungeon/RootNode.RootNode'"));
//static ConstructorHelpers::FObjectFinder<UClass> DefaultEndNodeMap(TEXT("Class'/Game/Maps/Dungeon/EndNode.EndNode'"));