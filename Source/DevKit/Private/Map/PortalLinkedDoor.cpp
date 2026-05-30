#include "Map/PortalLinkedDoor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

APortalLinkedDoor::APortalLinkedDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
	LeftDoorMesh->SetupAttachment(SceneRoot);

	RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
	RightDoorMesh->SetupAttachment(SceneRoot);
}

void APortalLinkedDoor::BeginPlay()
{
	Super::BeginPlay();

	if (bCloseOnBeginPlay)
	{
		ApplyClosedPose();
	}
}

void APortalLinkedDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bOpening)
	{
		return;
	}

	OpenElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(OpenElapsed / FMath::Max(OpenDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	ApplyOpenPose(EasedAlpha);

	if (Alpha >= 1.0f)
	{
		bOpening = false;
		bDoorOpen = true;
	}
}

void APortalLinkedDoor::OpenDoor()
{
	if (bDoorOpen || bOpening)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(OpenDelayTimer);
	if (OpenDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(OpenDelayTimer, this, &APortalLinkedDoor::StartOpenAnimation, OpenDelay, false);
	}
	else
	{
		StartOpenAnimation();
	}
}

void APortalLinkedDoor::CloseDoor()
{
	GetWorldTimerManager().ClearTimer(OpenDelayTimer);
	bOpening = false;
	bDoorOpen = false;
	OpenElapsed = 0.0f;
	ApplyClosedPose();
}

void APortalLinkedDoor::StartOpenAnimation()
{
	if (bDoorOpen || bOpening)
	{
		return;
	}

	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	}

	OpenElapsed = 0.0f;
	bOpening = true;
	ApplyOpenPose(0.0f);
}

void APortalLinkedDoor::ApplyClosedPose()
{
	if (LeftDoorMesh)
	{
		LeftDoorMesh->SetRelativeRotation(LeftClosedRotation);
	}
	if (RightDoorMesh)
	{
		RightDoorMesh->SetRelativeRotation(RightClosedRotation);
	}
}

void APortalLinkedDoor::ApplyOpenPose(float Alpha)
{
	const FQuat LeftQuat = FQuat::Slerp(LeftClosedRotation.Quaternion(), LeftOpenRotation.Quaternion(), Alpha);
	const FQuat RightQuat = FQuat::Slerp(RightClosedRotation.Quaternion(), RightOpenRotation.Quaternion(), Alpha);

	if (LeftDoorMesh)
	{
		LeftDoorMesh->SetRelativeRotation(LeftQuat.Rotator());
	}
	if (RightDoorMesh)
	{
		RightDoorMesh->SetRelativeRotation(RightQuat.Rotator());
	}
}
