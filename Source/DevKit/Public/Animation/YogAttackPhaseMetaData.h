#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMetaData.h"
#include "YogAttackPhaseMetaData.generated.h"

/**
 * Phase layout of an attack montage, in montage-local seconds.
 *
 * Attach one to an attack montage's MetaData. Read it with UAnimMontage::GetMetaData() before the
 * montage starts, so callers can size a windup (e.g. the pre-attack telegraph) against the real
 * duration instead of discovering it when a notify window happens to open. A notify state or an
 * anim curve can only report the phase the montage is in right now, never how long it lasts.
 *
 * Hand-authored and NOT validated against the animation: retiming the montage silently invalidates
 * these numbers.
 */
UCLASS(EditInlineNew, DisplayName = "Yog Attack Phase Layout")
class DEVKIT_API UYogAttackPhaseMetaData : public UAnimMetaData
{
	GENERATED_BODY()

public:
	// UAnimMetaData is abstract and declares only an FObjectInitializer constructor, so the
	// derived default constructor has to forward explicitly.
	UYogAttackPhaseMetaData(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
	}

	/** Windup ends / active attack begins. */
	UPROPERTY(EditAnywhere, Category = "Attack Phase", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float PreAttackEnd = 0.f;

	/** Active attack ends / recovery begins. */
	UPROPERTY(EditAnywhere, Category = "Attack Phase", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float AttackEnd = 0.f;
};
