// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Abilities/TargetTypes/GSCTargetTypeUseOwner.h"
#include "Abilities/GameplayAbilityTypes.h"

void UGSCTargetTypeUseOwner::GetTargets_Implementation(AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	OutActors.Add(TargetingActor);
}
