#pragma once

#include "CoreMinimal.h"
#include "../AbilitySystem/Abilities/YogAbilitySet.h"

#include "ItemInstance.generated.h"

USTRUCT()
struct FItemActorToSpawn
{
	GENERATED_BODY()

	FItemActorToSpawn()
	{}

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;

};


UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UItemInstance : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this empty's properties
	UItemInstance();


	// Gameplay ability sets to grant when this is equipped
	UPROPERTY(EditAnywhere, Category = Equipment)
	TArray<TObjectPtr<UYogAbilitySet>> AbilitySetsToGrant;


	UPROPERTY(EditAnywhere, Category = Equipment)
	TArray<FItemActorToSpawn> actorToSpawn;



	UFUNCTION(BlueprintImplementableEvent, Category = "ID | Equip")
	void EquipItem(APawn* Pawn);
};
