// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "YogEquipmentDefinition.h"


#include "YogPickupDefinition.generated.h"


class UNiagaraSystem;
class UObject;
class USoundBase;
class UStaticMesh;

/**
 *
 */
UCLASS(Blueprintable, BlueprintType, Const, Meta = (DisplayName = "Pickup Data", ShortTooltip = "Data asset used to configure a pickup."))
class DEVKIT_API UYogPickupDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	//Visual representation of the pickup
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;

	//Cool down time between pickups in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	int32 SpawnCoolDownSeconds;

	//Sound to play when picked up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USoundBase> PickedUpSound;

	//Sound to play when pickup is respawned
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USoundBase> RespawnedSound;

	//Particle FX to play when picked up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	//Particle FX to play when pickup is respawned
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UNiagaraSystem> RespawnedEffect;
};


UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UYogWeaponPickupDefinition : public UYogPickupDefinition
{
	GENERATED_BODY()

public:

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshOffset;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Amine")
	TObjectPtr<UAnimMontage> PickupMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup|WeaponDefiniton")
	TObjectPtr<UYogEquipmentDefinition> WeaponDefinition;
};
