#include "Actors/YogTelegraphZoneActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AYogTelegraphZoneActor::AYogTelegraphZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Plane is parented under the scene root so the spawn yaw (enemy facing) orients the
	// footprint while the plane itself keeps lying flat.
	ZonePlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZonePlane"));
	ZonePlane->SetupAttachment(SceneRoot);
	ZonePlane->SetMobility(EComponentMobility::Movable);
	ZonePlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZonePlane->SetGenerateOverlapEvents(false);
	ZonePlane->SetCastShadow(false);
	ZonePlane->SetReceivesDecals(false);
	ZonePlane->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		ZoneMesh = PlaneFinder.Object;
	}
}

void AYogTelegraphZoneActor::BeginPlay()
{
	Super::BeginPlay();

	if (ZoneMesh)
	{
		ZonePlane->SetStaticMesh(ZoneMesh);
	}

	EnsureDynamicMaterial();
}

UMaterialInstanceDynamic* AYogTelegraphZoneActor::EnsureDynamicMaterial()
{
	if (DynMaterial)
	{
		return DynMaterial;
	}

	if (!ZoneMaterial)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s has no ZoneMaterial; the telegraph zone will be invisible. Assign it on the BP subclass."),
			*GetClass()->GetName());
		return nullptr;
	}

	if (!ZonePlane->GetStaticMesh())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s has no ZoneMesh; the telegraph zone has nothing to draw on."),
			*GetClass()->GetName());
		return nullptr;
	}

	DynMaterial = ZonePlane->CreateDynamicMaterialInstance(0, ZoneMaterial);
	return DynMaterial;
}

void AYogTelegraphZoneActor::Show(const FYogTelegraphShape& Shape, FLinearColor Color)
{
	const float SafeMeshSize = FMath::Max(MeshSize, 1.f);

	// Only Square makes the two axes differ; fan and circle stay square so the radial masks
	// keep a 1:1 aspect and do not shear.
	const bool bUseHalfWidth = Shape.ShapeType == EHitBoxType::Square && Shape.HalfWidth > KINDA_SMALL_NUMBER;
	const float LateralExtent = bUseHalfWidth ? Shape.HalfWidth : Shape.Radius;

	// The masks reach their outer edge at half the plane width, so a zone of radius R needs the
	// plane scaled to diameter 2R. X is the owner's forward, Y lateral.
	ZonePlane->SetRelativeLocation(FVector(0.f, 0.f, GroundZOffset));
	ZonePlane->SetRelativeScale3D(FVector(
		(Shape.Radius * 2.f) / SafeMeshSize,
		(LateralExtent * 2.f) / SafeMeshSize,
		1.f));

	// Best-effort convenience params; harmless if the material does not declare them.
	if (UMaterialInstanceDynamic* MID = EnsureDynamicMaterial())
	{
		MID->SetScalarParameterValue(FName(TEXT("ArcRadius")), Shape.Radius);
		MID->SetScalarParameterValue(FName(TEXT("HalfAngle")), Shape.HalfAngle);
		const float InnerRatio = Shape.Radius > KINDA_SMALL_NUMBER
			? FMath::Clamp(Shape.InnerRadius / Shape.Radius, 0.f, 1.f)
			: 0.f;
		MID->SetScalarParameterValue(FName(TEXT("InnerRadiusRatio")), InnerRatio);
		MID->SetVectorParameterValue(FName(TEXT("Color")), Color);
	}

	ZonePlane->SetVisibility(true);
	BP_OnShow(Shape, Color);
}

void AYogTelegraphZoneActor::StartProgress(float Duration)
{
	ProgressDuration = Duration;
	ProgressElapsed = 0.f;

	// No timeline to animate against, so show the finished zone instead of an empty one.
	PushProgressRatio(Duration > KINDA_SMALL_NUMBER ? 0.f : 1.f);
}

void AYogTelegraphZoneActor::AdvanceProgress(float DeltaSeconds)
{
	if (ProgressDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	ProgressElapsed += DeltaSeconds;
	PushProgressRatio(FMath::Clamp(ProgressElapsed / ProgressDuration, 0.f, 1.f));
}

void AYogTelegraphZoneActor::PushProgressRatio(float Ratio)
{
	if (UMaterialInstanceDynamic* MID = EnsureDynamicMaterial())
	{
		MID->SetScalarParameterValue(FName(TEXT("OuterRadiusRatio")), Ratio);
	}
}

void AYogTelegraphZoneActor::Hide()
{
	ZonePlane->SetVisibility(false);
	BP_OnHide();
}
