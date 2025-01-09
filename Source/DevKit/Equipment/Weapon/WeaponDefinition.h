
#pragma once

#include "CoreMinimal.h"
#include "..\..\AbilitySystem\Abilities\YogAbilitySet.h"

#include "WeaponDefinition.generated.h"

class UYogAbilitySet;

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UWeaponDefinition : public UDataAsset
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


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TSubclassOf<UYogAbilitySet> GrantAbility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<AActor> WeaponActor;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup|WeaponDefiniton")
	//TObjectPtr<UYogEquipmentDefinition> WeaponDefinition;
};
