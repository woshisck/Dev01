
#pragma once

#include "CoreMinimal.h"
//#include "..\..\AbilitySystem\Abilities\YogAbilitySet.h"

#include "DevKit/AbilitySystem/Abilities/YogAbilitySet.h"
#include "DevKit/Data/AbilityData.h"
#include "DevKit/Animation/YogAnimInstance.h"

#include "WeaponDefinition.generated.h"

class UYogAbilitySet;
class AWeaponInstance;
class APlayerCharacterBase;
//class UYogAnimInstance;



USTRUCT(BlueprintType)
struct FWeaponSpawnData
{
	GENERATED_BODY()

	FWeaponSpawnData()
	{}

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AWeaponInstance> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	// Optional: Save game data for persistence
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldSaveToGame = false;
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
	//UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	//TArray<TObjectPtr<UYogAbilitySet>> AbilitySetsToGrant;
	UWeaponDefinition(){};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UAbilityData> AbilityData;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<FWeaponSpawnData> ActorsToSpawn;

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

	UFUNCTION(BlueprintCallable)
	void SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar);


private:
	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);

};
