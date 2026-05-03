#include "BuffFlow/Actors/Rune512FlipbookVFXActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARune512FlipbookVFXActor::ARune512FlipbookVFXActor()
{
	PrimaryActorTick.bCanEverTick = true;

	PlaneComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	PlaneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaneComponent->SetGenerateOverlapEvents(false);
	PlaneComponent->SetReceivesDecals(false);
	SetRootComponent(PlaneComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjectPlaneMesh(
		TEXT("/Game/Free_Magic/Mesh/SM_Free_Magic_Plane1.SM_Free_Magic_Plane1"));
	if (ProjectPlaneMesh.Succeeded())
	{
		PlaneComponent->SetStaticMesh(ProjectPlaneMesh.Object);
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> EnginePlaneMesh(
			TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (EnginePlaneMesh.Succeeded())
		{
			PlaneComponent->SetStaticMesh(EnginePlaneMesh.Object);
		}
	}
}

void ARune512FlipbookVFXActor::BeginPlay()
{
	Super::BeginPlay();
	StartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	UpdateMaterialTime();
}

void ARune512FlipbookVFXActor::InitializeFlipbook(
	UTexture2D* InTexture,
	UMaterialInterface* InMaterial,
	UStaticMesh* InPlaneMesh,
	int32 InRows,
	int32 InColumns,
	float InDuration,
	float InSize,
	bool bInFaceCamera,
	const FLinearColor& InEmissiveColor,
	float InAlphaScale)
{
	if (InPlaneMesh)
	{
		PlaneComponent->SetStaticMesh(InPlaneMesh);
	}

	if (InMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(InMaterial, this);
		PlaneComponent->SetMaterial(0, DynamicMaterial);
	}

	const float SafeSize = FMath::Max(1.f, InSize);
	const float Scale = SafeSize / 100.f;
	PlaneComponent->SetRelativeScale3D(FVector(Scale));

	Duration = FMath::Max(0.01f, InDuration);
	bFaceCamera = bInFaceCamera;
	StartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (DynamicMaterial)
	{
		DynamicMaterial->SetTextureParameterValue(TEXT("FlipbookTexture"), InTexture);
		DynamicMaterial->SetScalarParameterValue(TEXT("Rows"), FMath::Max(1, InRows));
		DynamicMaterial->SetScalarParameterValue(TEXT("Columns"), FMath::Max(1, InColumns));
		DynamicMaterial->SetScalarParameterValue(TEXT("Duration"), Duration);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), InEmissiveColor);
		DynamicMaterial->SetScalarParameterValue(TEXT("AlphaScale"), FMath::Max(0.f, InAlphaScale));
	}

	UpdateMaterialTime();
	FaceCamera();
	SetLifeSpan(Duration);
}

void ARune512FlipbookVFXActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMaterialTime();
	if (bFaceCamera)
	{
		FaceCamera();
	}
}

void ARune512FlipbookVFXActor::UpdateMaterialTime()
{
	if (!DynamicMaterial || !GetWorld())
	{
		return;
	}

	DynamicMaterial->SetScalarParameterValue(
		TEXT("Time"),
		FMath::Max(0.f, GetWorld()->GetTimeSeconds() - StartWorldTime));
}

void ARune512FlipbookVFXActor::FaceCamera()
{
	if (!GetWorld())
	{
		return;
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APlayerCameraManager* CameraManager = PC ? PC->PlayerCameraManager : nullptr;
	if (!CameraManager)
	{
		return;
	}

	const FVector ToCamera = CameraManager->GetCameraLocation() - GetActorLocation();
	if (!ToCamera.IsNearlyZero())
	{
		SetActorRotation(ToCamera.Rotation());
	}
}
