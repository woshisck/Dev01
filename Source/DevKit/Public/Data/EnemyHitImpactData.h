// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EnemyHitImpactData.generated.h"

class USoundBase;
class UNiagaraSystem;

/** Sound + VFX payload for one intensity level of one entry. */
USTRUCT(BlueprintType)
struct FHitImpactFX
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact", meta = (ClampMin = "0.0"))
	float SoundVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact", meta = (ClampMin = "0.0"))
	float SoundPitch = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	TObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	FVector VFXScale = FVector(1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	FRotator VFXRotationOffset = FRotator::ZeroRotator;
};

/**
 * One rule: "while the victim owns MatchTag, contribute this feedback".
 * MatchTag is matched hierarchically, so a parent tag entry covers all its children.
 */
USTRUCT(BlueprintType)
struct FHitImpactEntry
{
	GENERATED_BODY()

	// Either a HitReact.Material.* surface tag or any state tag the victim can own (Buff.Status.*).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	FGameplayTag MatchTag;

	// Higher wins the cascade. Ties resolve in array order, so keep distinct values for clarity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	int32 Priority = 0;

	// Selected by the attacker's weapon HitImpactLevel (1-based, out of range clamps).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact")
	TArray<FHitImpactFX> Levels;

	// 0 = contribute no shake. Resolved via UYogSettings::CameraShakeLevelTable / PlayShakeLevel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitImpact", meta = (ClampMin = "0"))
	int32 CameraShakeLevel = 0;
};

/** Flattened cascade output. Not reflected — plain by-value result of a Resolve call. */
struct FHitImpactResolved
{
	USoundBase* Sound = nullptr;
	float SoundVolume = 1.f;
	float SoundPitch = 1.f;

	UNiagaraSystem* VFX = nullptr;
	FVector VFXScale = FVector(1.f);
	FRotator VFXRotationOffset = FRotator::ZeroRotator;

	int32 CameraShakeLevel = 0;
};

/**
 * Project-wide victim hit feedback table, keyed by GameplayTag. Referenced from
 * UYogSettings::EnemyHitImpactData and read by UHitImpactVisualComponent at hit time.
 *
 * This is the single definition site for hit-contact sound. The attacker side contributes only
 * the pre-contact swing whoosh (UWeaponDefinition::WhiffSound), which is independent of hit
 * resolution and never mixes with these entries in the same frame.
 */
UCLASS(BlueprintType)
class DEVKIT_API UEnemyHitImpactData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	TArray<FHitImpactEntry> Entries;

	/**
	 * Resolves every entry whose MatchTag the victim owns into one result, highest Priority first.
	 * Each field independently takes the first value that is set as the cascade descends, so a
	 * high-priority state entry can contribute only VFX and still inherit the material sound
	 * beneath it.
	 * Level is 1-based and clamps into each entry's Levels array.
	 */
	FHitImpactResolved Resolve(const FGameplayTagContainer& VictimTags, int32 Level) const;
};
