#pragma once

#include "CoreMinimal.h"
#include "GameFramework/RootMotionSource.h"
#include "YogRootMotionSource_CurveMotion.generated.h"

class UCurveFloat;
class UCharacterMovementComponent;

UENUM(BlueprintType)
enum class EYogCurveMoveDirection : uint8
{
	/** Re-read the actor's facing every tick, so a rotation window running at the same time steers the path. */
	ActorForward,
	/** Capture the facing on the first simulated tick and keep it for the whole window. */
	LockedAtStart,
};

/**
 * Curve-driven displacement and yaw for in-place montages, driven by ANS_MoveDistance / ANS_RotateOverride.
 *
 * Progress comes only from this source's own clock (GetTime() / Duration), never from the montage
 * position. CharacterMovement re-runs PrepareRootMotion when it replays saved moves after a server
 * correction, and only the source's own time is restored for that replay, so anything else would make
 * the client and the server integrate different paths.
 *
 * Both curves map normalized window time (X, 0-1) to the fraction of the total that has been applied
 * (Y, usually 0-1). A null curve means linear.
 *
 * Yaw is not written into RootMotionParams: the engine only reads rotation from the single
 * highest-priority Override source, which would let a move window silently swallow a rotation window.
 * UYogCharacterMovementComponent::PhysicsRotation applies PreparedDeltaYaw instead.
 */
USTRUCT()
struct DEVKIT_API FYogRootMotionSource_CurveMotion : public FRootMotionSource
{
	GENERATED_USTRUCT_BODY()

	FYogRootMotionSource_CurveMotion();

	virtual ~FYogRootMotionSource_CurveMotion() = default;

	/** Centimetres covered over the whole window. Negative moves backwards. */
	UPROPERTY()
	float TotalDistance = 0.f;

	UPROPERTY()
	TObjectPtr<UCurveFloat> DistanceCurve = nullptr;

	UPROPERTY()
	EYogCurveMoveDirection DirectionMode = EYogCurveMoveDirection::ActorForward;

	/** Degrees turned over the whole window. Positive turns right (clockwise seen from above). */
	UPROPERTY()
	float TotalYaw = 0.f;

	UPROPERTY()
	TObjectPtr<UCurveFloat> YawCurve = nullptr;

	UPROPERTY()
	FVector LockedDirection = FVector::ZeroVector;

	UPROPERTY()
	bool bHasLockedDirection = false;

	/** Yaw to add this tick, written by PrepareRootMotion and consumed by the movement component. */
	float PreparedDeltaYaw = 0.f;

	/**
	 * Applied from a plain sequence (state machine clip) rather than a montage. Local bookkeeping only, not
	 * serialized: it lets gameplay cancel a cut-short locomotion clip's motion without touching ability montages.
	 */
	bool bFromSequence = false;

	bool DrivesRotation() const { return !FMath::IsNearlyZero(TotalYaw); }

	/** Fraction of the total applied at normalized time Alpha (clamped to 0-1). */
	static float EvaluateFraction(const UCurveFloat* Curve, float Alpha);

	/** Fraction gained while the window clock goes from WindowTimeBegin to WindowTimeEnd. */
	static float EvaluateFractionDelta(const UCurveFloat* Curve, float WindowTimeBegin, float WindowTimeEnd, float WindowDuration);

	virtual FRootMotionSource* Clone() const override;

	virtual bool Matches(const FRootMotionSource* Other) const override;

	virtual void PrepareRootMotion(
		float SimulationTime,
		float MovementTickTime,
		const ACharacter& Character,
		const UCharacterMovementComponent& MoveComponent
		) override;

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	virtual UScriptStruct* GetScriptStruct() const override;

	virtual FString ToSimpleString() const override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
};

template<>
struct TStructOpsTypeTraits<FYogRootMotionSource_CurveMotion> : public TStructOpsTypeTraitsBase2<FYogRootMotionSource_CurveMotion>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
