#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SacrificeShadowEchoActor.generated.h"

class APlayerCharacterBase;
class USkeletalMeshComponent;

/**
 * Stationary visual echo spawned by sacrifice runes.
 * Runtime damage and card-effect replay are driven by USacrificeRuneComponent.
 */
UCLASS()
class DEVKIT_API ASacrificeShadowEchoActor : public AActor
{
	GENERATED_BODY()

public:
	ASacrificeShadowEchoActor();

	void InitializeFromPlayer(APlayerCharacterBase* Player, int32 InAttackCharges, float InLifetime);
	bool ConsumeAttackCharge();
	int32 GetRemainingAttackCharges() const { return RemainingAttackCharges; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sacrifice|Shadow")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sacrifice|Shadow")
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

private:
	void Expire();

	int32 RemainingAttackCharges = 0;
	FTimerHandle LifetimeTimer;
};
