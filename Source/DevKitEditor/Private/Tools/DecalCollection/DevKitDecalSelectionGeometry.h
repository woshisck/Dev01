#pragma once

#include "CoreMinimal.h"

namespace DevKit::DecalSelection
{

/** Exact ray / oriented projector-box test. DecalSize contains half extents,
 * matching UDecalComponent::CalcBounds. Distance is measured in world units. */
inline bool IntersectProjectionBox(const FTransform& Transform, const FVector& DecalSize,
	const FVector& RayOrigin, const FVector& RayDirection, double& OutDistance)
{
	OutDistance = 0.0;
	const FVector Scale = Transform.GetScale3D().GetAbs();
	const FVector Extent = DecalSize.GetAbs();
	if (Scale.GetMin() <= UE_SMALL_NUMBER || Extent.GetMin() <= UE_SMALL_NUMBER
		|| RayDirection.IsNearlyZero() || Transform.ContainsNaN() || DecalSize.ContainsNaN() || RayOrigin.ContainsNaN() || RayDirection.ContainsNaN())
	{
		return false;
	}
	const FVector LocalOrigin = Transform.InverseTransformPosition(RayOrigin);
	// Do not normalize after transforming: this preserves world-distance t under non-uniform scale.
	const FVector LocalDirection = Transform.InverseTransformVector(RayDirection.GetSafeNormal());
	double Near = 0.0;
	double Far = TNumericLimits<double>::Max();
	for (int32 Axis = 0; Axis != 3; ++Axis)
	{
		if (FMath::Abs(LocalDirection[Axis]) <= UE_DOUBLE_SMALL_NUMBER)
		{
			if (FMath::Abs(LocalOrigin[Axis]) > Extent[Axis]) return false;
			continue;
		}
		double First = (-Extent[Axis] - LocalOrigin[Axis]) / LocalDirection[Axis];
		double Second = (Extent[Axis] - LocalOrigin[Axis]) / LocalDirection[Axis];
		if (First > Second) Swap(First, Second);
		Near = FMath::Max(Near, First);
		Far = FMath::Min(Far, Second);
		if (Far < Near) return false;
	}
	OutDistance = Near;
	return true;
}

}
