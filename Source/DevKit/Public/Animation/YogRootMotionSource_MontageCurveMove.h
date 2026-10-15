#pragma once

#include "CoreMinimal.h"
#include "GameFramework/RootMotionSource.h"
#include "YogRootMotionSource_MontageCurveMove.generated.h"

class UAnimMontage;

DECLARE_LOG_CATEGORY_EXTERN(LogYogMontageMove, Log, All);

namespace YogMontageMove
{
	/**
	 * Float curve authored on the montage asset's own Curves panel, where Y is the
	 * cumulative centimetres the character has travelled so far.
	 *
	 * Must live on the montage itself: UAnimMontage does not override EvaluateCurveData,
	 * so curves authored on the AnimSequences inside the montage slots are not visible here.
	 */
	extern DEVKIT_API const FName MoveDistanceCurveName;
}

/**
 * Drives a character along its own facing using a cumulative-distance curve sampled from the
 * montage currently playing on it. Exists because several skeletons in this project have no
 * root bone, so authored root motion is unavailable.
 *
 * Sampling is keyed off the montage's playback position rather than this source's own
 * elapsed time, so the motion stays locked to the animation when the play rate changes
 * (attack speed stat, HitStop slow/freeze) instead of drifting out of sync.
 *
 * The travel direction is re-read from the character every tick rather than captured when the
 * move starts, so turning the actor mid-montage (ANS_AttackRotate, AI facing updates) steers
 * the path instead of being ignored.
 */
USTRUCT()
struct DEVKIT_API FYogRootMotionSource_MontageCurveMove : public FRootMotionSource
{
	GENERATED_USTRUCT_BODY()

	FYogRootMotionSource_MontageCurveMove();

	virtual ~FYogRootMotionSource_MontageCurveMove() = default;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY()
	float PreviousCurveValue = 0.f;

	UPROPERTY()
	float PreviousPosition = 0.f;

	UPROPERTY()
	bool bHasPreviousCurveValue = false;

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
struct TStructOpsTypeTraits<FYogRootMotionSource_MontageCurveMove> : public TStructOpsTypeTraitsBase2<FYogRootMotionSource_MontageCurveMove>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
