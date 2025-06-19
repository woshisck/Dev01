// Fill out your copyright notice in the Description page of Project Settings.


#include "InputBufferComponent.h"


// Sets default values for this component's properties
UInputBufferComponent::UInputBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;


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

void UInputBufferComponent::UpdateActionBuffer(EPlayerActionInput actionInput)
{
	//UpdateBuffer(ActionBuffer, actionInput, MAX_BUFFER_SIZE);


	if (ActionBuffer.Num() < MAX_BUFFER_SIZE)
	{
		ActionBuffer.Insert(actionInput, 0);
		/*FVector2D cache_item = MovementBuffer[0];*/
	}
	else
	{
		ActionBuffer.Pop();
		ActionBuffer.Insert(actionInput, 0);
	}
}

void UInputBufferComponent::UpdateMoveBuffer(FVector2D move)
{
	//UpdateBuffer(this->MovementBuffer, move, this->MAX_BUFFER_SIZE);
	

	if (MovementBuffer.Num() <= MAX_BUFFER_SIZE)
	{
		MovementBuffer.Insert(move, 0);
		/*FVector2D cache_item = MovementBuffer[0];*/
	}
	else
	{
		MovementBuffer.Pop();
		MovementBuffer.Insert(move, 0);
	}

	//UE_LOG(LogTemp, Warning, TEXT("this->MovementBuffer: %d"), this->MovementBuffer.Num());
}



//FVector2D UInputBufferComponent::GetLastFrameInput(FVector2D Movement)
//{
//	return buffer.Pop();
//}

void UInputBufferComponent::ClearActionBuffer()
{
	ClearBuffer(ActionBuffer);
}

void UInputBufferComponent::ClearMovementBuffer()
{
	ClearBuffer(MovementBuffer);
}

void UInputBufferComponent::DebugPrintAction()
{
	int count = 0;
	FString results;
	for (const EPlayerActionInput& Element : ActionBuffer)
	{
		FString cache = "[" + FString::FromInt(count) + "]:" + UEnum::GetDisplayValueAsText(Element).ToString() + " ";
		results += cache;
		count++;
		
	}
	//UE_LOG(LogTemp, Warning, TEXT("Array element: %s"), *results);
}

void UInputBufferComponent::DebugPrintMovement()
{

	int count = 0;
	FString results;
	for (const FVector2D& Element : MovementBuffer)
	{
		
		FString cache = "[" + FString::FromInt(count) + "]:" + Element.ToString()+" ";
		results += cache;
		count++;
	}
	//UE_LOG(LogTemp, Warning, TEXT("Array element: %s"), *results);
}


FVector2D UInputBufferComponent::GetLastMoveInput(FVector2D Movement)
{
	return MovementBuffer.Pop();
	//return GetLastItem(MovementBuffer);
}

EPlayerActionInput UInputBufferComponent::GetLastActionInput(FVector2D Movement)
{
	if (ActionBuffer.Num() > 0)
	{
		return ActionBuffer.Pop();
	}
	else
	{
		return EPlayerActionInput::None;
	}
}


