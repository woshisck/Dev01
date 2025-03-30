// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Array.h"
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
	void Init();



	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category = "WorldLevel")
	UWorld* GetCurrentWorld();

	void HandleLoadFinished();



protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonLevel")
	TArray<TSoftObjectPtr<UWorld>> DungeonMaps;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonLevel")
	int32 CurrentMapIndex;

	///** Map of items to item data */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FPrimaryAssetId, FRPGItemData> InventoryData;

	///** Map of slotted items */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FRPGItemSlot, FPrimaryAssetId> SlottedItems;

	/** User's unique id */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Gamelevel)
	TArray<UWorld*> DungeonLevels;

	TLinkedList<UWorld*> DungeonLinkedList;

	UFUNCTION(BlueprintCallable)
	void InitDungeonMap();


	UFUNCTION(BlueprintCallable)
	void LoadDungeonMap(TSoftObjectPtr<UWorld>& Map);

};
