// Copyright 2021, Project Zero, All Rights Reserved.


#include "TopDownPhysicsCameraActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "FTopDownPhysicsCameraActorModule"

// Each rotator reverses the previous rotation applied.
const FVector RightArmRelRotation = FVector(0, 0, 90);
const FVector LeftArmRelRotation = FVector(0, 0, 270);
const FVector UpArmRelRotation = FVector(0, 0, 180);
const FVector DownArmRelRotation = FVector(0, 0, 0);


// Sets default values
ATopDownPhysicsCameraActor::ATopDownPhysicsCameraActor()
{
	RootSceneComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);

	RightArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("RightArm"));
	RightArm->SetupAttachment(RootSceneComponent);

	RightArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RightArrow"));
	RightArrow->SetupAttachment(RightArm);

	LeftArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("LeftArm"));
	LeftArm->SetupAttachment(RightArm);

	LeftArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftArrow"));
	LeftArrow->SetupAttachment(LeftArm);

	UpArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("UpArm"));
	UpArm->SetupAttachment(LeftArm);

	UpArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("UpArrow"));
	UpArrow->SetupAttachment(UpArm);

	DownArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("DownArm"));
	DownArm->SetupAttachment(UpArm);

	DownArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DownArrow"));
	DownArrow->SetupAttachment(DownArm);

	ScreenArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ScreenArm"));
	ScreenArm->SetupAttachment(DownArm);
	ScreenArm->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(ScreenArm);

	//ScreenArm->SetRelativeRotation(FRotator::MakeFromEuler(CameraRotator));
	RightArm->SetRelativeRotation(FRotator::MakeFromEuler(RightArmRelRotation));
	LeftArm->SetRelativeRotation(FRotator::MakeFromEuler(LeftArmRelRotation));
	UpArm->SetRelativeRotation(FRotator::MakeFromEuler(UpArmRelRotation));
	DownArm->SetRelativeRotation(FRotator::MakeFromEuler(DownArmRelRotation));
	//CameraComponent->SetRelativeRotation(FRotator::MakeFromEuler(CameraRelRotation));

	RootSceneComponent->SetGenerateOverlapEvents(false);
	RootSceneComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	ScreenArmSetup();
	DirectionalArmSetup();
	ArrowSetup();
}

// Called when the game starts or when spawned
void ATopDownPhysicsCameraActor::BeginPlay()
{
	Super::BeginPlay();
	auto PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->SetViewTarget(this);
	}
	//ScreenArm->bDoCollisionTest = false;
}

//To toggle visual possession
void ATopDownPhysicsCameraActor::TakeOverView()
{
	auto PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->SetViewTarget(this);
	}
}

#if WITH_EDITOR
void ATopDownPhysicsCameraActor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	DirectionalArmSetup();
	ScreenArmSetup();
	ArrowSetup();
}
#endif

void ATopDownPhysicsCameraActor::ToggleDirectionalArmCollision(bool bNewDoCollisionTest)
{
	TArray<USpringArmComponent*> SpringArms = { RightArm, LeftArm, UpArm, DownArm };

	for (int i = 0; i < SpringArms.Num(); i++)
	{
		auto& SpringArm = SpringArms[i];
		if (SpringArm)
		{
			SpringArm->bDoCollisionTest = bNewDoCollisionTest;
		}
	}
}

void ATopDownPhysicsCameraActor::SetDirectionalArmCameraLag(bool NewEnableCameraLag, float CameraLagSpeed)
{
	TArray<USpringArmComponent*> SpringArms = { RightArm, LeftArm, UpArm, DownArm };
	for (int i = 0; i < SpringArms.Num(); i++)
	{
		auto& SpringArm = SpringArms[i];
		if (SpringArm)
		{
			SpringArm->CameraLagSpeed = CameraLagSpeed;
			SpringArm->bUseCameraLagSubstepping = true;
			SpringArm->bEnableCameraLag = NewEnableCameraLag;
		}
		UE_LOG(LogTemp, Warning, TEXT("The float value is: %f"), CameraLagSpeed);
	}
}

void ATopDownPhysicsCameraActor::SetDirectionalArmCameraLagVertical(bool NewEnableCameraLag, float CameraLagSpeed)
{
	TArray<USpringArmComponent*> SpringArms = {UpArm, DownArm };
	for (int i = 0; i < SpringArms.Num(); i++)
	{
		auto& SpringArm = SpringArms[i];
		if (SpringArm)
		{
			SpringArm->CameraLagSpeed = CameraLagSpeed;
			SpringArm->bUseCameraLagSubstepping = true;
			SpringArm->bEnableCameraLag = NewEnableCameraLag;
		}
	}
}

void ATopDownPhysicsCameraActor::SetDirectionalArmCameraLagHorizontal(bool NewEnableCameraLag, float CameraLagSpeed)
{
	TArray<USpringArmComponent*> SpringArms = {RightArm, LeftArm};
	for (int i = 0; i < SpringArms.Num(); i++)
	{
		auto& SpringArm = SpringArms[i];
		if (SpringArm)
		{
			SpringArm->CameraLagSpeed = CameraLagSpeed;
			SpringArm->bUseCameraLagSubstepping = true;
			SpringArm->bEnableCameraLag = NewEnableCameraLag;
		}
	}
}

