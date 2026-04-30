// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/MontageConfigDA.h"

#include "Data/MontageAttackDataAsset.h"

UMontageAttackDataAsset* UMontageConfigDA::ResolveAttackData(const FGameplayTagContainer& ContextTags) const
{
	UMontageAttackDataAsset* BestAttackData = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Min();

	for (UMontageNotifyEntry* Entry : Entries)
	{
		const UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Entry);
		if (!HitWindow)
		{
			continue;
		}

		for (const FTaggedMontageAttackData& Candidate : HitWindow->AttackDataCandidates)
		{
			if (Candidate.Matches(ContextTags) && Candidate.Priority > BestPriority)
			{
				BestAttackData = Candidate.AttackData.Get();
				BestPriority = Candidate.Priority;
			}
		}
	}

	return BestAttackData;
}
