#include "Animation/YogRootMotionSource_CurveMotion.h"

#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

FYogRootMotionSource_CurveMotion::FYogRootMotionSource_CurveMotion()
{
	Priority = 5;

	// The curve owns XY only; leaving Z alone keeps gravity, slopes and ledges working.
	Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
}

float FYogRootMotionSource_CurveMotion::EvaluateFraction(const UCurveFloat* Curve, float Alpha)
{
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	return Curve ? Curve->GetFloatValue(Alpha) : Alpha;
}

float FYogRootMotionSource_CurveMotion::EvaluateFractionDelta(const UCurveFloat* Curve, float WindowTimeBegin, float WindowTimeEnd, float WindowDuration)
{
	if (WindowDuration <= KINDA_SMALL_NUMBER)
	{
		// A zero-length window applies everything on its only tick.
		return WindowTimeBegin <= 0.f ? EvaluateFraction(Curve, 1.f) - EvaluateFraction(Curve, 0.f) : 0.f;
	}

	return EvaluateFraction(Curve, WindowTimeEnd / WindowDuration) - EvaluateFraction(Curve, WindowTimeBegin / WindowDuration);
}

FRootMotionSource* FYogRootMotionSource_CurveMotion::Clone() const
{
	return new FYogRootMotionSource_CurveMotion(*this);
}

bool FYogRootMotionSource_CurveMotion::Matches(const FRootMotionSource* Other) const
{
	if (!FRootMotionSource::Matches(Other))
	{
		return false;
	}

	// Safe cast: FRootMotionSource::Matches() already established ScriptStruct equality.
	const FYogRootMotionSource_CurveMotion* OtherCast = static_cast<const FYogRootMotionSource_CurveMotion*>(Other);

	return DistanceCurve == OtherCast->DistanceCurve
		&& YawCurve == OtherCast->YawCurve
		&& DirectionMode == OtherCast->DirectionMode
		&& FMath::IsNearlyEqual(TotalDistance, OtherCast->TotalDistance, KINDA_SMALL_NUMBER)
		&& FMath::IsNearlyEqual(TotalYaw, OtherCast->TotalYaw, KINDA_SMALL_NUMBER);
}

void FYogRootMotionSource_CurveMotion::PrepareRootMotion(
	float SimulationTime,
	float MovementTickTime,
	const ACharacter& Character,
	const UCharacterMovementComponent& MoveComponent
	)
{
	RootMotionParams.Clear();
	PreparedDeltaYaw = 0.f;

	const float WindowTimeBegin = GetTime();
	const float WindowTimeEnd = WindowTimeBegin + SimulationTime;

	FVector Velocity = FVector::ZeroVector;
	if (!FMath::IsNearlyZero(TotalDistance) && MovementTickTime > UE_SMALL_NUMBER)
	{
		FVector Direction;
		if (DirectionMode == EYogCurveMoveDirection::LockedAtStart && bHasLockedDirection)
		{
			Direction = LockedDirection;
		}
		else
		{
			Direction = Character.GetActorForwardVector();
			Direction.Z = 0.f;
			Direction = Direction.GetSafeNormal();

			if (DirectionMode == EYogCurveMoveDirection::LockedAtStart)
			{
				LockedDirection = Direction;
				bHasLockedDirection = true;
			}
		}

		const float Distance = TotalDistance * EvaluateFractionDelta(DistanceCurve, WindowTimeBegin, WindowTimeEnd, Duration);

		// Translation is consumed as a velocity. Dividing by the real tick time (not SimulationTime) lets
		// simulated catch-up cover more ground in one tick, the same way the engine's own sources do.
		Velocity = Direction * (Distance / MovementTickTime);
	}

	if (DrivesRotation())
	{
		PreparedDeltaYaw = TotalYaw * EvaluateFractionDelta(YawCurve, WindowTimeBegin, WindowTimeEnd, Duration);
	}

	RootMotionParams.Set(FTransform(Velocity));

	SetTime(WindowTimeEnd);
}

bool FYogRootMotionSource_CurveMotion::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!FRootMotionSource::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	bOutSuccess = true;

	// Curves resolve across machines only because they are saved Content assets, same as the engine's
	// FRootMotionSource_ConstantForce::StrengthOverTime.
	Ar << TotalDistance;
	Ar << DistanceCurve;
	Ar << TotalYaw;
	Ar << YawCurve;

	uint8 DirectionModeSerialize = static_cast<uint8>(DirectionMode);
	Ar << DirectionModeSerialize;
	DirectionMode = static_cast<EYogCurveMoveDirection>(DirectionModeSerialize);

	Ar << bHasLockedDirection;
	if (bHasLockedDirection)
	{
		Ar << LockedDirection;
	}

	return true;
}

UScriptStruct* FYogRootMotionSource_CurveMotion::GetScriptStruct() const
{
	return FYogRootMotionSource_CurveMotion::StaticStruct();
}

FString FYogRootMotionSource_CurveMotion::ToSimpleString() const
{
	return FString::Printf(TEXT("[ID:%u]FYogRootMotionSource_CurveMotion %s Dist(%.1f, %s) Yaw(%.1f, %s) Time(%.3f/%.3f)"),
		LocalID,
		*InstanceName.GetPlainNameString(),
		TotalDistance,
		*GetNameSafe(DistanceCurve),
		TotalYaw,
		*GetNameSafe(YawCurve),
		GetTime(),
		Duration);
}

void FYogRootMotionSource_CurveMotion::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(DistanceCurve);
	Collector.AddReferencedObject(YawCurve);

	FRootMotionSource::AddReferencedObjects(Collector);
}
