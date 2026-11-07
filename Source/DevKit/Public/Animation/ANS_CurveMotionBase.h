#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/YogRootMotionSource_CurveMotion.h"
#include "ANS_CurveMotionBase.generated.h"

class ACharacter;

/**
 * Shared plumbing for notify states that drive an in-place montage through FYogRootMotionSource_CurveMotion.
 *
 * Network model: the source is applied on the server and on the owning client only. CharacterMovement
 * then predicts, saves, replays and corrects it like any engine root motion source; simulated proxies
 * just receive the replicated movement. Every input to the source comes from the notify and saved curve
 * assets, so both machines build identical sources.
 *
 * Notify state objects are shared by every character playing the montage, so no per-run state lives here.
 */
UCLASS(Abstract)
class DEVKIT_API UANS_CurveMotionBase : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
							 const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
						   const FAnimNotifyEventReference& EventReference) override;

protected:
	/** Fill the motion fields. Duration, priority, accumulate mode and InstanceName are set by the base. */
	virtual void ConfigureSource(FYogRootMotionSource_CurveMotion& Source) const PURE_VIRTUAL(UANS_CurveMotionBase::ConfigureSource, );

	virtual ERootMotionAccumulateMode GetAccumulateMode() const PURE_VIRTUAL(UANS_CurveMotionBase::GetAccumulateMode, return ERootMotionAccumulateMode::Additive;);

	virtual bool HasAnyEffect() const PURE_VIRTUAL(UANS_CurveMotionBase::HasAnyEffect, return false;);

private:
	/**
	 * Must be identical on server and client: CharacterMovement pairs the server's sources with the
	 * client's by Matches(), which compares InstanceName. Derived from the notify's path, not its
	 * runtime UniqueID.
	 */
	FName MakeInstanceName() const;

	static ACharacter* GetNetRelevantCharacter(const USkeletalMeshComponent* MeshComp);
};
