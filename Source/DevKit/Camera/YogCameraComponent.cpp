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
	UpdatePosition();
}

FVector UYogCameraComponent::GetDesiredLocation()
{
	AActor* Owner = this->GetOwner();
	FVector OwnerLoc = Owner->GetActorLocation();

	FVector CameraFowardVector = this->GetForwardVector();
	FVector RevertForwardVec = CameraFowardVector * -1.f;

	FVector DesiredLoc = (CameraHeight / RevertForwardVec.Z) * RevertForwardVec + OwnerLoc;

	return DesiredLoc;
}

void UYogCameraComponent::InitCamera()
{
	AActor* Owner = this->GetOwner();
	FVector OwnerLoc = Owner->GetActorLocation();

	FVector CameraFowardVector = this->GetForwardVector();
	FVector RevertForwardVec = CameraFowardVector * -1.f;

	FVector AttachLoc = (CameraHeight / RevertForwardVec.Z) * RevertForwardVec + OwnerLoc;
}

void UYogCameraComponent::UpdatePosition()
{	
	FVector DesiredLoc = GetDesiredLocation();


	this->SetWorldLocation(DesiredLoc);
	FVector offset = GetOffsetDirection();
	this->AddWorldOffset(offset);
}

FVector UYogCameraComponent::GetOffsetDirection()
{
	AYogBaseCharacter* OwnerCharacter = Cast<AYogBaseCharacter>(this->GetOwner());
	FVector CurrentVec = OwnerCharacter->GetVelocity();
	FVector NormVec = CurrentVec.GetSafeNormal();

	return NormVec * CameraLerpSpeed;
}


