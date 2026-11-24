#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "YogPhysicalMaterial.generated.h"

/**
 * Physical material that carries a hit-impact surface tag. Assign it to environment materials or
 * static mesh overrides; melee swings that touch the surface play the matching entry of
 * UEnemyHitImpactData (UYogSettings::EnemyHitImpactData).
 */
UCLASS(BlueprintType)
class DEVKIT_API UYogPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitImpact", meta = (Categories = "HitReact.Material"))
	FGameplayTag HitReactMaterial;
};
