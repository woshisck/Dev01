// Fill out your copyright notice in the Description page of Project Settings.



#include "YogCameraComponent.h"
#include "../Character/YogCharacterBase.h"

UYogCameraComponent::UYogCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UYogCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	//UpdatePosition();
}

FVector UYogCameraComponent::GetDesiredLocation(const FVector& GroundLoc)
{
	return FVector(0, 0, 0);
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

	return FVector(0, 0, 0);
}


