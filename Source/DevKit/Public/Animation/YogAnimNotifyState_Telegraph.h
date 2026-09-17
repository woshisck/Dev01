#pragma once

#include "CoreMinimal.h"
#include "YogAnimNotifyState.h"
#include "UObject/ObjectKey.h"
#include "Actors/YogTelegraphZoneActor.h"
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
 * The zone footprint is always read off the AN_MeleeDamage that this window warns about - the
 * earliest one triggering at or after the window ends - so the warning and the hit cannot drift
 * apart. Place the window so it ends on the damage frame. If that notify is missing or its
 * hitbox carries no usable size, FYogTelegraphShape's defaults apply.
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
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	/** Fallback zone BP, used when ZoneClassByShape has no entry for the resolved shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	TSubclassOf<AYogTelegraphZoneActor> TelegraphClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	FLinearColor Color = FLinearColor(1.f, 0.2f, 0.f, 1.f);

	/**
	 * Zone BP per hitbox shape. Triangle falls back to the Annulus entry, since it is the same
	 * cone with no inner cull.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	TMap<EHitBoxType, TSubclassOf<AYogTelegraphZoneActor>> ZoneClassByShape;

private:
	FYogTelegraphShape ResolveShape(USkeletalMeshComponent* MeshComp,
		const UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) const;

	TSubclassOf<AYogTelegraphZoneActor> ResolveZoneClass(EHitBoxType ShapeType) const;

	mutable TMap<TObjectKey<USkeletalMeshComponent>, TWeakObjectPtr<AYogTelegraphZoneActor>> SpawnedByMesh;
};
