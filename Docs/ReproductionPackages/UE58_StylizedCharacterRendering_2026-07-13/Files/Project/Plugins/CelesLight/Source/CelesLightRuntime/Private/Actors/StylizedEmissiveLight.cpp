#include "Actors/StylizedEmissiveLight.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSourceMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> DefaultEmissiveMaterial(
		TEXT("/Game/Art/Material/LightingTools/M_StylizedHiddenEmissive.M_StylizedHiddenEmissive"), LOAD_NoWarn);
	SourceMesh = DefaultSourceMesh.Object;
	EmissiveMaterial = DefaultEmissiveMaterial.Get();

	EmissiveSource = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmissiveSource"));
	EmissiveSource->SetupAttachment(SceneRoot);
	EmissiveSource->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmissiveSource->SetGenerateOverlapEvents(false);
	EmissiveSource->SetCastShadow(false);
	EmissiveSource->SetHiddenInGame(true);
	EmissiveSource->SetRenderInMainPass(true);
	EmissiveSource->SetAffectDynamicIndirectLighting(true);
	EmissiveSource->SetAffectIndirectLightingWhileHidden(true);
	EmissiveSource->SetEmissiveLightSource(true);
	EmissiveSource->SetStaticMesh(SourceMesh);
	EmissiveSource->SetMaterial(0, EmissiveMaterial);
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
	LightComponent->SetVisibility(bUsePointLightProxy);

	if (!EmissiveSource)
	{
		return;
	}

	EmissiveSource->SetStaticMesh(SourceMesh);
	EmissiveSource->SetHiddenInGame(true);
	EmissiveSource->SetAffectDynamicIndirectLighting(true);
	EmissiveSource->SetAffectIndirectLightingWhileHidden(true);
	EmissiveSource->SetEmissiveLightSource(true);

	if (!EmissiveMaterialInstance || EmissiveMaterialInstance->Parent != EmissiveMaterial)
	{
		EmissiveMaterialInstance = EmissiveMaterial
			? UMaterialInstanceDynamic::Create(EmissiveMaterial, this, TEXT("StylizedHiddenEmissiveMID"))
			: nullptr;
	}
	if (EmissiveMaterialInstance)
	{
		EmissiveMaterialInstance->SetVectorParameterValue(TEXT("Emissive Color"), LightColor);
		EmissiveMaterialInstance->SetScalarParameterValue(TEXT("Emissive Intensity"), FMath::Max(0.0f, EmissiveIntensity));
		EmissiveSource->SetMaterial(0, EmissiveMaterialInstance);
	}
	else
	{
		EmissiveSource->SetMaterial(0, EmissiveMaterial);
	}
}
