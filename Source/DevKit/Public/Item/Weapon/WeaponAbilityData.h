#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponAbilityData.generated.h"

class UYogGameplayAbility;

UCLASS(BlueprintType)
class DEVKIT_API UWeaponAbilityData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Ability")
	TArray<TSubclassOf<UYogGameplayAbility>> WeaponAbilities;
};
