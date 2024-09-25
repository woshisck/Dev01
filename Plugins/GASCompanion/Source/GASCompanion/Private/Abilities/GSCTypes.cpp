// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Abilities/GSCTypes.h"

bool FGSCGameplayEffectContainerSpec::HasValidEffects() const
{
	return TargetGameplayEffectSpecs.Num() > 0;
}

bool FGSCGameplayEffectContainerSpec::HasValidTargets() const
{
	return TargetData.Num() > 0;
}

void FGSCGameplayEffectContainerSpec::AddTargets(const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors)
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
