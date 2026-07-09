#pragma once

#include "CoreMinimal.h"
#include "YogAnimNotifyState.h"
#include "UObject/ObjectKey.h"
#include "YogAnimNotifyState_Telegraph.generated.h"

class AYogTelegraphZoneActor;
class USkeletalMeshComponent;

/**
 * Pre-attack warning-zone telegraph window.
 *
 * Place this notify state over the windup/charge section of an enemy attack montage.
 * NotifyBegin spawns a AYogTelegraphZoneActor at the owner's feet (facing forward, following
 * the enemy if it lunges); NotifyEnd hides and destroys it. The actual hit is unchanged - it
 * still fires from AN_MeleeDamage / ANS Melee Damage Window later on the timeline.
 *
 * Instances are shared across every character playing the montage, so the spawned actor is
 * tracked per skeletal-mesh component.
 */
UCLASS(meta = (DisplayName = "ANS Pre-Attack Telegraph"))
class DEVKIT_API UYogAnimNotifyState_Telegraph : public UYogAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** Telegraph actor BP (with the warning-zone material) to spawn for the window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	TSubclassOf<AYogTelegraphZoneActor> TelegraphClass;

	/** Zone radius in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float Radius = 300.f;

	/** Sector half-angle in degrees (for sector-shaped warning materials). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float HalfAngle = 45.f;

	/** Local-space offset from the owner (e.g. push the zone forward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	FLinearColor Color = FLinearColor(1.f, 0.2f, 0.f, 1.f);

private:
	mutable TMap<TObjectKey<USkeletalMeshComponent>, TWeakObjectPtr<AYogTelegraphZoneActor>> SpawnedByMesh;
};
