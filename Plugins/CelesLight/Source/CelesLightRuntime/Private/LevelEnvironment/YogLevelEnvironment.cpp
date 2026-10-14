#include "LevelEnvironment/YogLevelEnvironment.h"

#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "StylizedLightingSettings.h"

namespace
{
	bool UsesYogGlobalEnvironment(const UWorld* World)
	{
		return World && (World->WorldType == EWorldType::Editor
			|| World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
	}
}

FYogLevelEnvironmentSettings FYogLevelEnvironmentSettings::Sanitized() const
{
	FYogLevelEnvironmentSettings Result = *this;
	// Old maps/configs can still carry the abandoned rendering settings. They
	// cannot opt any world back into custom exposure or composed-color quantization.
	Result.CharacterExposureEV = 0.0f;
	Result.bEnableColorLayers = false;
	Result.ColorLayerCount = 5;
	Result.ColorLayerSoftness = 1.0f;
	Result.ColorLayerStrength = 0.0f;
	Result.bColorLayersAffectCharacters = false;
	Result.bColorLayersAffectScene = false;
	Result.ColorLayerMinEV = -8.0f;
	Result.ColorLayerMaxEV = 6.0f;
	return Result;
}

bool FYogLevelEnvironmentSettings::HasRenderEffect() const
{
	return false;
}

FLinearColor FYogLevelEnvironmentSettings::TransformLinearColor(const FLinearColor& Color, bool bCharacter, float PreExposure) const
{
	// Compatibility API only. Native Lumen, material shading and camera exposure
	// own the full pixel value; the retired tool never modifies RGB or alpha.
	return Color;
}

UYogLevelEnvironmentComponent::UYogLevelEnvironmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FYogLevelEnvironmentSettings UYogLevelEnvironmentComponent::GetEffectiveSettings() const
{
	if (!bOverrideGlobalSettings)
	{
		return GetYogGlobalEnvironmentSettings();
	}
	return (Profile && !bOverrideProfile ? Profile->Settings : Settings).Sanitized();
}

AYogLevelEnvironment::AYogLevelEnvironment()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	EnvironmentComponent = CreateDefaultSubobject<UYogLevelEnvironmentComponent>(TEXT("Environment"));
	PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITOR
	// A level look must not disappear when a distant WP cell is unloaded.
	SetIsSpatiallyLoaded(false);
#endif
}

FYogLevelEnvironmentSettings GetYogGlobalEnvironmentSettings()
{
	check(IsInGameThread());
	return GetDefault<UStylizedLightingSettings>()->GlobalEnvironment.Sanitized();
}

const AYogLevelEnvironment* FindYogLevelEnvironmentOverride(const UWorld* World)
{
	check(IsInGameThread());
	const AYogLevelEnvironment* Best = nullptr;
	if (UsesYogGlobalEnvironment(World))
	{
		for (TActorIterator<AYogLevelEnvironment> It(const_cast<UWorld*>(World)); It; ++It)
		{
			const AYogLevelEnvironment* Actor = *It;
			const UYogLevelEnvironmentComponent* Component = Actor->EnvironmentComponent;
			if (!IsValid(Component) || !Component->bEnabled || !Component->bOverrideGlobalSettings || Actor->IsActorBeingDestroyed()
				|| !Actor->GetLevel() || !Actor->GetLevel()->bIsVisible)
			{
				continue;
			}
			if (!Best || Component->Priority > Best->EnvironmentComponent->Priority
				|| (Component->Priority == Best->EnvironmentComponent->Priority && Actor->GetPathName() < Best->GetPathName()))
			{
				Best = Actor;
			}
		}
	}
	return Best;
}

FYogLevelEnvironmentSettings ResolveYogLevelEnvironment(const UWorld* World)
{
	check(IsInGameThread());
	if (!UsesYogGlobalEnvironment(World))
	{
		return FYogLevelEnvironmentSettings();
	}
	const AYogLevelEnvironment* Override = FindYogLevelEnvironmentOverride(World);
	return Override ? Override->EnvironmentComponent->GetEffectiveSettings() : GetYogGlobalEnvironmentSettings();
}
