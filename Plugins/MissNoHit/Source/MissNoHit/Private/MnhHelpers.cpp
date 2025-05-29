// Copyright 2024 Eren Balatkan. All Rights Reserved.


#include "MnhHelpers.h"

#include "MnhTracer.h"
#include "MnhTracerComponent.h"
#include "Kismet/KismetMathLibrary.h"

FCollisionShape FMnhShapeData::GetTracerShape() const
{
	SCOPE_CYCLE_COUNTER(STAT_MnhGetTracerShape)
	switch (TraceShape)
	{
	case EMnhTraceShape::Sphere:
		return FCollisionShape::MakeSphere(Radius);
		break;
	case EMnhTraceShape::Box:
		return FCollisionShape::MakeBox(HalfSize);
		break;
	case EMnhTraceShape::Capsule:
		return FCollisionShape::MakeCapsule(Radius, HalfHeight);
		break;
	default:
		return FCollisionShape::MakeSphere(Radius);
		break;
	}
}

void FMnhHelpers::Mnh_Log(const FString& Text)
{
#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, 5.0, FColor::Red, Text);
	UE_LOG(LogMnh, Warning, TEXT("%s"), *Text)
#endif
}


void MnhDrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
	FVector const TraceVec = End - Start;
	float const Dist = TraceVec.Size();

	FVector const Center = Start + TraceVec * 0.5f;
	float const HalfHeight = (Dist * 0.5f) + Radius;

	FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
	::DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
}

void MnhDrawDebugSweptBox(const UWorld* InWorld, FVector const& Start, FVector const& End, FRotator const & Orientation, FVector const & HalfSize, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
	FVector const TraceVec = End - Start;

	FQuat const CapsuleRot = Orientation.Quaternion();
	::DrawDebugBox(InWorld, Start, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);

	//now draw lines from vertices
	FVector Vertices[8];
	Vertices[0] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));	//flt
	Vertices[1] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));	//frt
	Vertices[2] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));	//flb
	Vertices[3] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));	//frb
	Vertices[4] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));	//blt
	Vertices[5] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));	//brt
	Vertices[6] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));	//blb
	Vertices[7] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));		//brb
	for (int32 VertexIdx = 0; VertexIdx < 8; ++VertexIdx)
	{
		::DrawDebugLine(InWorld, Vertices[VertexIdx], Vertices[VertexIdx] + TraceVec, Color, bPersistentLines, LifeTime, DepthPriority);
	}

	DrawDebugBox(InWorld, End, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
}


void FMnhHelpers::DrawDebug(const FVector& Start, const FVector& End, const FQuat& Rot, TArray<FHitResult> Hits, const FMnhShapeData& ShapeData,
	const UWorld* World, const EDrawDebugTrace::Type DrawDebugType, float DebugDrawTime,
	const FColor& DebugTraceColor, const FColor& DebugTraceBlockColor, const FColor& DebugTraceHitColor)
{
	SCOPE_CYCLE_COUNTER(STAT_MnhTracerDebugDraw)
	
	if (Hits.Num() > 0) {
		const FHitResult& FirstHit = Hits[0];
		DrawTrace(Start, FirstHit.Location, Rot, DebugTraceColor, ShapeData, World, DrawDebugType, DebugDrawTime);

		if (Hits.Last().bBlockingHit)
		{
			if (Hits.Num() == 1)
			{
				DrawTrace(FirstHit.Location, End, Rot, DebugTraceBlockColor, ShapeData, World, DrawDebugType, DebugDrawTime);
			}
			else
			{
				DrawTrace(FirstHit.Location, Hits.Last().Location, Rot, DebugTraceHitColor, ShapeData, World, DrawDebugType, DebugDrawTime);
				DrawTrace(Hits.Last().Location, End, Rot, DebugTraceBlockColor, ShapeData, World, DrawDebugType, DebugDrawTime);
			}
		}
		else {DrawTrace(FirstHit.Location, End, Rot, DebugTraceHitColor, ShapeData, World, DrawDebugType, DebugDrawTime);}
	} else {DrawTrace(Start, End, Rot, DebugTraceColor, ShapeData, World, DrawDebugType, DebugDrawTime);}
	
	for (const FHitResult& Hit : Hits){
		DrawDebugPoint(World, Hit.ImpactPoint, 10, Hit.bBlockingHit ? DebugTraceColor : DebugTraceHitColor,
			DrawDebugType == EDrawDebugTrace::Persistent, DebugDrawTime);
		}
}

void FMnhHelpers::DrawTrace(const FVector& Start, const FVector& End, const FQuat& Rot, const FColor Color, const FMnhShapeData& ShapeData, const UWorld* World,
		const EDrawDebugTrace::Type DrawDebugType, float DebugDrawTime)
{
	switch (ShapeData.TraceShape)
	{
	case EMnhTraceShape::Sphere:
		{
			MnhDrawDebugSweptSphere(World, Start, End, ShapeData.Radius, Color, DrawDebugType == EDrawDebugTrace::Persistent, DebugDrawTime, 0);
			break;
		}
	case EMnhTraceShape::Box:
		{
			MnhDrawDebugSweptBox(World, Start, End, Rot.Rotator(), ShapeData.HalfSize, Color, DrawDebugType == EDrawDebugTrace::Persistent, DebugDrawTime, 0);
			break;
		}
	case EMnhTraceShape::Capsule:
		{
			DrawDebugCapsule(World, Start, ShapeData.HalfHeight, ShapeData.Radius, Rot, Color, DrawDebugType == EDrawDebugTrace::Persistent, DebugDrawTime);
			DrawDebugCapsule(World, End, ShapeData.HalfHeight, ShapeData.Radius, Rot, Color, DrawDebugType == EDrawDebugTrace::Persistent, DebugDrawTime);
			DrawDebugLine(World, Start, End, Color, DrawDebugType == EDrawDebugTrace::Persistent, DebugDrawTime);
			break;
		}
	default:
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid TraceShape value in UTracer::DrawTrace."));
			break;
		}
	}
}

void FMnhHelpers::PerformTrace(const FTransform& StartTransform, const FTransform& EndTransform, const FTransform& AverageTransform,
                               TArray<FHitResult>& OutHits, const UWorld* World,
                               const FMnhTraceSettings& TraceSettings,
                               const FMnhShapeData& ShapeData, const FCollisionQueryParams& CollisionParams,
                               const FCollisionResponseParams& CollisionResponseParams,
                               const FCollisionObjectQueryParams& ObjectQueryParams)
{
	switch (TraceSettings.TraceType)
	{
	case EMnhTraceType::ByChannel:
		World->SweepMultiByChannel(
			OutHits,
			StartTransform.GetLocation(),
			EndTransform.GetLocation(),
			AverageTransform.GetRotation(),
			TraceSettings.TraceChannel,
			ShapeData.GetTracerShape(),
			CollisionParams,
			CollisionResponseParams);
		break;
	case EMnhTraceType::ByObject:
		World->SweepMultiByObjectType(
			OutHits,
			StartTransform.GetLocation(),
			EndTransform.GetLocation(),
			AverageTransform.GetRotation(),
			ObjectQueryParams,
			ShapeData.GetTracerShape(),
			CollisionParams);
		break;
	case EMnhTraceType::ByProfile:
		World->SweepMultiByProfile(
			OutHits,
			StartTransform.GetLocation(),
			EndTransform.GetLocation(),
			AverageTransform.GetRotation(),
			TraceSettings.ProfileName,
			ShapeData.GetTracerShape(),
			CollisionParams);
		break;
	}
}
