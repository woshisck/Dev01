#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_FireProjectile.generated.h"

/**
 * Spawns a lightweight bullet directly via UYogBulletManagerSubsystem on the fire frame.
 *
 * Reads URangedProjectileDefinition from the character's EquippedWeaponDef.
 * Does nothing if the equipped weapon is not Ranged or has no ProjectileDefinition set.
 *
 * Workflow:
 *   1. Place on the "fire" frame of a ranged attack montage.
 *   2. Set MuzzleSocketName to the weapon/character socket for spawn position.
 *   3. Configure the projectile via WeaponDefinition.ProjectileDefinition (URangedProjectileDefinition).
 *   4. GA_RangeAttack listens for HitEventTag (from URangedProjectileDefinition) to apply damage.
 */
UCLASS(meta = (DisplayName = "AN Fire Projectile"))
class DEVKIT_API UAN_FireProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAN_FireProjectile();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/**
	 * Skeletal mesh socket used to compute the projectile spawn transform.
	 * Leave blank to spawn at the character root transform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FName MuzzleSocketName;
};
