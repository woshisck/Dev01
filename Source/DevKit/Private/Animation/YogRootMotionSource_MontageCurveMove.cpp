#include "Animation/YogRootMotionSource_MontageCurveMove.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogYogMontageMove);

namespace YogMontageMove
{
	const FName MoveDistanceCurveName(TEXT("MoveDistance"));
}

FYogRootMotionSource_MontageCurveMove::FYogRootMotionSource_MontageCurveMove()
{
	// Matches the priority the engine's root motion ability tasks use, so this competes
	// evenly with them: only the highest-priority override source is applied.
	Priority = 5;

	// Only XY is owned by the curve; leaving Z alone keeps gravity, slopes and ledges working.
	Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);

	FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	FinishVelocityParams.ClampVelocity = 0.f;
}

FRootMotionSource* FYogRootMotionSource_MontageCurveMove::Clone() const
{
	return new FYogRootMotionSource_MontageCurveMove(*this);
}

bool FYogRootMotionSource_MontageCurveMove::Matches(const FRootMotionSource* Other) const
{
	if (!FRootMotionSource::Matches(Other))
	{
		return false;
	}

	// Safe cast: FRootMotionSource::Matches() already established ScriptStruct equality.
	const FYogRootMotionSource_MontageCurveMove* OtherCast = static_cast<const FYogRootMotionSource_MontageCurveMove*>(Other);

	return Montage == OtherCast->Montage;
}

void FYogRootMotionSource_MontageCurveMove::PrepareRootMotion(
	float SimulationTime,
	float MovementTickTime,
	const ACharacter& Character,
	const UCharacterMovementComponent& MoveComponent
	)
{
	RootMotionParams.Clear();

	FVector Velocity = FVector::ZeroVector;

	const USkeletalMeshComponent* Mesh = Character.GetMesh();
	const UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* MontageInstance = (AnimInstance && Montage)
		? AnimInstance->GetActiveInstanceForMontage(Montage)
		: nullptr;

	// No montage instance means it has not started yet or has already blended out. Emitting a
	// zero velocity is intentional: under Override it pins the character instead of letting
	// AI navigation drive it for those frames.
	if (MontageInstance && MovementTickTime > UE_SMALL_NUMBER)
	{
		const float Position = MontageInstance->GetPosition();
		const FAnimExtractContext ExtractContext(static_cast<double>(Position));
		const float CurveValue = Montage->EvaluateCurveData(YogMontageMove::MoveDistanceCurveName, ExtractContext);

		// A rewound position means the montage looped or was restarted while this source was
		// still alive. Re-baseline instead of turning the curve's drop back to zero into one
		// enormous backwards velocity spike.
		if (!bHasPreviousCurveValue || Position < PreviousPosition)
		{
			PreviousCurveValue = CurveValue;
			bHasPreviousCurveValue = true;
		}

		// Re-read every tick so rotation applied while the montage plays steers the travel.
		FVector Direction = Character.GetActorForwardVector();
		Direction.Z = 0.f;
		Direction = Direction.GetSafeNormal();

		// RootMotionParams translation is consumed as a velocity, so convert the distance
		// the curve advanced this tick back into cm/s.
		Velocity = Direction * ((CurveValue - PreviousCurveValue) / MovementTickTime);
		PreviousCurveValue = CurveValue;
		PreviousPosition = Position;

		UE_LOG(LogYogMontageMove, Verbose,
			TEXT("Prepare %s: Pos=%.3f Curve=%.2f Dir=%s Vel=%s MoveMode=%d"),
			*GetNameSafe(Montage),
			Position,
			CurveValue,
			*Direction.ToCompactString(),
			*Velocity.ToCompactString(),
			static_cast<int32>(MoveComponent.MovementMode.GetValue()));
	}
	else
	{
		UE_LOG(LogYogMontageMove, Verbose,
			TEXT("Prepare %s: no montage instance (AnimInstance=%s TickTime=%.4f)"),
			*GetNameSafe(Montage),
			*GetNameSafe(AnimInstance),
			MovementTickTime);
	}

	RootMotionParams.Set(FTransform(Velocity));
}

bool FYogRootMotionSource_MontageCurveMove::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!FRootMotionSource::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	Ar << Montage;

	bOutSuccess = true;
	return true;
}

UScriptStruct* FYogRootMotionSource_MontageCurveMove::GetScriptStruct() const
{
	return FYogRootMotionSource_MontageCurveMove::StaticStruct();
}

FString FYogRootMotionSource_MontageCurveMove::ToSimpleString() const
{
	return FString::Printf(TEXT("[ID:%u]FYogRootMotionSource_MontageCurveMove %s Montage(%s)"),
		LocalID,
		*InstanceName.GetPlainNameString(),
		*GetNameSafe(Montage));
}

void FYogRootMotionSource_MontageCurveMove::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Montage);

	FRootMotionSource::AddReferencedObjects(Collector);
}
