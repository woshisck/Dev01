#include "Animation/HitStopManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

void UHitStopManager::RequestHitStop(float FrozenDuration, float SlowDuration, float SlowDilation)
{
	// 冻结阶段优先级最高：正在冻结时忽略新请求
	if (CurrentPhase == EPhase::Frozen) return;

	UWorld* World = GetWorld();
	if (!World) return;

	SavedTimeDilation  = World->GetWorldSettings()->TimeDilation;
	CachedFrozenDur   = FrozenDuration;
	CachedSlowDur     = SlowDuration;
	CachedSlowDilation = SlowDilation;

	if (FrozenDuration > 0.0f)
	{
		CurrentPhase       = EPhase::Frozen;
		PhaseStartRealTime = FPlatformTime::Seconds();
		ApplyDilation(0.0001f); // 近零而非绝对零，保证引擎仍能正常推进
	}
	else
	{
		TransitionToSlow();
	}
}

void UHitStopManager::Tick(float /*DeltaTime*/)
{
	// 使用真实时钟，不受 TimeDilation 影响
	const double Elapsed = FPlatformTime::Seconds() - PhaseStartRealTime;

	if (CurrentPhase == EPhase::Frozen && Elapsed >= CachedFrozenDur)
	{
		TransitionToSlow();
	}
	else if (CurrentPhase == EPhase::Slow && Elapsed >= CachedSlowDur)
	{
		EndHitStop();
	}
}

void UHitStopManager::TransitionToSlow()
{
	if (CachedSlowDur > 0.0f)
	{
		CurrentPhase       = EPhase::Slow;
		PhaseStartRealTime = FPlatformTime::Seconds();
		ApplyDilation(CachedSlowDilation);
	}
	else
	{
		EndHitStop();
	}
}

void UHitStopManager::EndHitStop()
{
	CurrentPhase = EPhase::None;
	ApplyDilation(SavedTimeDilation);
}

void UHitStopManager::ApplyDilation(float Dilation)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, Dilation);
	}
}

void UHitStopManager::Deinitialize()
{
	// 世界销毁前强制恢复，防止 dilation 泄漏到下一个关卡
	if (CurrentPhase != EPhase::None)
	{
		EndHitStop();
	}
	Super::Deinitialize();
}
