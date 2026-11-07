// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/PlayerLocomotionMath.h"

static float PlayerLocomotion_CardinalCenterDeg(ELocoCardinal Cardinal)
{
	switch (Cardinal)
	{
	case ELocoCardinal::Right:
		return 90.f;
	case ELocoCardinal::Back:
		return 180.f;
	case ELocoCardinal::Left:
		return -90.f;
	case ELocoCardinal::Front:
	default:
		return 0.f;
	}
}

float YogLocomotion::CalculateMoveAngle(const FVector& Direction2D, const FRotator& Facing)
{
	const FRotator YawOnly(0.f, Facing.Yaw, 0.f);
	const FVector Local = YawOnly.UnrotateVector(FVector(Direction2D.X, Direction2D.Y, 0.f));
	if (Local.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
}

ELocoCardinal YogLocomotion::AngleToCardinal(float AngleDeg, ELocoCardinal Previous, float DeadZoneDeg)
{
	const float Angle = FMath::UnwindDegrees(AngleDeg);
	const float DeadZone = FMath::Clamp(DeadZoneDeg, 0.f, 44.f);

	const float DeltaFromPrevious = FMath::Abs(FMath::UnwindDegrees(Angle - PlayerLocomotion_CardinalCenterDeg(Previous)));
	if (DeltaFromPrevious <= 45.f + DeadZone)
	{
		return Previous;
	}

	if (FMath::Abs(Angle) <= 45.f)
	{
		return ELocoCardinal::Front;
	}
	if (Angle > 45.f && Angle < 135.f)
	{
		return ELocoCardinal::Right;
	}
	if (Angle < -45.f && Angle > -135.f)
	{
		return ELocoCardinal::Left;
	}
	return ELocoCardinal::Back;
}

bool YogLocomotion::IsReversal(const FVector& Velocity2D, const FVector& InputDir, float ToleranceDeg)
{
	const FVector Vel = FVector(Velocity2D.X, Velocity2D.Y, 0.f).GetSafeNormal();
	const FVector Input = FVector(InputDir.X, InputDir.Y, 0.f).GetSafeNormal();
	if (Vel.IsNearlyZero() || Input.IsNearlyZero())
	{
		return false;
	}

	const float CosTolerance = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(ToleranceDeg, 0.f, 89.f)));
	return FVector::DotProduct(Vel, Input) <= -CosTolerance;
}

ELocoCardinal YogLocomotion::OppositeCardinal(ELocoCardinal Cardinal)
{
	switch (Cardinal)
	{
	case ELocoCardinal::Front:
		return ELocoCardinal::Back;
	case ELocoCardinal::Back:
		return ELocoCardinal::Front;
	case ELocoCardinal::Left:
		return ELocoCardinal::Right;
	case ELocoCardinal::Right:
	default:
		return ELocoCardinal::Left;
	}
}

ELocoMotionState YogLocomotion::NextMotionState(ELocoMotionState Previous, const FVector& Velocity, const FVector& Acceleration,
                                               float MaxSpeed, bool bAllowPivot, const FLocoMotionTuning& Tuning)
{
	const FVector Vel(Velocity.X, Velocity.Y, 0.f);
	const FVector Acc(Acceleration.X, Acceleration.Y, 0.f);
	const float Speed = Vel.Size();
	const bool bHasAccel = Acc.SizeSquared() > KINDA_SMALL_NUMBER;
	const bool bHasSpeed = Speed > Tuning.MinMoveSpeed;

	if (!bHasAccel)
	{
		return bHasSpeed ? ELocoMotionState::Stopping : ELocoMotionState::Idle;
	}

	if (Previous == ELocoMotionState::Pivoting && bAllowPivot)
	{
		// With ground friction CMC swings the velocity through zero within a frame or two rather than resting
		// at exactly zero, so "reached zero" must also accept the velocity having flipped onto the input side.
		if (bHasSpeed && FVector::DotProduct(Vel, Acc) < 0.f)
		{
			return ELocoMotionState::Pivoting;
		}
		return ELocoMotionState::Starting;
	}

	const float PivotMinSpeed = FMath::Max(Tuning.MinMoveSpeed, MaxSpeed * Tuning.PivotMinSpeedRatio);
	if (bAllowPivot && Speed >= PivotMinSpeed && IsReversal(Vel, Acc, Tuning.PivotAngleToleranceDeg))
	{
		return ELocoMotionState::Pivoting;
	}

	if (Previous == ELocoMotionState::Moving)
	{
		return ELocoMotionState::Moving;
	}

	const bool bReachedLoopSpeed = MaxSpeed <= KINDA_SMALL_NUMBER || Speed >= MaxSpeed * Tuning.StartToLoopSpeedRatio;
	return bReachedLoopSpeed ? ELocoMotionState::Moving : ELocoMotionState::Starting;
}

float YogLocomotion::ViewYawFromCameraRotation(const FRotator& CameraRotation)
{
	const FRotationMatrix CameraMatrix(CameraRotation);
	FVector ScreenUp = CameraMatrix.GetUnitAxis(EAxis::X) * FVector(1.f, 1.f, 0.f);
	if (ScreenUp.SizeSquared() <= KINDA_SMALL_NUMBER)
	{
		ScreenUp = CameraMatrix.GetUnitAxis(EAxis::Z) * FVector(1.f, 1.f, 0.f);
	}
	return ScreenUp.IsNearlyZero() ? CameraRotation.Yaw : ScreenUp.Rotation().Yaw;
}

ELocoPivotType YogLocomotion::ChoosePivotType(ELocoCardinal VelocityCardinal)
{
	switch (VelocityCardinal)
	{
	case ELocoCardinal::Front:
		return ELocoPivotType::Front_To_Back;
	case ELocoCardinal::Back:
		return ELocoPivotType::Back_To_Front;
	case ELocoCardinal::Left:
		return ELocoPivotType::Left_To_Right;
	case ELocoCardinal::Right:
	default:
		return ELocoPivotType::Right_To_Left;
	}
}
