// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerLocomotionMath.generated.h"

/**
 * Which frame every directional locomotion value is measured in.
 * Free: the camera view (Front = screen up). Locked: the body, which keeps facing the aim target.
 */
UENUM(BlueprintType)
enum class ELocoMode : uint8
{
	Free,
	Locked
};

/**
 * Locomotion phase derived purely from CharacterMovement velocity and acceleration.
 * Nothing gates input; the capsule moves exactly as CMC integrates it and this only labels what it is doing.
 */
UENUM(BlueprintType)
enum class ELocoMotionState : uint8
{
	// No input, capsule at rest.
	Idle,
	// Input held, speed still building toward the loop speed.
	Starting,
	// Input held at (near) full speed.
	Moving,
	// Input held against the velocity: CMC is braking the old direction. Ends when the velocity reaches zero
	// or swings onto the input side.
	Pivoting,
	// Input released, capsule still sliding.
	Stopping
};

/** Thresholds for NextMotionState. Plain data so the pure function stays testable. */
struct FLocoMotionTuning
{
	// Below this horizontal speed (cm/s) the capsule counts as stationary.
	float MinMoveSpeed = 3.f;
	// Acceleration within this many degrees of directly opposite the velocity starts a pivot.
	float PivotAngleToleranceDeg = 45.f;
	// A pivot only starts above this fraction of max speed, so tiny shuffles do not play a pivot clip.
	float PivotMinSpeedRatio = 0.5f;
	// Starting becomes Moving at this fraction of max speed.
	float StartToLoopSpeedRatio = 0.9f;
};

/** Movement direction relative to the current ELocoMode frame. */
UENUM(BlueprintType)
enum class ELocoCardinal : uint8
{
	Front,
	Back,
	Left,
	Right
};

/** Pivot clip family, chosen once when ELocoMotionState enters Pivoting. */
UENUM(BlueprintType)
enum class ELocoPivotType : uint8
{
	None,
	// No longer produced. Kept so existing ABP enum pins keep resolving.
	TurnAround_L,
	TurnAround_R,
	// Named after the velocity cardinal before the reversal, in the pivot's ELocoMode frame.
	Front_To_Back,
	Back_To_Front,
	Left_To_Right,
	Right_To_Left
};

/**
 * Pure locomotion math shared by UYogAnimInstance and UPlayerLocomotionComponent.
 * No world or actor state, so it can be unit-tested directly.
 */
namespace YogLocomotion
{
	/** Signed angle (degrees, -180..180, positive = right of facing) of a horizontal direction relative to a facing. */
	DEVKIT_API float CalculateMoveAngle(const FVector& Direction2D, const FRotator& Facing);

	/**
	 * Maps an angle to a cardinal. The Previous cardinal is kept while the angle stays inside its band
	 * widened by DeadZoneDeg, so the clip does not flicker on a band edge.
	 */
	DEVKIT_API ELocoCardinal AngleToCardinal(float AngleDeg, ELocoCardinal Previous, float DeadZoneDeg);

	/** True when InputDir points within ToleranceDeg of directly opposite Velocity2D. Both must be non-zero. */
	DEVKIT_API bool IsReversal(const FVector& Velocity2D, const FVector& InputDir, float ToleranceDeg);

	/** The cardinal that is directly opposite the given one. */
	DEVKIT_API ELocoCardinal OppositeCardinal(ELocoCardinal Cardinal);

	/**
	 * One step of the locomotion phase machine. Velocity and Acceleration are CMC's (vertical ignored).
	 * bAllowPivot is false in the air or while an ability / stun / UI owns movement.
	 */
	DEVKIT_API ELocoMotionState NextMotionState(ELocoMotionState Previous, const FVector& Velocity, const FVector& Acceleration,
	                                            float MaxSpeed, bool bAllowPivot, const FLocoMotionTuning& Tuning);

	/** Picks the swap from the cardinal the pre-reversal velocity has in the pivot's frame. */
	DEVKIT_API ELocoPivotType ChoosePivotType(ELocoCardinal VelocityCardinal);

	/**
	 * Yaw of "screen up" in world space for a camera rotation. Falls back to the camera's up axis when the
	 * camera looks straight down, where the forward axis has no horizontal component.
	 */
	DEVKIT_API float ViewYawFromCameraRotation(const FRotator& CameraRotation);
}
