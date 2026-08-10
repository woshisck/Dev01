// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "BuffHitFeedbackRow.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * Common on-hit feedback for a buffed victim, keyed by a Buff.* GameplayTag.
 * Rows live in a UDataTable referenced from Project Settings (UYogSettings::BuffHitFeedbackTable).
 * When a hit lands on a victim, GA_MeleeAttack picks the highest-Priority row whose BuffTag the
 * victim owns and plays its VFX + SFX (+ optional camera-shake level) at the victim's location.
 */
USTRUCT(BlueprintType)
struct FBuffHitFeedbackRow : public FTableRowBase
{
	GENERATED_BODY()

	// Matched hierarchically against the victim's owned tags via HasMatchingGameplayTag.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback")
	FGameplayTag BuffTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|VFX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|VFX")
	FVector VFXScale = FVector(1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|VFX")
	FRotator VFXRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|SFX")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|SFX", meta = (ClampMin = "0.0"))
	float SoundVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|SFX", meta = (ClampMin = "0.0"))
	float SoundPitch = 1.f;

	// When true and ImpactSound is set, suppress the victim's default tier hit sound this hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|SFX")
	bool bReplaceDefaultSound = true;

	// Optional. 0 = no shake. Resolved via UYogSettings::CameraShakeLevelTable / PlayShakeLevel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback|CameraShake", meta = (ClampMin = "0"))
	int32 CameraShakeLevel = 0;

	// Highest priority wins when a victim matches multiple rows (e.g. invincible beats poison).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffHitFeedback", meta = (ClampMin = "0"))
	int32 Priority = 0;
};
