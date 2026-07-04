#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Projectile/BuffFlowProjectile.h"
#include "RangedProjectileDefinition.generated.h"

/**
 * Weapon-owned projectile data used by GA_RangeAttack.
 *
 * Ranged weapons reference this asset from WeaponDefinition so a shared range
 * attack ability can spawn different projectile behavior per weapon.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "Ranged Projectile Definition")
class DEVKIT_API URangedProjectileDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	URangedProjectileDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<ABuffFlowProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FBuffFlowProjectileRuntimeConfig ProjectileConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bUseBulletManager = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FGameplayTag HitEffectContainerTag;
};
