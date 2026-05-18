#include "Animation/HitStopManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"

void UHitStopManager::RequestMontageHitStop(UAnimInstance* InAnimInst,
	float FrozenDuration, float SlowDuration, float SlowRate, float CatchUpRate,
	EHitStopScope Scope)
{
	if (Scope == EHitStopScope::SelfMontage && !InAnimInst) return;

	// 冻结阶段优先：正在冻结时忽略新请求，防止暴击连击叠加产生卡顿
	if (Phase == EPhase::Frozen) return;

	CachedScope       = Scope;
	AnimInst          = InAnimInst;
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
	AnimInst.Reset();
}

void UHitStopManager::PauseMontage()
{
	if (CachedScope == EHitStopScope::GlobalDilation)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 0.0001f);
		return;
	}
	if (UAnimInstance* AI = AnimInst.Get())
		if (UAnimMontage* M = AI->GetCurrentActiveMontage())
			AI->Montage_Pause(M);
}

void UHitStopManager::ResumeMontage()
{
	if (CachedScope == EHitStopScope::GlobalDilation)
	{
		// Global dilation is corrected by the immediately following SetPlayRate call
		return;
	}
	if (UAnimInstance* AI = AnimInst.Get())
		if (UAnimMontage* M = AI->GetCurrentActiveMontage())
			AI->Montage_Resume(M);
}

void UHitStopManager::SetPlayRate(float Rate)
{
	if (CachedScope == EHitStopScope::GlobalDilation)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, Rate);
		return;
	}
	if (UAnimInstance* AI = AnimInst.Get())
		if (UAnimMontage* M = AI->GetCurrentActiveMontage())
			AI->Montage_SetPlayRate(M, Rate);
}

void UHitStopManager::Deinitialize()
{
	if (Phase != EPhase::None)
		EndHitStop();
	Super::Deinitialize();
}
