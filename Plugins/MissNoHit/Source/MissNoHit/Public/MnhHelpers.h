// Copyright 2024 Eren Balatkan. All Rights Reserved.

#pragma once

#include "CollisionShape.h"
#include "CollisionQueryParams.h"
#include "CoreMinimal.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/Object.h"
#include "MnhHelpers.generated.h"

UENUM()
enum class EMnhTraceSource : uint8
{
	MnhShapeComponent 		UMETA(DisplayName = "Mnh Shape Component"),
	PhysicsAsset 			UMETA(DisplayName = "Physics Asset"),
	AnimNotify 				UMETA(DisplayName = "AnimNotify"),
	StaticMeshSockets 		UMETA(DisplayName = "Static Mesh Sockets"),
};

UENUM()
enum class EMnhTraceShape : uint8
{
	Sphere					UMETA(DisplayName = "Sphere"),
	Box						UMETA(DisplayName = "Box"),
	Capsule					UMETA(DisplayName = "Capsule")
};

UENUM(BlueprintType)
enum class EMnhTraceType : uint8
{
	ByChannel				UMETA(DisplayName = "By Channel"),
	ByObject				UMETA(DisplayName = "By Object"),
	ByProfile				UMETA(DisplayName = "By Profile")
};

UENUM(BlueprintType)
enum class EMnhTracerTickType : uint8
{
	MatchGameTick			UMETA(DisplayName = "Match Game Tick"),
	FixedRateTick			UMETA(DisplayName = "Fixed Rate Tick"),
	DistanceTick			UMETA(DisplayName = "Tick by Distance Traveled")
};

UENUM()
enum class EMnhTracerState : uint8
{
	Active					UMETA(DisplayName = "Active"),
	Stopped					UMETA(DisplayName = "Stopped"),
	PendingStop				UMETA(DisplayName = "PendingStop")
};

USTRUCT(BlueprintType)
struct FMnhShapeData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit")
	EMnhTraceShape TraceShape = EMnhTraceShape::Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), Category="MissNoHit")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceShape!=EMnhTraceShape::Box", EditConditionHides, ShowOnlyInnerProperties))
	float Radius = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceShape==EMnhTraceShape::Capsule", EditConditionHides, ShowOnlyInnerProperties))
	float HalfHeight = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceShape==EMnhTraceShape::Box", EditConditionHides, ShowOnlyInnerProperties))
	FVector HalfSize = FVector(10.f, 10.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceShape!=EMnhTraceShape::Sphere", EditConditionHides, ShowOnlyInnerProperties))
	FRotator Orientation = FRotator::ZeroRotator;

	FORCEINLINE FCollisionShape GetTracerShape() const;
};

USTRUCT(BlueprintType)
struct FMnhTraceSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	EMnhTraceType TraceType = EMnhTraceType::ByChannel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceType==EMnhTraceType::ByChannel", EditConditionHides))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceType==EMnhTraceType::ByObject", EditConditionHides))
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit",
		meta=(EditCondition="TraceType==EMnhTraceType::ByProfile", EditConditionHides))
	FName ProfileName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	bool bTraceComplex=true;
};

USTRUCT()
struct MISSNOHIT_API FMnhHelpers
{
	GENERATED_BODY()

public:
	FORCEINLINE static void Mnh_Log(const FString& Text);
	FORCEINLINE static void DrawDebug(const FVector& Start, const FVector& End, const FQuat& Rot, TArray<FHitResult> Hits, const FMnhShapeData& ShapeData, const UWorld* World,
		const EDrawDebugTrace::Type DrawDebugType, float DebugDrawTime, const FColor& DebugTraceColor, const FColor& DebugTraceBlockColor, const FColor& DebugTraceHitColor);
	FORCEINLINE static void DrawTrace(const FVector& Start, const FVector& End, const FQuat& Rot, const FColor Color, const FMnhShapeData& ShapeData, const UWorld* World,
		const EDrawDebugTrace::Type DrawDebugType, float DebugDrawTime);
	FORCEINLINE static void PerformTrace(const FTransform& StartTransform, const FTransform& EndTransform, const FTransform& AverageTransform, TArray<FHitResult>& OutHits, const UWorld* World,
		const FMnhTraceSettings& TraceSettings,
		const FMnhShapeData& ShapeData, const FCollisionQueryParams& CollisionParams,
		const FCollisionResponseParams& CollisionResponseParams,
		const FCollisionObjectQueryParams& ObjectQueryParams);
};
