// Fill out your copyright notice in the Description page of Project Settings.

#include "Volume/YogCameraVolume.h"
#include "Components/BrushComponent.h"
#include "Camera/YogPlayerCameraManager.h"
#include "Character/PlayerCharacterBase.h"
#include "Kismet/GameplayStatics.h"

AYogCameraVolume::AYogCameraVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetBrushComponent()->SetHiddenInGame(false);
}

void AYogCameraVolume::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AYogCameraVolume::OnOverlapBegin);
	OnActorEndOverlap.AddDynamic(this, &AYogCameraVolume::OnOverlapEnd);
}

void AYogCameraVolume::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;

	if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(
		UGameplayStatics::GetPlayerCameraManager(this, 0)))
	{
		CM->SetConstraintVolume(this);
	}
}

void AYogCameraVolume::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;

	if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(
		UGameplayStatics::GetPlayerCameraManager(this, 0)))
	{
		CM->SetConstraintVolume(nullptr);
	}
}
