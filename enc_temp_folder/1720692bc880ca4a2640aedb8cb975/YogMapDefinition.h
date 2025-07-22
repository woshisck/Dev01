// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../System/YogWorldSubsystem.h"
#include "YogMapDefinition.generated.h"

/**
* 
 * 
 */

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
	int GateIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	TArray<FNextMapNode> NextLevels;



};

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UYogMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	//TArray<FLevel2DRow> LevelDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	TArray<FPortalEntry> LevelPortals;

};
