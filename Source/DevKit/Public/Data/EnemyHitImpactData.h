// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyHitImpactData.generated.h"

class USoundBase;
class UNiagaraSystem;

/**
 * Physical read of a victim's surface when hit. Every enemy is classified into one tier;
 * the shared UEnemyHitImpactData maps each tier to a common sound/VFX so enemies of the
 * same tier feel alike without per-Blueprint authoring.
 */
UENUM(BlueprintType)
enum class EHitReactTier : uint8
{
	Soft        UMETA(DisplayName = "Soft (flesh/ooze/fungal)"),
	Hard        UMETA(DisplayName = "Hard (metal/chitin/bone/stone)"),
	Immaterial  UMETA(DisplayName = "Immaterial (void/spectral/aether)"),
};

USTRUCT(BlueprintType)
struct FHitImpactTierFX
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
};

/**
 * Project-wide table of hit sound/VFX per EHitReactTier. Referenced from
 * UYogSettings::EnemyHitImpactData and read by UHitImpactVisualComponent at hit time.
 */
UCLASS(BlueprintType)
class DEVKIT_API UEnemyHitImpactData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	FHitImpactTierFX Soft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	FHitImpactTierFX Hard;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	FHitImpactTierFX Immaterial;

	const FHitImpactTierFX& GetTierFX(EHitReactTier Tier) const
	{
		switch (Tier)
		{
		case EHitReactTier::Hard:       return Hard;
		case EHitReactTier::Immaterial: return Immaterial;
		default:                        return Soft;
		}
	}
};
