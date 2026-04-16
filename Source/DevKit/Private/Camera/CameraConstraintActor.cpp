// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/CameraConstraintActor.h"
#include "DrawDebugHelpers.h"

ACameraConstraintActor::ACameraConstraintActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// 允许编辑器中也 Tick，以便持续绘制边界线
	PrimaryActorTick.bStartWithTickEnabled = true;

	// 默认在游戏中不可见（纯逻辑 Actor）
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ACameraConstraintActor::BeginPlay()
{
	Super::BeginPlay();
}

void ACameraConstraintActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	// 绘制边界线（编辑器和开发版本均可见）
	// BoundaryPoints 存储本地坐标，先转换为世界坐标再绘制
	const int32 N = BoundaryPoints.Num();
	if (N < 2) return;

	const FTransform ActorTM = GetActorTransform();
	for (int32 i = 0; i < N; i++)
	{
		const FVector A = ActorTM.TransformPosition(BoundaryPoints[i]);
		const FVector B = ActorTM.TransformPosition(BoundaryPoints[(i + 1) % N]);
		DrawDebugLine(GetWorld(), A, B, FColor::Cyan, false, -1.0f, 0, 4.0f);
		// 顶点标记
		DrawDebugSphere(GetWorld(), A, 20.0f, 6, FColor::Yellow, false, -1.0f, 0, 2.0f);
	}
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// 算法实现
// ─────────────────────────────────────────────────────────────────────────────

bool ACameraConstraintActor::IsPointInsidePolygon(FVector2D Point) const
{
	const int32 N = BoundaryPoints.Num();
	if (N < 3) return false;

	// BoundaryPoints 为本地坐标，转换为世界坐标后再做射线法（Ray Casting）
	const FTransform ActorTM = GetActorTransform();
	bool bInside = false;
	int32 j = N - 1;
	for (int32 i = 0; i < N; i++)
	{
		const FVector Wi = ActorTM.TransformPosition(BoundaryPoints[i]);
		const FVector Wj = ActorTM.TransformPosition(BoundaryPoints[j]);
		const FVector2D Pi(Wi.X, Wi.Y);
		const FVector2D Pj(Wj.X, Wj.Y);

		if (((Pi.Y > Point.Y) != (Pj.Y > Point.Y)) &&
			(Point.X < (Pj.X - Pi.X) * (Point.Y - Pi.Y) / (Pj.Y - Pi.Y) + Pi.X))
		{
			bInside = !bInside;
		}
		j = i;
	}
	return bInside;
}

FVector2D ACameraConstraintActor::GetClosestBoundaryPoint(FVector2D Point) const
{
	const int32 N = BoundaryPoints.Num();
	if (N == 0) return Point;

	// BoundaryPoints 为本地坐标，先全部转换为世界坐标
	const FTransform ActorTM = GetActorTransform();
	const FVector W0 = ActorTM.TransformPosition(BoundaryPoints[0]);
	FVector2D Closest(W0.X, W0.Y);
	float MinDistSq = FLT_MAX;

	for (int32 i = 0; i < N; i++)
	{
		const FVector WA = ActorTM.TransformPosition(BoundaryPoints[i]);
		const FVector WB = ActorTM.TransformPosition(BoundaryPoints[(i + 1) % N]);
		const FVector2D A(WA.X, WA.Y);
		const FVector2D B(WB.X, WB.Y);

		const FVector2D EdgeClosest = FMath::ClosestPointOnSegment2D(Point, A, B);
		const float DistSq = FVector2D::DistSquared(Point, EdgeClosest);

		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			Closest = EdgeClosest;
		}
	}
	return Closest;
}

FVector2D ACameraConstraintActor::ConstrainPosition(FVector2D InPos) const
{
	if (BoundaryPoints.Num() < 3)
	{
		// 顶点不足，无法约束
		return InPos;
	}

	if (IsPointInsidePolygon(InPos))
	{
		return InPos;
	}

	return GetClosestBoundaryPoint(InPos);
}
