// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CampaignDataAsset.generated.h"

class URoomDataAsset;

/**
 * UCampaignDataAsset - the room list of one run.
 *
 * Naming: DA_Campaign_<name>, for example DA_Campaign_MainRun.
 */
UCLASS(BlueprintType)
class DEVKIT_API UCampaignDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Layer this campaign belongs to (Room.Layer.L1 / L2 / L3 ...).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
	FGameplayTag LayerTag;

	// Every room a portal may send the player to. Each room carries its own type and layer tags.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPool")
	TArray<TObjectPtr<URoomDataAsset>> RoomPool;

	// Room used when the run starts without a portal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
	TObjectPtr<URoomDataAsset> DefaultStartingRoom;
};
