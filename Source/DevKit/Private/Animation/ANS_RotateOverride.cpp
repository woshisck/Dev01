#include "Animation/ANS_RotateOverride.h"

#include "Curves/CurveFloat.h"

void UANS_RotateOverride::ConfigureSource(FYogRootMotionSource_CurveMotion& Source) const
{
	Source.TotalYaw = YawDegrees;
	Source.YawCurve = YawCurve;
}

ERootMotionAccumulateMode UANS_RotateOverride::GetAccumulateMode() const
{
	// Override would zero velocity and could outrank a move window; additive with zero translation is inert.
	return ERootMotionAccumulateMode::Additive;
}

bool UANS_RotateOverride::HasAnyEffect() const
{
	return !FMath::IsNearlyZero(YawDegrees);
}

FString UANS_RotateOverride::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Rotate %.0f deg%s"),
		YawDegrees,
		YawCurve ? *FString::Printf(TEXT(" | %s"), *YawCurve->GetName()) : TEXT(""));
}
