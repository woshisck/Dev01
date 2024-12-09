// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraVolume.h"
#include "Components/BrushComponent.h"
#include "Async/TaskGraphInterfaces.h"

ACameraVolume::ACameraVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Overlap);
}
