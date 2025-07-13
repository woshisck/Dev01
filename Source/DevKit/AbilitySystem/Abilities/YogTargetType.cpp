// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogTargetType.h"
#include "YogGameplayAbility.h"
#include <Devkit/Character/YogCharacterBase.h>

void UYogTargetType::GetTargets_Implementation(int HitboxIndex, AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
}

void UYogTargetType_UseOwner::GetTargets_Implementation(int HitboxIndex, AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	OutActors.Add(TargetingCharacter);
}

void UYogTargetType_UseEventData::GetTargets_Implementation(int HitboxIndex, AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	const FHitResult* FoundHitResult = EventData.ContextHandle.GetHitResult();
	if (FoundHitResult)
	{
		OutHitResults.Add(*FoundHitResult);
	}
	else if (EventData.Target)
	{
		//OutActors.Add(const_cast<AActor*>(EventData.Target));
		OutActors.Add(const_cast<AActor*>(ToRawPtr(EventData.Target)));
	}
}