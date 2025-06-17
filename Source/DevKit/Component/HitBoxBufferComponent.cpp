// Fill out your copyright notice in the Description page of Project Settings.


#include "HitBoxBufferComponent.h"

// Sets default values for this component's properties
UHitBoxBufferComponent::UHitBoxBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UHitBoxBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UHitBoxBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHitBoxBufferComponent::Clear()
{
	array_HitboxBuffer.Empty();
}

void UHitBoxBufferComponent::Initialize(TArray<FHitBoxData> array_GA)
{
	Clear();
	array_HitboxBuffer.Append(array_GA);
}

void UHitBoxBufferComponent::UpdateTrigger(int index, bool trigger)
{
	if (index < array_HitboxBuffer.Num())
	{
		array_HitboxBuffer[index].HasTriggered = trigger;
	}
}

