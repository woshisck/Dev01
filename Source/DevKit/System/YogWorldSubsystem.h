// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/List.h"
#include "YogWorldSubsystem.generated.h"


/** Object that is written to and read from the save game archive, with a data version */
UCLASS()
class UYogWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Constructor */
	UYogWorldSubsystem();

	UFUNCTION()
	void InitLevel();



	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category = "WorldLevel")
	UWorld* GetCurrentWorld();

	/** Map of items to item data */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FPrimaryAssetId, FRPGItemData> InventoryData;

	///** Map of slotted items */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FRPGItemSlot, FPrimaryAssetId> SlottedItems;

	/** User's unique id */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TLinkedList<UWorld*> ChapterList;


	//UFUNCTION()
	//void FillChapterList()

};
