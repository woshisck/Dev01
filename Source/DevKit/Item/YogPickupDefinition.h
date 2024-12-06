// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"



#include "YogPickupDefinition.generated.h"


class UNiagaraSystem;
class UObject;
class USoundBase;
class UStaticMesh;
class UGameplayEffect;


UCLASS(Blueprintable, BlueprintType, Const, Meta = (DisplayName = "Pickup Data", ShortTooltip = "Data asset used to configure a pickup."))
class DEVKIT_API UYogPickupDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	//Visual representation of the pickup
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;

	//Particle FX to play when picked up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	//Particle FX to play when pickup is respawned
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UNiagaraSystem> RespawnedEffect;

	//GameplayEffect for the pickup
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GE")
	TSubclassOf<UGameplayEffect> GameplayEffect;
};