void ATopDownPhysicsCameraActor::SetVerticalArmCollision(bool bEnableCollisionTest)
{
	TArray<USpringArmComponent*> SpringArms = { UpArm, DownArm };
	for (auto& SpringArm : SpringArms)
	{
		if (SpringArm)
		{
			SpringArm->bDoCollisionTest = bEnableCollisionTest;
		}
	}
}

void ATopDownPhysicsCameraActor::SetHorizontalArmCollision(bool bEnableCollisionTest)
{
	TArray<USpringArmComponent*> SpringArms = { RightArm, LeftArm };
	for (auto& SpringArm : SpringArms)
	{
		if (SpringArm)
		{
			SpringArm->bDoCollisionTest = bEnableCollisionTest;
		}
	}
}

void ATopDownPhysicsCameraActor::ScreenArmSetup()
{
	if (!ScreenArm) return;
	ScreenArm->TargetArmLength = DistanceFromCharacter;
	ScreenArm->bEnableCameraLag = this->bEnablePlayerCameraLag;
	ScreenArm->CameraLagSpeed = this->PlayerCameraLagSpeed;
	// Additional shift, e.g. to have the player be able to look further ahead than if the camera was centered
	ScreenArm->SocketOffset = CameraAdditionalLocalOffset;
	ScreenArm->bDoCollisionTest = false; // Don't want the Z arm to collide with the character itself
	ScreenArm->bInheritPitch = false;
	ScreenArm->bInheritRoll = false;
	ScreenArm->bInheritYaw = false;
	//ScreenArm->SetRelativeLocation(DownCollisionDistance, 0, 0);
	ScreenArm->SetRelativeRotation(CameraRotator);
}

void ATopDownPhysicsCameraActor::DirectionalArmSetup()
{
	ScreenArm->bDoCollisionTest = false;
	ScreenArm->bEnableCameraLag = true;
	ScreenArm->ProbeSize = this->CameraCollisionProbeSize;
	ScreenArm->ProbeChannel = this->CameraCollisionProbeChannel;
	ScreenArm->bInheritPitch = false;
	ScreenArm->bInheritRoll = false;
	ScreenArm->bInheritYaw = false;
	ScreenArm->CameraLagSpeed = 0;


	TArray<USpringArmComponent*> SpringArms = { RightArm, LeftArm, UpArm, DownArm, ScreenArm };
	for (int i = 0; i < SpringArms.Num(); i++)
	{
		auto& SpringArm = SpringArms[i];
		if (SpringArm)
		{
			if (i < 4)
			{
				SpringArm->bDoCollisionTest = this->bDoCollisionTest;
			}
			else if (i == 4) 
			{
				SpringArm->bDoCollisionTest = false;
			}
			
			SpringArm->bEnableCameraLag = true;
			SpringArm->ProbeSize = this->CameraCollisionProbeSize;
			SpringArm->ProbeChannel = this->CameraCollisionProbeChannel;
			SpringArm->bInheritPitch = false;
			SpringArm->bInheritRoll = false;
			SpringArm->bInheritYaw = false;
			SpringArm->CameraLagSpeed = PlayerCameraLagSpeed;


			if (bUseUniformCollisionDistance == true)
			{
				if (i < 4)
				{
					SpringArm->TargetArmLength = UniformCollisionDistance;
				}

				if (i != 0)
				{
					// Every arm except the first one (right) needs to be set back to the center.
					SpringArm->SetRelativeLocation(FVector(UniformCollisionDistance, 0, 0));
				}
				
			}
			else if (bUseUniformCollisionDistance == false)
			{
				switch (i)
				{
				case 0: // Right
					SpringArm->TargetArmLength = RightCollisionDistance;
					break;
				case 1: // Left
					SpringArm->TargetArmLength = LeftCollisionDistance;
					SpringArm->SetRelativeLocation(FVector(RightCollisionDistance, 0, 0));
					break;
				case 2: // Up
					SpringArm->TargetArmLength = UpCollisionDistance;
					SpringArm->SetRelativeLocation(FVector(LeftCollisionDistance, 0, 0));
					break;
				case 3: // Down
					SpringArm->TargetArmLength = DownCollisionDistance;
					SpringArm->SetRelativeLocation(FVector(UpCollisionDistance, 0, 0));
					break;
				case 4: // Screen
						SpringArm->TargetArmLength = DistanceFromCharacter;
						SpringArm->SetRelativeLocation(FVector(DownCollisionDistance, 0, 0));
						SpringArm->bDoCollisionTest = false;
				default:
					//check(false);
					break;
				}
			}
		}
	}
}

void ATopDownPhysicsCameraActor::ArrowSetup()
{
	TArray<UArrowComponent*> Arrows = { RightArrow, LeftArrow, UpArrow, DownArrow };
	for (auto& Arrow : Arrows)
	{
		if (Arrow)
		{
			Arrow->SetHiddenInGame(!bShowDebug);
		}
	}
}


