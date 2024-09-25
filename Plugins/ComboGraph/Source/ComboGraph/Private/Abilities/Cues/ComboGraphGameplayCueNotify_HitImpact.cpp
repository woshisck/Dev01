// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Abilities/Cues/ComboGraphGameplayCueNotify_HitImpact.h"

#include "AbilitySystemGlobals.h"
#include "ComboGraphAbilityTypes.h"
#include "ComboGraphLog.h"
#include "GameplayCueManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

bool UComboGraphGameplayCueNotify_HitImpact::HandlesEvent(EGameplayCueEvent::Type Event) const
{
	return (Event == EGameplayCueEvent::Executed);
}

void UComboGraphGameplayCueNotify_HitImpact::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	check(EventType == EGameplayCueEvent::Executed);

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueNotify_HitImpact HandleGameplayCue Target: %s"), *GetNameSafe(MyTarget))

	const UObject* WorldContextObject = MyTarget;
	if (!WorldContextObject)
	{
		WorldContextObject = UAbilitySystemGlobals::Get().GetGameplayCueManager();
	}

	// If we don't have valid custom cue params in effect context, cue params source object can still be used to pass individual VFX / SFX asset
	UObject* SourceObject = nullptr;
	if (Parameters.SourceObject.IsValid())
	{
		SourceObject = const_cast<UObject*>(Parameters.SourceObject.Get());
	}

	// Grab our custom context
	const FComboGraphGameplayEffectContext* Context = static_cast<const FComboGraphGameplayEffectContext*>(Parameters.EffectContext.Get());
	if (Context == nullptr)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphGameplayCueNotify_HitImpact No Valid Context :((((( Can't get back custom cue paramaters from effect context. Aborting gameplay cue handling."))
		return;
	}

	if (WorldContextObject)
	{
		// We have cue params already filled in (no async load needed)
		if (Context && Context->GetCueParamsObjects().Num() > 0)
		{
			for (TWeakObjectPtr<UObject> CueObject : Context->GetCueParamsObjects())
			{
				if (CueObject.IsValid())
				{
					HandleSourceObject(WorldContextObject, CueObject.Get(), MyTarget, EventType, Parameters);
				}
			}
		}

		// We have cue params passed down as soft object paths (async load needed)
		else if (Context && Context->GetCueParamsObjectsPaths().Num() > 0)
		{
			// Load all assets in one batch
			if (bAsyncLoadAllAtOnce)
			{
				const TArray<FSoftObjectPath> AssetsToLoad = Context->GetCueParamsObjectsPaths();
				AsyncLoadAssets(
					AssetsToLoad,
					FStreamableDelegate::CreateUObject(this, &UComboGraphGameplayCueNotify_HitImpact::OnMissingAssetsAsyncLoadComplete, AssetsToLoad, WorldContextObject, MyTarget, EventType, Parameters)
				);
			}
			else
			{
				for (FSoftObjectPath SoftObjectPath : Context->GetCueParamsObjectsPaths())
				{
					UObject* ResolvedObject = SoftObjectPath.ResolveObject();

					// If we already have a loaded object matching this path in memory, handle right away
					if (ResolvedObject != nullptr)
					{
						CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueNotify_HitImpact - HandleGameplayCue SourceObject %s is already resolved, handle right away"), *GetNameSafe(ResolvedObject));
						HandleSourceObject(WorldContextObject, ResolvedObject, MyTarget, EventType, Parameters);
					}
					// Otherwise, queue up an async request and handle on complete
					else if (SoftObjectPath.IsValid())
					{
						AsyncLoadAsset(
							SoftObjectPath,
							FStreamableDelegate::CreateUObject(this, &UComboGraphGameplayCueNotify_HitImpact::OnMissingAssetAsyncLoadComplete, SoftObjectPath, WorldContextObject, MyTarget, EventType, Parameters)
						);
					}
				}
			}
		}

		// Otherwise, forward to appropriate handler
		else
		{
			HandleSourceObject(WorldContextObject, SourceObject, MyTarget, EventType, Parameters);
		}
	}
}

void UComboGraphGameplayCueNotify_HitImpact::AsyncLoadAssets(const TArray<FSoftObjectPath>& AssetsToLoad, const FStreamableDelegate DoneDelegate)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueNotify_HitImpact::AsyncLoadAssets %d asset to load"), AssetsToLoad.Num())

	if (AssetsToLoad.Num() == 0)
	{
		DoneDelegate.ExecuteIfBound();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	CG_RUNTIME_LOG(Verbose, TEXT("  - Streamable Manager %s"), *StreamableManager.GetManagerName())

	for (FSoftObjectPath SoftObjectPath : AssetsToLoad)
	{
		CG_RUNTIME_LOG(Verbose, TEXT("  - Soft Path: %s"), *SoftObjectPath.ToString())
	}

	StreamableManager.RequestAsyncLoad(AssetsToLoad, DoneDelegate);
}

void UComboGraphGameplayCueNotify_HitImpact::AsyncLoadAsset(const FSoftObjectPath& AssetToLoad, FStreamableDelegate DoneDelegate)
{
	AsyncLoadAssets(TArray<FSoftObjectPath>{AssetToLoad}, MoveTemp(DoneDelegate));
}

void UComboGraphGameplayCueNotify_HitImpact::OnMissingAssetsAsyncLoadComplete(TArray<FSoftObjectPath> LoadedObjects, const UObject* WorldContextObject, AActor* MyTarget, const EGameplayCueEvent::Type Event, const FGameplayCueParameters Parameters)
{
	for (const FSoftObjectPath& LoadedObject : LoadedObjects)
	{
		HandleMissingAssetAsyncLoad(LoadedObject, WorldContextObject, MyTarget, Event, Parameters);
	}
}

void UComboGraphGameplayCueNotify_HitImpact::OnMissingAssetAsyncLoadComplete(const FSoftObjectPath LoadedObject, const UObject* WorldContextObject, AActor* MyTarget, const EGameplayCueEvent::Type Event, const FGameplayCueParameters Parameters)
{
	HandleMissingAssetAsyncLoad(LoadedObject, WorldContextObject, MyTarget, Event, Parameters);
}

void UComboGraphGameplayCueNotify_HitImpact::HandleMissingAssetAsyncLoad(const FSoftObjectPath& LoadedObject, const UObject* WorldContextObject, AActor* MyTarget, const EGameplayCueEvent::Type Event, const FGameplayCueParameters& Parameters)
{
	UObject* ResolvedObject = LoadedObject.ResolveObject();
	if (ResolvedObject == nullptr)
	{
		// Load failed
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphGameplayCueNotify_HitImpact - Async load of asset %s failed!"), *LoadedObject.ToString());
		return;
	}

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueNotify_HitImpact::HandleMissingAssetAsyncLoad - HandleSourceObject %s"), *GetNameSafe(ResolvedObject));
	HandleSourceObject(WorldContextObject, ResolvedObject, MyTarget, Event, Parameters);
}

void UComboGraphGameplayCueNotify_HitImpact::HandleSourceObject(const UObject* WorldContextObject, UObject* SourceObject, AActor* MyTarget, const EGameplayCueEvent::Type Event, const FGameplayCueParameters& Parameters)
{
	if (UParticleSystem* ParticleSystem = Cast<UParticleSystem>(SourceObject))
	{
		HandleCascadeParticleSystem(WorldContextObject, ParticleSystem, MyTarget, Event, Parameters);
	}
	else if (UNiagaraSystem* SystemTemplate = Cast<UNiagaraSystem>(SourceObject))
	{
		HandleNiagaraParticleSystem(WorldContextObject, SystemTemplate, MyTarget, Event, Parameters);
	}
	else if (USoundBase* SoundAsset = Cast<USoundBase>(SourceObject))
	{
		HandleSoundBase(WorldContextObject, SoundAsset, MyTarget, Event, Parameters);
	}
}

void UComboGraphGameplayCueNotify_HitImpact::HandleCascadeParticleSystem(const UObject* WorldContextObject, UParticleSystem* ParticleSystem, AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphGameplayCueNotify_HitImpact HandleCascadeParticleSystem: %s for %s"), *GetNameSafe(ParticleSystem), *GetNameSafe(MyTarget))

	FVector Location;
	FRotator Rotation;

	const FHitResult* HitResult = Parameters.EffectContext.GetHitResult();
	if (HitResult)
	{
		GetSpawnLocationAndRotationFromHitResult(HitResult, Location, Rotation);
	}
	else
	{
		GetSpawnLocationAndRotationFromParameters(MyTarget, Parameters, Location, Rotation);
	}

	UGameplayStatics::SpawnEmitterAtLocation(WorldContextObject, ParticleSystem, Location, Rotation, CascadeSpawnScale);
}

void UComboGraphGameplayCueNotify_HitImpact::HandleNiagaraParticleSystem(const UObject* WorldContextObject, UNiagaraSystem* SystemTemplate, AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphGameplayCueNotify_HitImpact HandleNiagaraParticleSystem: %s for %s"), *GetNameSafe(SystemTemplate), *GetNameSafe(MyTarget))

	FVector Location;
	FRotator Rotation;

	const FHitResult* HitResult = Parameters.EffectContext.GetHitResult();
	if (HitResult)
	{
		GetSpawnLocationAndRotationFromHitResult(HitResult, Location, Rotation);
	}
	else
	{
		GetSpawnLocationAndRotationFromParameters(MyTarget, Parameters, Location, Rotation);
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, SystemTemplate, Location, Rotation, NiagaraSpawnScale);
}

void UComboGraphGameplayCueNotify_HitImpact::HandleSoundBase(const UObject* WorldContextObject, USoundBase* SoundAsset, AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	CG_RUNTIME_LOG(Log, TEXT("UComboGraphGameplayCueNotify_HitImpact HandleSoundBase: %s for %s"), *GetNameSafe(SoundAsset), *GetNameSafe(MyTarget))

	FVector Location;
	FRotator Rotation;

	const FHitResult* HitResult = Parameters.EffectContext.GetHitResult();
	if (HitResult)
	{
		GetSpawnLocationAndRotationFromHitResult(HitResult, Location, Rotation);
	}
	else
	{
		GetSpawnLocationAndRotationFromParameters(MyTarget, Parameters, Location, Rotation);
	}

	UGameplayStatics::PlaySoundAtLocation(
		WorldContextObject,
		SoundAsset,
		Location,
		Rotation,
		VolumeMultiplier,
		PitchMultiplier,
		StartTime,
		AttenuationSettings,
		ConcurrencySettings,
		MyTarget
	);
}

void UComboGraphGameplayCueNotify_HitImpact::GetSpawnLocationAndRotationFromParameters(const AActor* MyTarget, const FGameplayCueParameters& Parameters, FVector& Location, FRotator& Rotation)
{
	Location = Parameters.Location;
	Rotation = Parameters.Normal.Rotation();

	if (MyTarget)
	{
		// If either location / rotation are not initialized properly, fallback to target actor location / rotation
		if (Location.IsZero())
		{
			Location = MyTarget->GetActorLocation();
		}

		if (Rotation.IsZero())
		{
			Rotation = MyTarget->GetActorRotation();
		}
	}
}

void UComboGraphGameplayCueNotify_HitImpact::GetSpawnLocationAndRotationFromHitResult(const FHitResult* HitResult, FVector& Location, FRotator& Rotation)
{
	check(HitResult);

	Location = HitResult->ImpactPoint;
	Rotation = HitResult->ImpactNormal.Rotation();
}
