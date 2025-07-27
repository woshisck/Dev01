// Fill out your copyright notice in the Description page of Project Settings.


#include "YogCameraVolume.h"
#include "Components/BrushComponent.h"
#include "Async/TaskGraphInterfaces.h"
#include "../Camera/YogCameraPawn.h"
#include "../Character/PlayerCharacterBase.h"

AYogCameraVolume::AYogCameraVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetBrushComponent()->SetCollisionObjectType(ECC_WorldStatic);
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	GetBrushComponent()->SetGenerateOverlapEvents(true);
	this->bDisplayShadedVolume = true;
	// Optional: Make the volume visible in game
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
	UE_LOG(LogTemp, Warning, TEXT("OverlappedActor: %s"), *OverlappedActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("OtherActor: %s"), *OtherActor->GetName());

	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(OtherActor);
	//AYogCameraPawn* Camera = Cast<AYogPlayerControllerBase>(player->GetController())
	AYogCameraPawn* Camera = player->GetOwnCamera();
	if (Camera)
	{
		Camera->SetCameraStates(EYogCameraStates::Idle);
	}

}

void AYogCameraVolume::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(OtherActor);
	//AYogCameraPawn* Camera = Cast<AYogPlayerControllerBase>(player->GetController())

	AYogCameraPawn* Camera = player->GetOwnCamera();
	if (Camera)
	{
		Camera->SetCameraStates(Camera->PrevStatus);
	}

}
