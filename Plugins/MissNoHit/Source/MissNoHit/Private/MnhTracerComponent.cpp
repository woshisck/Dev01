// Copyright 2024 Eren Balatkan. All Rights Reserved.


#include "..\Public\MnhTracerComponent.h"

#include "ComponentUtils.h"
#include "GameplayTagContainer.h"
#include "..\Public\MissNoHit.h"
#include "MnhTracer.h"

DEFINE_LOG_CATEGORY(LogMnh)

class FMissNoHitModule;
// Sets default values for this component's properties
UMnhTracerComponent::UMnhTracerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMnhTracerComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	OnDestroyed.Broadcast();

	for (const auto& Tracer : Tracers)
	{
		if (Tracer)
		{
			Tracer->MarkTracerDataForRemoval();
		}
	}
}


void UMnhTracerComponent::InitializeTracers(const FGameplayTagContainer TracerTags, UPrimitiveComponent* TracerSource)
{
	if (!bIsInitialized)
	{
		EarlyTracerInitializations.Add(FTracerInitializationData{TracerTags, TracerSource});
		return;
	}
	
	if (!TracerSource)
	{
		const auto Owner = GetOwner();
		FString Message;
		if (!Owner)
		{
			Message = FString::Printf(TEXT("MissNoHit Warning: Tracer Source is not valid"));
		}
		else
		{
			Message = FString::Printf(TEXT("MissNoHit Warning: Tracer Source is not valid on object: %s"), *Owner->GetName());
		}
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
		return;
	}

	int InitializedTracerCount = 0;
	for (const auto Tracer : Tracers)
	{
		if(TracerTags.HasTagExact(Tracer->TracerTag))
		{
			Tracer->InitializeParameters(TracerSource);
			InitializedTracerCount++;
		}
	}
	if (InitializedTracerCount == 0)
	{
		const FString Message = FString::Printf(TEXT("MissNoHit Warning: No Tracer with Tag [%s] found during initialization"), *TracerTags.ToString());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
	}
}


void UMnhTracerComponent::StopAllTracers()
{
	for (const auto Tracer : Tracers)
	{
		Tracer->ChangeTracerState(false);
		OnTracerStopped.Broadcast(Tracer->TracerTag);
	}
}

void UMnhTracerComponent::ResetHitCache()
{
	HitCache.Reset();
}

void UMnhTracerComponent::StartTracers(FGameplayTagContainer TracerTags, const bool bResetHitCache)
{
	StartTracersInternal<false>(TracerTags, bResetHitCache);
}

template <bool AllowAnimNotify>
void UMnhTracerComponent::StartTracersInternal(const FGameplayTagContainer& TracerTags, bool bResetHitCache)
{
	SCOPE_CYCLE_COUNTER(STAT_MNHStartTracer);

	for (auto &[HitIdxTickCache, HitCacheTick] : HitCache)
	{
		TArray<int> HitIndexesToRemove;
		if (bResetHitCache)
		{
			for (const auto TracerTag : TracerTags)
			{
				for (int HitCacheElemIdx = 0; HitCacheElemIdx<HitCacheTick.Num(); HitCacheElemIdx++)
				{
					const auto& HitCacheElem = HitCacheTick[HitCacheElemIdx];
					if (HitCacheElem.TracerTag.MatchesTag(TracerTag))
					{
						HitIndexesToRemove.Add(HitCacheElemIdx);
					}
				}
			}
		}
	
		HitIndexesToRemove.Sort();
		for (int HitCacheElemIdx = HitIndexesToRemove.Num() - 1; HitCacheElemIdx>=0; HitCacheElemIdx--)
		{
			HitCacheTick.RemoveAt(HitCacheElemIdx);
		}
	}
	
	for (const auto Tracer : Tracers)
	{
		if (Tracer->SourceComponent == nullptr && Tracer->TraceSource != EMnhTraceSource::AnimNotify)
		{
			FMnhHelpers::Mnh_Log("MissNoHit Warning: Tracer Source is not initialized for Tracer: " + Tracer->TracerTag.ToString());
			return;
		}
		
		if (TracerTags.HasTagExact(Tracer->TracerTag))
		{
			if constexpr (!AllowAnimNotify)
			{
				if (Tracer->TraceSource == EMnhTraceSource::AnimNotify)
				{
					const FString DebugMessage = FString::Printf(TEXT("MissNoHit Warning: "
															   "AnimNotifyTracer [%s] on Owner [%s] cannot be started manually"),
															   *Tracer->TracerTag.ToString(), *GetOwner()->GetName());
					FMnhHelpers::Mnh_Log(DebugMessage);
					break;
				}
			}
			
			Tracer->ChangeTracerState(true);
			OnTracerStarted.Broadcast(Tracer->TracerTag);
		}
	}
}

void UMnhTracerComponent::StopTracers(const FGameplayTagContainer TracerTags)
{
	if (!this)
	{
		return;
	}
	for (const auto& Tracer : Tracers)
	{
		if (TracerTags.HasTagExact(Tracer->TracerTag))
		{
			Tracer->ChangeTracerState(false);
			OnTracerStopped.Broadcast(Tracer->TracerTag);
		}
	}
}

