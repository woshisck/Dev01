#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SpawnRangedProjectiles.generated.h"

class AMusketBullet;
class UGameplayEffect;

/**
 * Spawns extra ranged projectiles for combat-card effects such as Split.
 * The projectiles inherit the current combat-card attack GUID so they do not consume extra cards.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Ranged Projectiles", Category = "BuffFlow|Projectile"))
class DEVKIT_API UBFNode_SpawnRangedProjectiles : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	TSubclassOf<AMusketBullet> BulletClass;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	TArray<float> YawOffsets;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	bool bUseCombatCardAttackDamage = true;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "!bUseCombatCardAttackDamage"))
	FFlowDataPinInputProperty_Float Damage;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	bool bShareAttackInstanceGuid = true;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
	bool bRequireRangedWeaponTag = true;

	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "bRequireRangedWeaponTag"))
	FGameplayTag RequiredWeaponTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	FVector ResolveMuzzleLocation(ACharacter* SourceCharacter) const;
};
