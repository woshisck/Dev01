#include "Actors/StylizedEmissiveLight.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"

AStylizedEmissiveLight::AStylizedEmissiveLight()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightComponent"));
	LightComponent->SetupAttachment(SceneRoot);
	LightComponent->Mobility = EComponentMobility::Movable;
	LightComponent->SetCastShadows(false);
	LightComponent->SetIntensity(Intensity);
	LightComponent->SetAttenuationRadius(AttenuationRadius);
	LightComponent->SetLightColor(LightColor);
}

void AStylizedEmissiveLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshLight();
}

void AStylizedEmissiveLight::RefreshLight()
{
	if (!LightComponent)
	{
		return;
	}

	LightComponent->SetLightColor(LightColor);
	LightComponent->SetIntensity(FMath::Max(0.0f, Intensity));
	LightComponent->SetAttenuationRadius(FMath::Max(1.0f, AttenuationRadius));
	LightComponent->SetSourceRadius(FMath::Max(0.0f, SourceRadius));
	LightComponent->SetUseInverseSquaredFalloff(bUseInverseSquaredFalloff);
	LightComponent->SetCastShadows(bCastShadows);
}
