// Fill out your copyright notice in the Description page of Project Settings.



#include "YogCameraComponent.h"
#include "../Character/YogBaseCharacter.h"

UYogCameraComponent::UYogCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UYogCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//UpdatePosition();
}

FVector UYogCameraComponent::GetDesiredLocation(const FVector& GroundLoc)
{
	//AActor* Owner = this->GetOwner();
	//FVector OwnerLoc = Owner->GetActorLocation();

	FVector CameraFowardVector = this->GetForwardVector();
	FVector RevertForwardVec = CameraFowardVector * -1.f;

	FVector DesiredLoc = (CameraHeight / RevertForwardVec.Z) * RevertForwardVec + GroundLoc;

	return DesiredLoc;
}

void UYogCameraComponent::InitCamera()
{
}

void UYogCameraComponent::UpdatePosition()
{	
	//FVector DesiredLoc = GetDesiredLocation();
	//this->SetWorldLocation(DesiredLoc);
	
	//FVector offset = GetOffsetDirection();
	//this->AddWorldOffset(offset);
}

FVector UYogCameraComponent::GetOffsetDirection()
{
	AYogBaseCharacter* Owner = Cast<AYogBaseCharacter>(this->GetOwner());
	AController* Controller = Owner->GetController();



	FVector OwnerLoc = Owner->GetActorLocation();

	return FVector(0, 0, 0);
}


