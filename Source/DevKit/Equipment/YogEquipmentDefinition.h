#pragma once

#include "CoreMinimal.h"
#include "../AbilitySystem/Abilities/YogAbilitySet.h"

#include "YogEquipmentDefinition.generated.h"


class AActor;
class UYogAbilitySet;
class UYogEquipmentInstance;

USTRUCT()
struct FYogEquipmentActorToSpawn {

	GENERATED_BODY()

	FYogEquipmentActorToSpawn(){}

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;
};




UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UYogEquipmentDefinition : public UObject
{
	GENERATED_BODY()
public:
	UYogEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Class to spawn
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<UYogEquipmentInstance> InstanceType;

	//Gameplay ability sets to grant when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const UYogAbilitySet>> AbilitySetsToGrant;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FYogEquipmentActorToSpawn> ActorsToSpawn;
};

