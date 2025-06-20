// Fill out your copyright notice in the Description page of Project Settings.


#include "YogTimelineComponent.h"

UYogTimelineComponent::UYogTimelineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UYogTimelineComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UYogTimelineComponent::TimelineUpdate(float Value)
{
}

void UYogTimelineComponent::TimelineFinished()
{
}
