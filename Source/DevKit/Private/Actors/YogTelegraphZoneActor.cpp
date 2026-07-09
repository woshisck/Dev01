#include "Actors/YogTelegraphZoneActor.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AYogTelegraphZoneActor::AYogTelegraphZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Decal is parented under the scene root so spawn yaw (enemy facing) rotates the footprint,
	// while the decal keeps its downward projection regardless of the actor's spawn rotation.
	ZoneDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("ZoneDecal"));
	ZoneDecal->SetupAttachment(SceneRoot);
	ZoneDecal->DecalSize = FVector(256.f, 300.f, 300.f);
	ZoneDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	ZoneDecal->SetVisibility(false);
}

void AYogTelegraphZoneActor::BeginPlay()
{
	Super::BeginPlay();

	if (ZoneMaterial)
	{
		ZoneDecal->SetDecalMaterial(ZoneMaterial);
		DynMaterial = ZoneDecal->CreateDynamicMaterialInstance();
	}
}

void AYogTelegraphZoneActor::Show(float RadiusCm, float HalfAngleDeg, FLinearColor Color)
{
	ZoneDecal->DecalSize = FVector(ZoneDecal->DecalSize.X, RadiusCm, RadiusCm);

	// Best-effort convenience params; harmless if the material does not declare them.
	if (DynMaterial)
	{
		DynMaterial->SetScalarParameterValue(FName(TEXT("ArcRadius")), RadiusCm);
		DynMaterial->SetScalarParameterValue(FName(TEXT("HalfAngle")), HalfAngleDeg);
		DynMaterial->SetVectorParameterValue(FName(TEXT("Color")), Color);
	}

	ZoneDecal->SetVisibility(true);
	BP_OnShow(RadiusCm, HalfAngleDeg, Color);
}

void AYogTelegraphZoneActor::Hide()
{
	ZoneDecal->SetVisibility(false);
	BP_OnHide();
}
