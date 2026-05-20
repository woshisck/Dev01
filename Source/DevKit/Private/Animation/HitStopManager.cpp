#include "Animation/HitStopManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UHitStopManager::RequestMontageHitStop(UAnimInstance* InAnimInst,
	float FrozenDuration, float SlowDuration, float SlowRate, float CatchUpRate)
{
	if (!InAnimInst) return;

	// 冻结阶段优先：正在冻结时忽略新请求，防止暴击连击叠加产生卡顿
	if (Phase == EPhase::Frozen) return;

	if (!AnimInstances.Contains(InAnimInst))
		AnimInstances.Add(InAnimInst);
	CachedFrozenDur   = FrozenDuration;
	CachedSlowDur     = SlowDuration;
	CachedSlowRate    = FMath::Clamp(SlowRate, 0.01f, 1.0f);
	CachedCatchUpRate = FMath::Max(CatchUpRate, 1.01f);

	// 追帧时长 = 慢放丢失帧量 / (CatchUpRate - 1)
	const float MissedTime = SlowDuration * (1.f - CachedSlowRate);
	CachedCatchUpDur = (SlowDuration > 0.f && MissedTime > 0.f)
		? MissedTime / (CachedCatchUpRate - 1.f)
		: 0.f;

	if (FrozenDuration > 0.f)
	{
		Phase              = EPhase::Frozen;
		PhaseStartRealTime = FPlatformTime::Seconds();
		PauseMontage();
		FreezeCharacterMovement();
	}
	else
	{
		TransitionToSlow();
	}
}

void UHitStopManager::Tick(float /*DeltaTime*/)
{
	const double Elapsed = FPlatformTime::Seconds() - PhaseStartRealTime;

	if (Phase == EPhase::Frozen && Elapsed >= CachedFrozenDur)
	{
		TransitionToSlow();
	}
	else if (Phase == EPhase::Slow && Elapsed >= CachedSlowDur)
	{
		TransitionToCatchUp();
	}
	else if (Phase == EPhase::CatchUp && Elapsed >= CachedCatchUpDur)
	{
		EndHitStop();
	}
}

void UHitStopManager::TransitionToSlow()
{
	ResumeMontage();
	RestoreCharacterMovement();

	if (CachedSlowDur > 0.f)
	{
		Phase              = EPhase::Slow;
		PhaseStartRealTime = FPlatformTime::Seconds();
		SetPlayRate(CachedSlowRate);
	}
	else
	{
		EndHitStop();
	}
}

void UHitStopManager::TransitionToCatchUp()
{
	if (CachedCatchUpDur > 0.f)
	{
		Phase              = EPhase::CatchUp;
		PhaseStartRealTime = FPlatformTime::Seconds();
		SetPlayRate(CachedCatchUpRate);
	}
	else
	{
		EndHitStop();
	}
}

void UHitStopManager::EndHitStop()
{
	Phase = EPhase::None;
	SetPlayRate(1.f);
	RestoreCharacterMovement();
	AnimInstances.Reset();
}

void UHitStopManager::FreezeCharacterMovement()
{
	for (TWeakObjectPtr<UAnimInstance>& Weak : AnimInstances)
	{
		if (UAnimInstance* AI = Weak.Get())
		{
			if (USkeletalMeshComponent* Mesh = AI->GetOwningComponent())
			{
				if (ACharacter* Char = Cast<ACharacter>(Mesh->GetOwner()))
				{
					if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
					{
						CMC->DisableMovement();
					}
				}
			}
		}
	}
}

void UHitStopManager::RestoreCharacterMovement()
{
	for (TWeakObjectPtr<UAnimInstance>& Weak : AnimInstances)
	{
		if (UAnimInstance* AI = Weak.Get())
		{
			if (USkeletalMeshComponent* Mesh = AI->GetOwningComponent())
			{
				if (ACharacter* Char = Cast<ACharacter>(Mesh->GetOwner()))
				{
					if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
					{
						if (CMC->MovementMode == MOVE_None)
						{
							CMC->SetMovementMode(MOVE_Walking);
						}
					}
				}
			}
		}
	}
}

void UHitStopManager::PauseMontage()
{
	for (TWeakObjectPtr<UAnimInstance>& Weak : AnimInstances)
		if (UAnimInstance* AI = Weak.Get())
			if (UAnimMontage* M = AI->GetCurrentActiveMontage())
				AI->Montage_Pause(M);
}

void UHitStopManager::ResumeMontage()
{
	for (TWeakObjectPtr<UAnimInstance>& Weak : AnimInstances)
		if (UAnimInstance* AI = Weak.Get())
			if (UAnimMontage* M = AI->GetCurrentActiveMontage())
				AI->Montage_Resume(M);
}

void UHitStopManager::SetPlayRate(float Rate)
{
	for (TWeakObjectPtr<UAnimInstance>& Weak : AnimInstances)
		if (UAnimInstance* AI = Weak.Get())
			if (UAnimMontage* M = AI->GetCurrentActiveMontage())
				AI->Montage_SetPlayRate(M, Rate);
}

void UHitStopManager::Deinitialize()
{
	if (Phase != EPhase::None)
		EndHitStop();
	Super::Deinitialize();
}
