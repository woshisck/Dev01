// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "Item/ItemInstance.h"


#include "ItemDefinition.generated.h"


class UNiagaraSystem;
class UObject;
class USoundBase;
class UStaticMesh;
class UGameplayEffect;
struct FYogGameplayEffectContainer;





UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon			UMETA(DisplayName = "Weapon"),
	Buffer			UMETA(DisplayName = "Buffer"),
	EventKey		UMETA(DisplayName = "Event Key"),
	Trinket			UMETA(DisplayName = "Trinket")
};

USTRUCT()
struct FItemToSpawn
{
	GENERATED_BODY()

	FItemToSpawn()
	{}

};

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Mesh")
	TSubclassOf<UYogGameplayEffect> RuneGrant;



};

