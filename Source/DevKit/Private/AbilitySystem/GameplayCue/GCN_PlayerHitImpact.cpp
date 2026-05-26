#include "AbilitySystem/GameplayCue/GCN_PlayerHitImpact.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

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

	const FVector HitLocation = Parameters.Location.IsZero() && Target
		? Target->GetActorLocation()
		: FVector(Parameters.Location);

	// Orient VFX to face away from the surface along the hit normal when available.
	FRotator HitRotation = Parameters.Normal.IsZero()
		? FRotator::ZeroRotator
		: Parameters.Normal.ToOrientationRotator();
	HitRotation += VFXRotationOffset;

	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			ImpactVFX,
			HitLocation,
			HitRotation,
			VFXScale,
			true,
			true,
			ENCPoolMethod::None,
			false);
	}

	if (ImpactSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			World,
			ImpactSound,
			HitLocation,
			HitRotation,
			SoundVolumeMultiplier,
			SoundPitchMultiplier);
	}

	if (CameraShakeClass)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(CameraShakeClass, CameraShakeScale);
		}
	}

	return true;
}
