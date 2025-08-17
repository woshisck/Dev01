
#pragma once

#include "CoreMinimal.h"
#include "..\..\AbilitySystem\Abilities\YogAbilitySet.h"

#include "WeaponDefinition.generated.h"

class UYogAbilitySet;
class AWeaponInstance;
class UYogAnimInstance;


USTRUCT()
struct FWeaponActorToSpawn
{
	GENERATED_BODY()

	FWeaponActorToSpawn()
	{}

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AWeaponInstance> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;
};


UCLASS(Blueprintable, BlueprintType, Const)
class UWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Class to spawn
	//UPROPERTY(EditDefaultsOnly, Category = Equipment)
	//TSubclassOf<AWeaponInstance> InstanceType;

	// Gameplay ability sets to grant when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<TObjectPtr<const UYogAbilitySet>> AbilitySetsToGrant;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<FWeaponActorToSpawn> ActorsToSpawn;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FRotator WeaponRotation;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Anime")
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	UFUNCTION()
	void SetupWeaponToCharacter(UWorld* World, USkeletalMeshComponent* AttachTarget, AYogCharacterBase* ReceivingChar);



	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup|WeaponDefiniton")
	//TObjectPtr<UYogEquipmentDefinition> WeaponDefinition;
};
