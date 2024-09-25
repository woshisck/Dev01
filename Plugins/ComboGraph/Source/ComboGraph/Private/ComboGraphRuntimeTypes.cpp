// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "ComboGraphRuntimeTypes.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/GameplayAbilityTypes.h"

void UComboGraphTargetType::GetTargets_Implementation(AActor* TargetingActor, const FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	const FHitResult* ContextHitResult = EventData.ContextHandle.GetHitResult();
	FHitResult TargetDataHitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData, 0);

#if ENGINE_MAJOR_VERSION == 5
	AActor* TargetActor = TargetDataHitResult.GetActor();
#else
	AActor* TargetActor = TargetDataHitResult.Actor.IsValid() ? TargetDataHitResult.GetActor() : nullptr;
#endif

	if (ContextHitResult)
	{
		OutHitResults.Add(*ContextHitResult);
	}
	else if (TargetActor)
	{
		OutHitResults.Add(TargetDataHitResult);
	}
	else if (EventData.Target)
	{
		const AActor* Actor = EventData.Target;
		OutActors.Add(const_cast<AActor*>(Actor));
	}
}

bool FComboGraphGameplayEffectContainerSpec::HasValidEffects() const
{
	return TargetGameplayEffectSpecs.Num() > 0;
}

bool FComboGraphGameplayEffectContainerSpec::HasValidTargets() const
{
	return TargetData.Num() > 0;
}

void FComboGraphGameplayEffectContainerSpec::AddTargets(const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors)
{
	for (const FHitResult& HitResult : HitResults)
	{
		FGameplayAbilityTargetData_SingleTargetHit* NewData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
		TargetData.Add(NewData);
	}

	if (TargetActors.Num() > 0)
	{
		FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();
		NewData->TargetActorArray.Append(TargetActors);
		TargetData.Add(NewData);
	}
}
