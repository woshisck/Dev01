#include "AbilitySystem/GameplayCue/GCN_PlayerHitImpact.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "AbilitySystem/GameplayCue/HitCueData.h"

AGCN_PlayerHitImpact::AGCN_PlayerHitImpact()
{
	bAutoDestroyOnRemove = true;
	bAllowMultipleOnActiveEvents = false;
}

bool AGCN_PlayerHitImpact::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	Super::OnExecute_Implementation(Target, Parameters);

	UWorld* World = Target ? Target->GetWorld() : GetWorld();
	if (!World)
	{
		return false;
	}

	if (bSkipDedicatedServer && World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	// Per-hit visual payload (weapon/notify) overrides the class defaults when provided.
	const UHitCueData* CueData = Cast<UHitCueData>(Parameters.SourceObject.Get());

	UNiagaraSystem* EffectiveVFX = CueData && CueData->ImpactVFX ? CueData->ImpactVFX.Get() : ImpactVFX.Get();
	const FRotator EffectiveVFXRotationOffset = CueData ? CueData->VFXRotationOffset : VFXRotationOffset;
	const FVector EffectiveVFXScale = CueData ? CueData->VFXScale : VFXScale;
	USoundBase* EffectiveSound = CueData && CueData->ImpactSound ? CueData->ImpactSound.Get() : ImpactSound.Get();
	const float EffectiveSoundVolume = CueData ? CueData->SoundVolumeMultiplier : SoundVolumeMultiplier;
	const float EffectiveSoundPitch = CueData ? CueData->SoundPitchMultiplier : SoundPitchMultiplier;
	const TSubclassOf<UCameraShakeBase> EffectiveShakeClass = CueData && CueData->CameraShakeClass ? CueData->CameraShakeClass : CameraShakeClass;
	const float EffectiveShakeScale = CueData ? CueData->CameraShakeScale : CameraShakeScale;

	const FVector HitLocation = Parameters.Location.IsZero() && Target
		? Target->GetActorLocation()
		: FVector(Parameters.Location);

	// Orient VFX to face away from the surface along the hit normal when available.
	FRotator HitRotation = Parameters.Normal.IsZero()
		? FRotator::ZeroRotator
		: Parameters.Normal.ToOrientationRotator();
	HitRotation += EffectiveVFXRotationOffset;

	if (EffectiveVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			EffectiveVFX,
			HitLocation,
			HitRotation,
			EffectiveVFXScale,
			true,
			true,
			ENCPoolMethod::None,
			false);
	}

	if (EffectiveSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			World,
			EffectiveSound,
			HitLocation,
			HitRotation,
			EffectiveSoundVolume,
			EffectiveSoundPitch);
	}

	if (EffectiveShakeClass)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(EffectiveShakeClass, EffectiveShakeScale);
		}
	}

	return true;
}
