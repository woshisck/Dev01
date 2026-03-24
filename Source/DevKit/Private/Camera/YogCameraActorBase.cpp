// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/YogCameraActorBase.h"

AYogCameraActorBase::AYogCameraActorBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// setting the rotation absolute so it does not change the rotation based on the attached character
	RootComponent->SetUsingAbsoluteRotation(true);

	CameraBoom = CreateDefaultSubobject<UYogSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bInheritYaw = false;

}

void AYogCameraActorBase::SetupTarget(APawn& targetPawn)
{

}

void AYogCameraActorBase::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);
}

void AYogCameraActorBase::DisplayDebug(UCanvas* canvas, const FDebugDisplayInfo& debugDisplay, float& yL, float& yPos)
{
	Super::DisplayDebug(canvas, debugDisplay, yL, yPos);
}

void AYogCameraActorBase::UpdateCameraAttachment()
{
}

void AYogCameraActorBase::UpdateSettings(float deltaTime)
{
}