void UMnhTracerComponent::StopTracersDelayed(const FGameplayTagContainer& TracerTags)
{
	if (!this)
	{
		return;
	}
	for (const auto& Tracer : Tracers)
	{
		if (TracerTags.HasTagExact(Tracer->TracerTag))
		{
			Tracer->ChangeTracerState(false, false);
			OnTracerStopped.Broadcast(Tracer->TracerTag);
		}
	}
}

UMnhTracer* UMnhTracerComponent::FindTracer(const FGameplayTag TracerTag)
{
	for (const auto Tracer : Tracers)
	{
		if (Tracer->TracerTag.MatchesTagExact(TracerTag))
		{
			return Tracer;
		}
	}
	return nullptr;
}

// Called when the game starts
void UMnhTracerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	for (const auto& Tracer : Tracers)
	{
		if(!Tracer)
		{
			continue;
		}
		Tracer->OwnerComponent = this;
		Tracer->RegisterTracerData();
	}
	bIsInitialized = true;
	
	for (const auto& [TracerTags, TracerSource] : EarlyTracerInitializations)
	{
		InitializeTracers(TracerTags, TracerSource);
	}
}

bool UMnhTracerComponent::CheckFilters(const FHitResult& HitResult, const FGameplayTag TracerTag, int TickIdxArg)
{
	SCOPE_CYCLE_COUNTER(STAT_MNHCheckFilters);

	if (HitCache.Num() > 2048)
	{
		HitCache.Reset();
		const FString Message = FString::Printf(TEXT("MissNoHit Warning: Your Tracer Hit Cache"
													 " contains over 2048 elements, it is very likely that you are not resetting"
													 "your Hit Cache, this can lead to performance and memory leak problems!"
													 " Please enable MissNoHit tracers only when they are needed"));
		FMnhHelpers::Mnh_Log(Message);
	}

	// Filter if same actor or component was hit by same tracer in same tick. This is to ensure we don't produce multiple HitResults due to tick sub-stepping.
	if (HitCache.Contains(TickIdxArg))
	{
		for (const auto& HitCacheElem : HitCache[TickIdxArg])
		{
			if (HitCacheElem.TracerTag.MatchesTag(TracerTag) && HitCacheElem.TickIdx == TickIdxArg &&
				(HitCacheElem.HitResult.GetComponent() == HitResult.GetComponent() || HitCacheElem.HitResult.GetActor() == HitResult.GetActor()))
			{
				return false;
			}
		}
	}

	if (FilterType == EMnhFilterType::None)
	{
		return true;
	}

	for (auto& HitRecord : HitCache)
    {
        TArray<FMnhHitCache>& ValueArray = HitRecord.Value;
        for (const auto& HitCacheTick : ValueArray)
        {
        	if (FilterType == EMnhFilterType::FilterSameActorPerTracer)
        	{
        		if (HitCacheTick.TracerTag.MatchesTag(TracerTag) && HitCacheTick.HitResult.GetActor() == HitResult.GetActor())
        		{
        			return false;
        		}
        	}
        	else if (FilterType == EMnhFilterType::FilterSameActorAcrossAllTracers)
        	{
        		if (HitCacheTick.HitResult.GetActor() == HitResult.GetActor())
        		{
        			return false;
        		}
        	}
        }
    }
	
	return true;
}

bool UMnhTracerComponent::AreAllTracersRunning(const FGameplayTagContainer& TracerTags)
{
	bool bAreAllTracersRunning = true;
	for (const auto& Tracer  : Tracers)
	{
		if (TracerTags.HasTagExact(Tracer->TracerTag))
		{
			bAreAllTracersRunning &= Tracer->IsTracerActive();
		}
	}
	return bAreAllTracersRunning;
}

bool UMnhTracerComponent::AreAllTracersStopped(const FGameplayTagContainer& TracerTags)
{
	bool bAreAllTracersStopped = true;
	for (const auto& Tracer  : Tracers)
	{
		if (TracerTags.HasTagExact(Tracer->TracerTag))
		{
			bAreAllTracersStopped &= !Tracer->IsTracerActive();
		}
	}
	return bAreAllTracersStopped;
}

void UMnhTracerComponent::OnTracerHitDetected(FGameplayTag TracerTag, const TArray<FHitResult>& HitResults, const float DeltaTime, const int TickIdx)
{
	SCOPE_CYCLE_COUNTER(STAT_MnhTracerComponentHitDetected)
	FScopeLock ScopedLock(&TraceDoneScopeLock);

	for (const auto& HitResult : HitResults)
	{
		if(CheckFilters(HitResult, TracerTag, TickIdx))
		{
			auto& HitCacheTick = HitCache.FindOrAdd(TickIdx);
			HitCacheTick.Add(FMnhHitCache{HitResult, TracerTag, TickIdx});
			OnHitDetected.Broadcast(TracerTag, HitResult, DeltaTime);
		}
	}
}

