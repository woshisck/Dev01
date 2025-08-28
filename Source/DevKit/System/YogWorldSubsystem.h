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
struct FLevelTreeNode
{
	GENERATED_BODY()


public:
	FString LevelName;
	TArray<TSharedPtr<FLevelTreeNode>> Children;
	ESublevelType NodeType;

	UPROPERTY()
	FName LevelPackageName;


	// Reference to the streaming level (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath LevelMapSoftPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> pos_Enter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> pos_Leave;

	FLevelTreeNode()
	{
		LevelPackageName = "DefaultNode";
		LevelMapSoftPath = FSoftObjectPath(TEXT("/Game/Maps/Dungeon/RootNode.RootNode"));

		NodeType = ESublevelType::Default;

	}

	void AddChild(TSharedPtr<FLevelTreeNode> Child)
	{
		Children.Add(Child);
	}
};


///////////////////////////////////////////// 2D Matrix Level /////////////////////////////////////////////
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

	UPROPERTY(BlueprintReadOnly)
	int x = 0;


	UPROPERTY(BlueprintReadOnly)
	int y = 0;


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

	FLevel2DRow(int32 x, int32 y)
		:row_length(x)
	{
		
		for (int i = 0; i < x; i++)
		{
			FLevel2DNode node = FLevel2DNode();
			node.x = i;
			node.y = y;
			rows_levelNode.Add(node);
		}
		//rows_levelNode.Init(node, row_length);
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

	int Num()
	{
		return rows_levelNode.Num();
	}

};
///////////////////////////////////////////// 2D Matrix Level /////////////////////////////////////////////

/** Object that is written to and read from the save game archive, with a data version */

class AYogLevelScript;

UCLASS()
class UYogWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Constructor */
	UYogWorldSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;


	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	TArray<int32> GenerateRandomIntegers( int rangeMax, int rangeMin);

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void StartLoadingLevels(const TArray<FName>& LevelsToStream, float DelayBetweenLoads = 0.5f);
	
	////////////////////////////////////////////////// Level Matrix //////////////////////////////////////////////////
	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	TArray<FLevel2DRow>& GetLevelMatrix();


	UFUNCTION(BlueprintCallable, Category = "Sublevel Tree")
	void InitializeMatrix(int x, int y);

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void ClearMatrix();

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void SetupMatrixElement(int x, int y, ESublevelType Type, FSoftObjectPath soft_object_path);

	void Shuffle(TArray<int32>& array);

	UFUNCTION()
	bool checkRange(int index);
	////////////////////////////////////////////////// Level Matrix //////////////////////////////////////////////////

	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void OpenLevelAsPersistentAtRuntime(UObject* WorldContextObject, const FString& LevelName);




	UFUNCTION(BlueprintCallable, Category = "Level Streaming")
	void GetAllSubLevel(UObject* WorldContextObject);



	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;


	ULevelStreaming* FindLoadedLevel(const FName& LevelName);


protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Streaming")
	TArray<FLevel2DRow> LevelMatrix;


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

	UFUNCTION(BlueprintCallable)
	AYogLevelScript* GetCurrentLevelScript();


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