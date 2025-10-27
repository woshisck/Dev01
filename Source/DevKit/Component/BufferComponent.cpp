// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BufferComponent.h"
#include "DevKit\AbilitySystem\GameplayEffect\YogGameplayEffect.h"

// Sets default values for this component's properties
UBufferComponent::UBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

UYogGameplayEffect* UBufferComponent::GetItemAt(int index)
{
	return BufferArray[index];
}

void UBufferComponent::MoveToNextItem()
{

	if (CurrentIndex == BufferArray.Num())
	{
		CurrentIndex = 0;
	}
	else
	{
		CurrentIndex++;
	}
}


// Called every frame
void UBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

