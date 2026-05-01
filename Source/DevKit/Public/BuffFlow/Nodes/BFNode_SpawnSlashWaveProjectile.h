#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_SpawnSlashWaveProjectile.generated.h"

class ASlashWaveProjectile;
class UGameplayEffect;

/**
 * Spawns a configurable slash-wave projectile. Used by combat cards such as Moonlight.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Slash Wave Projectile", Category = "BuffFlow|Projectile"))
class DEVKIT_API UBFNode_SpawnSlashWaveProjectile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	TSubclassOf<ASlashWaveProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1.0"))
	float Speed = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0"))
	float MaxDistance = 800.f;

	/** <= 0 means unlimited targets. Each actor is still hit only once. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	int32 MaxHitCount = 2;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	FVector SpawnOffset = FVector(80.f, 0.f, 45.f);

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
