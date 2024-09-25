// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Abilities/TargetTypes/GSCTargetTypeUseEventData.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"

void UGSCTargetTypeUseEventData::GetTargets_Implementation(AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	const FHitResult* FoundHitResult = EventData.ContextHandle.GetHitResult();
	const FHitResult TargetDataHitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData, 0);

	if (FoundHitResult)
	{
		OutHitResults.Add(*FoundHitResult);
	}
	else if (TargetDataHitResult.IsValidBlockingHit())
	{
		OutHitResults.Add(TargetDataHitResult);
	}
	else if (EventData.Target)
	{
		const AActor* Actor = EventData.Target;
		OutActors.Add(const_cast<AActor*>(Actor));
	}
}
