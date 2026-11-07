#pragma once

#include "CoreMinimal.h"
#include "Animation/ANS_CurveMotionBase.h"
#include "ANS_RotateOverride.generated.h"

class UCurveFloat;

/**
 * Turns the character by YawDegrees over the notify window, for in-place animations.
 * Adds no velocity (Additive), so it composes with ANS_MoveDistance or with normal walking.
 * While active it replaces orient-to-movement / controller-desired rotation.
 *
 * Applied in UYogCharacterMovementComponent::PhysicsRotation, so the character must use that class.
 */
UCLASS(meta = (DisplayName = "ANS Rotate Override"))
class DEVKIT_API UANS_RotateOverride : public UANS_CurveMotionBase
{
	GENERATED_BODY()

public:
	/** Total turn across the window. Positive turns right (clockwise seen from above). */
	UPROPERTY(EditAnywhere, Category = "Rotate", meta = (ClampMin = "-1440.0", ClampMax = "1440.0"))
	float YawDegrees = 180.f;

	/**
	 * X: normalized window time (0-1). Y: fraction of YawDegrees turned so far (0 at X=0, 1 at X=1).
	 * Empty means linear. Must be a saved asset so it resolves on every machine.
	 */
	UPROPERTY(EditAnywhere, Category = "Rotate")
	TObjectPtr<UCurveFloat> YawCurve = nullptr;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	virtual void ConfigureSource(FYogRootMotionSource_CurveMotion& Source) const override;
	virtual ERootMotionAccumulateMode GetAccumulateMode() const override;
	virtual bool HasAnyEffect() const override;
};
