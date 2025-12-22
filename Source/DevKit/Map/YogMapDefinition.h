// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../System/YogWorldSubsystem.h"
#include "DevKit/Enemy/EnemyCharacterBase.h"
#include "YogMapDefinition.generated.h"

/**
* 
 * 
 */

UENUM(BlueprintType)
enum class EEnemySpawnRule : uint8
{
	OneByOne	UMETA(DisplayName = "OneByOne"),
	AllInOnce	UMETA(DisplayName = "AllInOnce"),
	Wave		UMETA(DisplayName = "Wave"),
	Default		UMETA(DisplayName = "Default")
};



USTRUCT(BlueprintType)
struct FNextMapNode
{
	GENERATED_BODY()
public:
	FNextMapNode()
	{
		//LevelMapSoftPath = FSoftObjectPath(TEXT("/Game/Maps/Dungeon/RootNode.RootNode"));

		NodeType = ESublevelType::Default;

	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESublevelType NodeType;



	// Reference to the streaming level (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> LevelMapSoftPtr;


	void setNodeType(ESublevelType type)
	{
		this->NodeType = type;
	}

};

USTRUCT(BlueprintType)
struct FPortalEntry {
	
	GENERATED_BODY()
public:

	FPortalEntry() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	int GateIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	TArray<FNextMapNode> NextLevels;

};

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UYogMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Mob")
	EEnemySpawnRule EnemySpawnRule;


	//OneByOne	UMETA(DisplayName = "OneByOne"),
	//AllInOnce	UMETA(DisplayName = "AllInOnce"),
	//Wave		UMETA(DisplayName = "Wave"),
	//Default		UMETA(DisplayName = "Default")

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EnemySpawnRule == EEnemySpawnRule::OneByOne || EnemySpawnRule == EEnemySpawnRule::AllInOnce"))
	int TotalCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EnemySpawnRule == EEnemySpawnRule::OneByOne"))
	float SpawnDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EnemySpawnRule == EEnemySpawnRule::Wave"))
	int WaveCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EnemySpawnRule == EEnemySpawnRule::Wave"))
	int NumPerWave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EnemySpawnRule == EEnemySpawnRule::AllInOnce"))
	int NumCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AEnemyCharacterBase> MobSpawn;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Mob")
	int	DifficultPoint;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	//TArray<FPortalEntry> LevelPortals;

};
