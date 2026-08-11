// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/EnemyHitImpactData.h"

#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

FHitImpactResolved UEnemyHitImpactData::Resolve(const FGameplayTagContainer& VictimTags, int32 Level) const
{
	FHitImpactResolved Result;

	TArray<const FHitImpactEntry*, TInlineAllocator<8>> Matches;
	for (const FHitImpactEntry& Entry : Entries)
	{
		if (Entry.MatchTag.IsValid() && VictimTags.HasTag(Entry.MatchTag))
		{
			Matches.Add(&Entry);
		}
	}

	if (Matches.Num() == 0)
	{
		return Result;
	}

	// Stable so equal priorities fall back to authoring order rather than an arbitrary one.
	Matches.StableSort([](const FHitImpactEntry& A, const FHitImpactEntry& B)
		{
			return A.Priority > B.Priority;
		});

	bool bSoundSet = false;
	bool bVFXSet = false;

	for (const FHitImpactEntry* Entry : Matches)
	{
		if (Result.CameraShakeLevel == 0)
		{
			Result.CameraShakeLevel = Entry->CameraShakeLevel;
		}

		if (Entry->Levels.Num() == 0)
		{
			continue;
		}

		const int32 Index = FMath::Clamp(Level - 1, 0, Entry->Levels.Num() - 1);
		const FHitImpactFX& FX = Entry->Levels[Index];

		if (!bSoundSet && FX.Sound)
		{
			Result.Sound = FX.Sound.Get();
			Result.SoundVolume = FX.SoundVolume;
			Result.SoundPitch = FX.SoundPitch;
			bSoundSet = true;
		}

		if (!bVFXSet && FX.VFX)
		{
			Result.VFX = FX.VFX.Get();
			Result.VFXScale = FX.VFXScale;
			Result.VFXRotationOffset = FX.VFXRotationOffset;
			bVFXSet = true;
		}
	}

	return Result;
}
