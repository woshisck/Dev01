#include "AbilitySystem/GameplayCue/GCN_PlayerHitImpact.h"

#include "Camera/PlayerCameraManager.h"
#include "Camera/YogPlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "AbilitySystem/GameplayCue/HitCueData.h"
#include "AbilitySystem/GameplayCue/GlobalHitShakeData.h"
#include "System/YogSettings.h"

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

	// Camera shake: prefer the per-hit discrete level from the cue payload (heavy = 1,
	// crit-promoted = CritCameraShakeLevel). When no level is configured, fall back to
	// the global damage-scaled shake on UYogSettings::HitShakeConfig.
	int32 ShakeLevel = 0;
	if (CueData)
	{
		static const FGameplayTag TAG_CritHit = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.CritHit"));
		const bool bCrit = Parameters.AggregatedSourceTags.HasTag(TAG_CritHit);
		ShakeLevel = (bCrit && CueData->CritCameraShakeLevel > 0)
			? CueData->CritCameraShakeLevel
			: CueData->CameraShakeLevel;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);

	if (ShakeLevel > 0)
	{
		if (AYogPlayerCameraManager* CM = PC ? Cast<AYogPlayerCameraManager>(PC->PlayerCameraManager) : nullptr)
		{
			CM->PlayShakeLevel(ShakeLevel);
		}
	}
	else
	{
		const UYogSettings* Settings = UYogSettings::Get();
		const UGlobalHitShakeData* ShakeConfig = Settings ? Settings->HitShakeConfig.LoadSynchronous() : nullptr;
		if (ShakeConfig && ShakeConfig->CameraShakeClass && PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(
				ShakeConfig->CameraShakeClass,
				ShakeConfig->ResolveShakeScale(Parameters.RawMagnitude));
		}
	}

	return true;
}
