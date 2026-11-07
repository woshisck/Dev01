#include "Animation/ANS_CurveMotionBase.h"

#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

static const FAnimMontageInstance* CurveMotion_FindMontageInstance(const USkeletalMeshComponent* MeshComp,
																	const FAnimNotifyEventReference& EventReference)
{
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return nullptr;
	}

	const UE::Anim::FAnimNotifyMontageInstanceContext* Context =
		EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	return Context ? AnimInstance->GetMontageInstanceForID(Context->MontageInstanceID) : nullptr;
}

ACharacter* UANS_CurveMotionBase::GetNetRelevantCharacter(const USkeletalMeshComponent* MeshComp)
{
	ACharacter* Character = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
	if (!IsValid(Character) || !Character->GetCharacterMovement())
	{
		return nullptr;
	}

	// Simulated proxies take the server's replicated movement; a local source would fight it.
	if (!Character->HasAuthority() && !Character->IsLocallyControlled())
	{
		return nullptr;
	}

	return Character;
}

FName UANS_CurveMotionBase::MakeInstanceName() const
{
	const uint32 PathHash = GetTypeHash(GetPathName());

	// FName number 0 means "no number"; keep it non-zero and positive.
	return FName(TEXT("YogCurveMotion"), static_cast<int32>((PathHash & 0x7FFFFFFE) + 1));
}

void UANS_CurveMotionBase::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
									   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!HasAnyEffect())
	{
		return;
	}

	ACharacter* Character = GetNetRelevantCharacter(MeshComp);
	if (!Character)
	{
		return;
	}

	// The source clock runs in world time while TotalDuration is in animation time, so fold in the
	// playback rate (attack speed) that is active when the window opens.
	float PlayRate = Animation ? Animation->RateScale : 1.f;
	if (const FAnimMontageInstance* MontageInstance = CurveMotion_FindMontageInstance(MeshComp, EventReference))
	{
		PlayRate *= MontageInstance->GetPlayRate();
	}
	PlayRate = FMath::Abs(PlayRate);

	const float WindowDuration = PlayRate > KINDA_SMALL_NUMBER ? TotalDuration / PlayRate : TotalDuration;

	const FName InstanceName = MakeInstanceName();
	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();

	// A montage restarted mid-window would otherwise stack a second copy under the same name.
	MoveComp->RemoveRootMotionSource(InstanceName);

	TSharedPtr<FYogRootMotionSource_CurveMotion> Source = MakeShared<FYogRootMotionSource_CurveMotion>();
	Source->InstanceName = InstanceName;
	Source->AccumulateMode = GetAccumulateMode();
	Source->Duration = FMath::Max(WindowDuration, 0.f);
	Source->bFromSequence = !(Animation && Animation->IsA<UAnimMontage>());
	ConfigureSource(*Source);

	MoveComp->ApplyRootMotionSource(Source);
}

void UANS_CurveMotionBase::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
									 const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ACharacter* Character = GetNetRelevantCharacter(MeshComp);
	if (!Character)
	{
		return;
	}

	// On a normal end the source times out on its own Duration, which keeps the full distance/yaw and
	// keeps server and client in step. Removing it here would cut the tail at whatever frame each machine
	// happened to fire NotifyEnd. Only an interrupted or stopped montage cancels the remaining motion.
	const FAnimMontageInstance* MontageInstance = CurveMotion_FindMontageInstance(MeshComp, EventReference);
	const bool bMontageStillRunning = MontageInstance && MontageInstance->IsActive();
	if (bMontageStillRunning)
	{
		return;
	}

	if (!Animation || !Animation->IsA<UAnimMontage>())
	{
		// Notify on a plain sequence (e.g. inside the AnimBP): no montage instance to inspect, let it time out.
		return;
	}

	Character->GetCharacterMovement()->RemoveRootMotionSource(MakeInstanceName());
}
