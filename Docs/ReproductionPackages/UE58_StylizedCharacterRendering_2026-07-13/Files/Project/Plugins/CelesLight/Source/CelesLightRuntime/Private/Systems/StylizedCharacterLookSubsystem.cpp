#include "Systems/StylizedCharacterLookSubsystem.h"

#include "Actors/StylizedCharacterLookVolume.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "StylizedCharacterLighting.h"

void UStylizedCharacterLookSubsystem::Deinitialize()
{
	SetStylizedCharacterLightingProfileOverride(INDEX_NONE, 0.0f);
	Super::Deinitialize();
}

void UStylizedCharacterLookSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
	if (!CameraManager)
	{
		SetStylizedCharacterLightingProfileOverride(INDEX_NONE, 0.0f);
		return;
	}

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	AStylizedCharacterLookVolume* BestVolume = nullptr;
	float BestWeight = 0.0f;

	for (TActorIterator<AStylizedCharacterLookVolume> It(World); It; ++It)
	{
		AStylizedCharacterLookVolume* Volume = *It;
		if (!Volume || !Volume->bEnableCharacterLook || !Volume->bEnabled || Volume->BlendWeight <= 0.0f)
		{
			continue;
		}

		float Weight = FMath::Clamp(Volume->BlendWeight, 0.0f, 1.0f);
		if (!Volume->bUnbound)
		{
			float DistanceToVolume = 0.0f;
			if (!Volume->EncompassesPoint(CameraLocation, FMath::Max(Volume->BlendRadius, 0.0f), &DistanceToVolume))
			{
				continue;
			}

			if (Volume->BlendRadius > KINDA_SMALL_NUMBER)
			{
				Weight *= 1.0f - FMath::Clamp(DistanceToVolume / Volume->BlendRadius, 0.0f, 1.0f);
			}
		}

		if (!BestVolume || Volume->Priority > BestVolume->Priority || (Volume->Priority == BestVolume->Priority && Weight > BestWeight))
		{
			BestVolume = Volume;
			BestWeight = Weight;
		}
	}

	SetStylizedCharacterLightingProfileOverride(BestVolume ? BestVolume->LightingProfile : INDEX_NONE, BestWeight);
}

TStatId UStylizedCharacterLookSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStylizedCharacterLookSubsystem, STATGROUP_Tickables);
}

bool UStylizedCharacterLookSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
