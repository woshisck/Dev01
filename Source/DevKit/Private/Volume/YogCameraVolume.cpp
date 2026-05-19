// Fill out your copyright notice in the Description page of Project Settings.

#include "Volume/YogCameraVolume.h"
#include "Components/BrushComponent.h"
#include "Camera/YogPlayerCameraManager.h"
#include "Character/PlayerCharacterBase.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

AYogCameraVolume::AYogCameraVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetBrushComponent()->SetHiddenInGame(false);
}

void AYogCameraVolume::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AYogCameraVolume::OnOverlapBegin);
	OnActorEndOverlap.AddDynamic(this, &AYogCameraVolume::OnOverlapEnd);
}

void AYogCameraVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	if (!bShowDebugInGame)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	const FBox Bounds = GetComponentsBoundingBox(true);
	if (!Bounds.IsValid)
	{
		return;
	}

	DrawDebugBox(
		World,
		Bounds.GetCenter(),
		Bounds.GetExtent(),
		DebugColor,
		false,
		-1.f,
		0,
		DebugLineThickness);
#endif
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
