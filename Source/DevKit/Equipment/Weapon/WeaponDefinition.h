
#pragma once

#include "CoreMinimal.h"


#include "WeaponDefinition.generated.h"



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
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup|WeaponDefiniton")
	//TObjectPtr<UYogEquipmentDefinition> WeaponDefinition;
};
