#include "Animation/ANS_MoveDistance.h"

#include "Curves/CurveFloat.h"

void UANS_MoveDistance::ConfigureSource(FYogRootMotionSource_CurveMotion& Source) const
{
	Source.TotalDistance = Distance;
	Source.DistanceCurve = DistanceCurve;
	Source.DirectionMode = Direction;

	// Stop dead when the window ends instead of carrying the last frame's lunge speed into walking.
	Source.FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	Source.FinishVelocityParams.ClampVelocity = 0.f;
}

ERootMotionAccumulateMode UANS_MoveDistance::GetAccumulateMode() const
{
	return ERootMotionAccumulateMode::Override;
}

bool UANS_MoveDistance::HasAnyEffect() const
{
	// Zero distance still pins the character in place for the window, which is a valid authoring choice.
	return true;
}

FString UANS_MoveDistance::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Move %.0fcm%s"),
		Distance,
		DistanceCurve ? *FString::Printf(TEXT(" | %s"), *DistanceCurve->GetName()) : TEXT(""));
}
