// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "ItemInstance.h"


#include "ItemDefinition.generated.h"


class UNiagaraSystem;
class UObject;
class USoundBase;
class UStaticMesh;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon			UMETA(DisplayName = "Weapon"),
	Buffer			UMETA(DisplayName = "Buffer"),
	EventKey		UMETA(DisplayName = "Event Key"),
	Trinket			UMETA(DisplayName = "Trinket")
};


UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Mesh")
	TSubclassOf<UItemInstance> InstanceType;


	////////////////////Visual representation of the pickup////////////////////
	//Particle FX to play when picked up
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDefine | Display")
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	//Particle FX to play when pickup is respawned
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDefine | Display")
	TObjectPtr<UNiagaraSystem> RespawnedEffect;

	//GameplayEffect for the pickup
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDefine | Display")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDefine | Display")
	TObjectPtr<UStaticMesh> DisplayMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDefine | Display")
	FVector ItemMeshOffset;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemDefine | Display")
	FVector ItemMeshScale = FVector(1.0f, 1.0f, 1.0f);

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditAnywhere, Category = "ItemDefine | Type")
	EItemType ItemType;



	//Visual representation of the pickup
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Mesh")
	//TObjectPtr<UItemDisplayData> DisplayData;

};

