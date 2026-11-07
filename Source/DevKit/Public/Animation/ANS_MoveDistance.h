#pragma once

#include "CoreMinimal.h"
#include "Animation/ANS_CurveMotionBase.h"
#include "ANS_MoveDistance.generated.h"

class UCurveFloat;

/**
 * Moves the character Distance cm along its facing over the notify window, for in-place animations.
 * Owns horizontal velocity for the window (Override), so walk input does not add to it; gravity is kept.
 *
 * Requires: root motion disabled on the montage's sequences (anim root motion takes precedence over
 * root motion sources) and a movement mode other than MOVE_None.
 */
UCLASS(meta = (DisplayName = "ANS Move Distance"))
class DEVKIT_API UANS_MoveDistance : public UANS_CurveMotionBase
{
	GENERATED_BODY()

public:
	/** Total travel in cm across the window. Negative moves backwards. */
	UPROPERTY(EditAnywhere, Category = "Move")
	float Distance = 200.f;

	/**
	 * X: normalized window time (0-1). Y: fraction of Distance covered so far (0 at X=0, 1 at X=1).
	 * Empty means linear. Must be a saved asset so it resolves on every machine.
	 */
	UPROPERTY(EditAnywhere, Category = "Move")
	TObjectPtr<UCurveFloat> DistanceCurve = nullptr;

	/** ActorForward follows any rotation applied during the window (e.g. ANS_RotateOverride). */
	UPROPERTY(EditAnywhere, Category = "Move")
	EYogCurveMoveDirection Direction = EYogCurveMoveDirection::ActorForward;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	virtual void ConfigureSource(FYogRootMotionSource_CurveMotion& Source) const override;
	virtual ERootMotionAccumulateMode GetAccumulateMode() const override;
	virtual bool HasAnyEffect() const override;
};
