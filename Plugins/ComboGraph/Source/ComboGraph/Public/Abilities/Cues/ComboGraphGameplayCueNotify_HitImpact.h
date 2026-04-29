// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Engine/StreamableManager.h"
#include "ComboGraphGameplayCueNotify_HitImpact.generated.h"

class UNiagaraSystem;
class USoundAttenuation;
class USoundConcurrency;

/**
 * Non instanced GameplayCueNotify for spawning particle and sound FX.
 *
 * Slightly modified version of UGameplayCueNotify_HitImpact to handle spawning of particle / sound FX from passed in Effect Context in Cue Paramaters.
 *
 * Checks Objects to test if it is:
 *
 * - A Niagara Emitter template, and will spawn emitter at location
 * - A Cascade Particle system, and will spawn system at location
 * - A Sound Base (Cue, Wave, ...), and will spawn sound at location
 *
 * Cue parameters should only contain the soft object paths to the actual asset that this cue handler will async load if necessary.
 */
UCLASS(Blueprintable, Category = "GameplayCue")
class COMBOGRAPH_API UComboGraphGameplayCueNotify_HitImpact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	/**
	 * Whether to do a single async load request for all assets that are passed down through gameplay cue parameters effect context.
	 *
	 * Set it to true to issue a single async load resolved when all the assets are loaded in memory.
	 *
	 * Set it to false to load all assets in parallel issuing multiple requests and resolved individually.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	bool bAsyncLoadAllAtOnce = false;

	/** For passed down assets that are Niagara Systems, this is the scale vector to use. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Niagara Settings")
	FVector NiagaraSpawnScale = FVector(1.f);

	/** For passed down assets that are Cascade Emitters, this is the scale vector to use. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Cascade Settings")
	FVector CascadeSpawnScale = FVector(1.f);

	/** A linear scalar multiplied with the volume, in order to make the sound louder or softer. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Sound Settings")
	float VolumeMultiplier = 1.f;

	/** PitchMultiplier - A linear scalar multiplied with the pitch. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Sound Settings")
	float PitchMultiplier = 1.f;

	/** How far in to the sound to begin playback at */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Sound Settings")
	float StartTime = 0.f;

	/** Override attenuation settings package to play sound with */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Sound Settings")
	TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;

	/**
	 * Override concurrency settings package to play sound with
	 *
	 * (The actor used as the "owner" for concurrency settings purposes will be the target of the gameplay cue. Allows PlaySound calls to do a concurrency limit per owner.)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Sound Settings")
	TObjectPtr<USoundConcurrency> ConcurrencySettings = nullptr;

	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	/** Gather assets to load for a cue container and request async load from Streamable Manager, invoking DoneDelegate when loading is complete. */
	static void AsyncLoadAssets(const TArray<FSoftObjectPath>& AssetsToLoad, FStreamableDelegate DoneDelegate = FStreamableDelegate());
	static void AsyncLoadAsset(const FSoftObjectPath& AssetToLoad, FStreamableDelegate DoneDelegate = FStreamableDelegate());

	void OnMissingAssetsAsyncLoadComplete(TArray<FSoftObjectPath> LoadedObjects, const UObject* WorldContextObject, AActor* MyTarget, EGameplayCueEvent::Type Event, const FGameplayCueParameters Parameters);
	void OnMissingAssetAsyncLoadComplete(const FSoftObjectPath LoadedObject, const UObject* WorldContextObject, AActor* MyTarget, EGameplayCueEvent::Type Event, const FGameplayCueParameters Parameters);
	void HandleMissingAssetAsyncLoad(const FSoftObjectPath& LoadedObject, const UObject* WorldContextObject, AActor* MyTarget, EGameplayCueEvent::Type Event, const FGameplayCueParameters& Parameters);

	/** Checks passed in SourceObject and forwards to appropriate handler */
	virtual void HandleSourceObject(const UObject* WorldContextObject, UObject* SourceObject, AActor* MyTarget, EGameplayCueEvent::Type Event, const FGameplayCueParameters& Parameters);

	virtual void HandleCascadeParticleSystem(const UObject* WorldContextObject, UParticleSystem* ParticleSystem, AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters);
	virtual void HandleNiagaraParticleSystem(const UObject* WorldContextObject, UNiagaraSystem* SystemTemplate, AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters);
	virtual void HandleSoundBase(const UObject* WorldContextObject, USoundBase* SoundAsset, AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters);

	/** Extract location and rotation information from Cue Parameters, and fallback to target actor location / rotation if cue params are zeroed out */
	static void GetSpawnLocationAndRotationFromParameters(const AActor* MyTarget, const FGameplayCueParameters& Parameters, FVector& Location, FRotator& Rotation);

	/** Extract location and rotation information from Hit Result */
	static void GetSpawnLocationAndRotationFromHitResult(const FHitResult* HitResult, FVector& Location, FRotator& Rotation);
};
