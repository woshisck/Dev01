// Fill out your copyright notice in the Description page of Project Settings.


#include "InputBufferComponent.h"

// Sets default values for this component's properties
UInputBufferComponent::UInputBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UInputBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInputBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UInputBufferComponent::UpdateMoveBuffer(FVector2D move)
{
	if (MoveBuffer.Top() != move) {
		//update buffer 
		MoveBuffer.Push(move);

	}
	else if (MoveBuffer.Num() > MAX_BUFFER_SIZE) {
		MoveBuffer.Empty();
		MoveBuffer.Push(move);
		
	}
	
}

FVector2D UInputBufferComponent::GetLastFrameInput(TArray<FVector2D>& buffer)
{
	return buffer.Pop();
}

void UInputBufferComponent::ClearActWindowBuffer()
{
}

void UInputBufferComponent::ClearMoveWindowBuffer()
{
}

